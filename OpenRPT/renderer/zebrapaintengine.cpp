/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2014 by OpenMFG, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include <QPrinter>
#include <QPrintEngine>
#include <QFile>
#include <QBuffer>
#include <QtDebug>
#include <QHostInfo>
#include <QTextCodec>

#include "zebrapaintengine.h"
#include "barcodes.h"


static QByteArray compressedHexa(const QByteArray &in)
{
  const int maxOccurrences = 400;
  QByteArray out;
  for (int pos=0; pos<in.length(); pos++) {
    int occurrences = 1;
    while(pos+occurrences < in.length() && in.at(pos+occurrences) == in.at(pos)) {
      occurrences++;
      if(occurrences >= maxOccurrences) {
        break;
      }
    }
    if(occurrences>1) {
      if(occurrences / 20 > 0) {
        char c = 'f' + (occurrences / 20);
        out.append(c);
      }
      if(occurrences % 20 > 0) {
        char c = 'F' + (occurrences % 20);
        out.append(c);
      }
      pos += (occurrences-1);
    }
    out.append(in.at(pos));
  }
  return out;
}


ZebraPaintEngine::ZebraPaintEngine(ReportPrinter *parentPrinter) : LabelPaintEngine(parentPrinter, "^")
{
}


bool 	ZebraPaintEngine::begin ( QPaintDevice * pdev )
{
  Q_UNUSED(pdev);
  QString init = m_CmdPrefix + "XA" + m_CmdPrefix + "LRY";

  if(!customInitString().isEmpty()) {
    init += customInitString();
  }
  else {
    int height = m_parentPrinter->paperRect().height() * (resolution()/72.0); // ?? doc says that paperRect() is in device coordinates, but we get it in PS points
    int width = m_parentPrinter->paperRect().width() * (resolution()/72.0);
    init += QString(m_CmdPrefix + "LL%1" + m_CmdPrefix + "PW%2" + m_CmdPrefix + "CI8").arg(height).arg(width);
  }

  init += "\n";

  m_printBuffer.append(init);
  return true;
}


void 	ZebraPaintEngine::addEndMessage ()
{
  QString printMode = m_parentPrinter->getParam("printmode");
  if(printMode.isEmpty()) {
    printMode = "T";
  }
  m_printBuffer += QString(m_CmdPrefix + "MM%1").arg(printMode);
  m_printBuffer += QString(m_CmdPrefix + "PQ%1%2").arg(m_parentPrinter->numCopies()).arg(m_parentPrinter->getParam("printqty_options"));
  m_printBuffer += QString(m_CmdPrefix + "XZ\n");
}


void ZebraPaintEngine::drawText ( const QPointF &p, const QString & text, const QFont &font )
{
  Q_UNUSED(p);
  QTransform transform = painter()->worldTransform();

  int xInDots = (int)(transform.dx());
  int yInDots = (int)(transform.dy());

  int carHeight = (font.pointSize() * resolution()) / 72;
  const int minSize = 10;
  if(carHeight < minSize) {
    carHeight = minSize;
  }
  int carWidth = qRound((carHeight * 15.0) / 28.0); // fixed width/height ratio for the scalable A0 font

  QString rotation = transformRotationCmd();
  if(rotation == rotation90Cmd()) {
    xInDots -= carHeight;
  }
  else if(rotation == rotation180Cmd()) {
    yInDots -= carHeight;
    xInDots -= (carWidth * text.length());
  }
  else if(rotation == rotation270Cmd()) {
    yInDots -= (carWidth * text.length());
  }

  QString output = QString(m_CmdPrefix + "FO%1,%2" + m_CmdPrefix + "FW%3").arg(xInDots).arg(yInDots).arg(rotation);
  output += QString(m_CmdPrefix + "A0,%1,0" + m_CmdPrefix + "FD" + text + m_CmdPrefix + "FS\n").arg(carHeight);

  QTextCodec *codec = QTextCodec::codecForName("IBM 850");
  m_printBuffer.append(codec->fromUnicode(output));
}


void ZebraPaintEngine::drawBarcode ( const QPointF & p, const QString &format, int height, int width, int narrowBar, QString barcodeData )
{
  QString barcodeFont;
  if(format == "3of9" || format == "3of9+")
    barcodeFont = "B3";
  else if(format == "128")
    barcodeFont = "BC,,N,N,N,A";
  else if(format == "ean13" || format == "upc-a")
    barcodeFont = "B8";
  else if(format == "ean8")
    barcodeFont = "B8";
  else if(format == "upc-e")
    barcodeFont = "B9";
  else if(format == "i2of5")
    barcodeFont = "BI";
  else if(format.contains("datamatrix"))
  {
    DmtxInfos dmtxInfos = extractInfosDtmx(format);
    int eltSize = qRound (qBound((qreal)2.0, (qreal)height / (qreal)dmtxInfos.ySize, (qreal)20.0));
    barcodeFont = QString("BX,%1,200,%2,%3").arg(eltSize).arg(dmtxInfos.xSize).arg(dmtxInfos.ySize);
  }
  else {
    drawText(p, "ERR: " + format);
  }

  QTransform transform = painter()->worldTransform();

  int yInDots = (int)(transform.dy());
  int xInDots = (int)(transform.dx());

  QString rotation = transformRotationCmd();
  if(rotation == rotation90Cmd()) {
    xInDots -= height;
  }
  else if(rotation == rotation180Cmd()) {
    yInDots -= height;
    // we can't calculate the printed barcode width, so we can't print 180� and 270� correctly! Well done Zebra!
    // we use the TextItem's width instead as a fallback
    xInDots -= width;
  }
  else if(rotation == rotation270Cmd()) {
    yInDots -= width;
  }

  qreal narrowWidthRatio = 2.5;

  m_printBuffer += QString(m_CmdPrefix + "FO%1,%2" + m_CmdPrefix + "FW%3").arg(xInDots).arg(yInDots).arg(transformRotationCmd());
  m_printBuffer += QString(m_CmdPrefix + "BY%1,%2,%3" + m_CmdPrefix + barcodeFont + m_CmdPrefix + "FD" + barcodeData + m_CmdPrefix + "FS\n").arg(narrowBar).arg(narrowWidthRatio).arg(height);
}


void ZebraPaintEngine::drawImage ( const QRectF & rectangle, const QImage & image, const QRectF & sr, Qt::ImageConversionFlags flags )
{
  Q_UNUSED(sr);
  Q_UNUSED(flags);
  QImage monoImage = image.convertToFormat(QImage::Format_Mono);
/*
  //PNG encoding (untested)
  QByteArray encodedImage;
  QBuffer buffer(&encodedImage);
  buffer.open(QIODevice::Append);
  monoImage.save(&buffer, "PNG");

  // Hexa-encoded PNG
  m_printBuffer += QString("~DYR:IMG,P,G,%1,0,").arg(encodedImage.size());
  for(int i=0; i<encodedImage.size(); i++) {
    m_printBuffer.append(QString().sprintf("%02X",(unsigned char)encodedImage.at(i)).toUpper());
  }
  // OR: B64-encoded PNG
  m_printBuffer += QString("~DYR:IMG,P,G,%1,0,:B64:").arg(encodedImage.size());
  encodedImage = encodedImage.toBase64();
  m_printBuffer.append(encodedImage);

  quint16 icrc = qChecksum(encodedImage.data(), encodedImage.length());
  m_printBuffer += ":";
  m_printBuffer.append(QString().sprintf("%02X",icrc));
  // end of B64 encoding
*/
  int width = monoImage.width();
  int nbOfLines = monoImage.height();

  QByteArray grfImage;

  for(int line=0; line < nbOfLines; line++) {
    for (int x=0; x<width; x+=8) {
      uchar imageByte = monoImage.scanLine(line)[x/8];
      int validPix = width-x;
      if(validPix<8) { // set out-of-bounds pixels to zero
        imageByte >>= 8-validPix;
        imageByte <<= 8-validPix;
      }
      grfImage.append(QString().sprintf("%02X",imageByte));
    }
  }

  int bytesPerLine = (width+7)/8;
  m_printBuffer += QString("~DGR:IMG.GRF,%1,%2,").arg(bytesPerLine*nbOfLines).arg(bytesPerLine);
  m_printBuffer += compressedHexa(grfImage);

  m_printBuffer += "\n";

  QTransform transform = painter()->worldTransform();

  int xInDots = (int)(rectangle.top() + transform.dx());
  int yInDots = (int)(rectangle.left() + transform.dy());

  m_printBuffer += QString(m_CmdPrefix + "FO%1,%2" + m_CmdPrefix + "XGR:IMG.GRF,1,1" + m_CmdPrefix + "FS\n").arg(xInDots).arg(yInDots);
}


void 	ZebraPaintEngine::drawLines ( const QLineF * lines, int lineCount )
{
  for (int i=0; i< lineCount; i++) {

    QTransform transform = painter()->worldTransform();

    int xInDots = (int)(lines[i].x1() + transform.dx());
    int yInDots = (int)(lines[i].y1() + transform.dy());

    int length = (int) lines[i].length();
    int widthInDots = 0;
    int heightInDots = 0;

    qreal angle = lines[i].angle();
    if(angle==0) {
      widthInDots = length;
      heightInDots = 0;
    }
    else if(angle<=90) {
      widthInDots = 0;
      heightInDots = length;
      yInDots -= length;
    }
    else if(angle<=180) {
      widthInDots = length;
      heightInDots = 0;
      xInDots -= length;
    }
    else {
      widthInDots = 0;
      heightInDots = length;
    }

    int thickness = painter()->pen().width();
    if(thickness<=0) {
      thickness = 1;
    }

    m_printBuffer += QString(m_CmdPrefix + "FO%1,%2").arg(xInDots).arg(yInDots);
    m_printBuffer += QString(m_CmdPrefix + "GB%1,%2,%3" + m_CmdPrefix + "FS\n").arg(widthInDots).arg(heightInDots).arg(thickness);
  }
}


void 	ZebraPaintEngine::drawRects ( const QRectF * rects, int rectCount )
{
  for (int i=0; i< rectCount; i++) {

    QTransform transform = painter()->worldTransform();

    int xInDots = (int)(rects[i].left() + transform.dx());
    int yInDots = (int)(rects[i].top() + transform.dy());

    int width = (int)(rects[i].width());
    int height = (int)(rects[i].height());

    int thickness = painter()->pen().width();
    if(painter()->brush().style() != Qt::NoBrush && painter()->brush().color()==Qt::black) {
      thickness = qMin(width, height); // filled rectangle
    }
    else if(thickness<=0) {
      thickness = 1;
    }

    m_printBuffer += QString(m_CmdPrefix + "FO%1,%2").arg(xInDots).arg(yInDots);
    m_printBuffer += QString(m_CmdPrefix + "GB%1,%2,%3" + m_CmdPrefix + "FS\n").arg(width).arg(height).arg(thickness);
  }
}

