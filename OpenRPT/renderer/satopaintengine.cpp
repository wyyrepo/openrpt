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
#include <QtDebug>
#include <QHostInfo>
#include <QTextCodec>

#include "satopaintengine.h"
#include "barcodes.h"



// numericStretch: returns the number of digits from the beginning of the string
static int numericStretch (const QString &s, int pos=0)
{
  int nb = 0;
  for( ; pos < s.length(); pos++) {
    if(s[pos].isDigit()) {
      nb++;
    }
    else {
      break;
    }
  }

  return nb;
}


static QString addCode128Subset(const QString &s)
{
  if(s.startsWith('>'))
    return s; // the input already has a subset start code

  QString res;
  int pos = 0;
  bool subSetC = false;
  while (pos < s.length()) {
    int nbOfNumCar = numericStretch(s, pos);
    if(nbOfNumCar % 2) {
      nbOfNumCar = 0;
    }
    if(nbOfNumCar>=4 && (!subSetC || pos==0)) {
      res += (pos==0 ? ">I" : ">C");
      subSetC = true;
    }
    else if(subSetC || pos==0) {
      res += (pos==0 ? ">H" : ">D");
      subSetC = false;
    }

    res += s.mid(pos, nbOfNumCar);
    pos += nbOfNumCar;
    if(!subSetC && pos < s.length()) {
       res += s.mid(pos, 1);
       pos ++;
    }
  }

  return res;
}


SatoPaintEngine::SatoPaintEngine(ReportPrinter *parentPrinter) : LabelPaintEngine(parentPrinter, "\x1B")
{
}


bool 	SatoPaintEngine::begin ( QPaintDevice * pdev )
{
  Q_UNUSED(pdev);

  int height = m_parentPrinter->paperRect().height() * (resolution()/72.0); // ?? doc says that paperRect() is in device coordinates, but we get it in PS points
  int width = m_parentPrinter->paperRect().width() * (resolution()/72.0);

  QString init = m_CmdPrefix + "A";
  if (height > 1780 || width > 1780) {
      init += m_CmdPrefix + "AX"; // big size
  }
  else {
      init += m_CmdPrefix + "AR"; // standard size
  }
  init += "\n";

  if(!customInitString().isEmpty()) {
    init += customInitString() + "\n";
  }

  init += m_CmdPrefix + QString("A1%1%2\n").arg(height, 4, 10, QLatin1Char('0')).arg(width, 4, 10, QLatin1Char('0'));

  QString recallMask = ReportPrinter::getRecallMask(m_parentPrinter->getParams());
  if(recallMask.length()==3) {
    QChar memory = recallMask[0];
    if(memory=='1' || memory=='2') {
      init += m_CmdPrefix + "CC" + memory;
      init += m_CmdPrefix + "&R" + "," + recallMask.right(2);
      init += "\n";
    }
  }

  m_printBuffer.append(init);

  return true;
}


void 	SatoPaintEngine::addEndMessage ()
{
  // white on black areas
  foreach(QRect r, m_ReverseZones) {
    QString output = QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2" + m_CmdPrefix + "(%3,%4\n")
                    .arg(QString::number(r.top())).arg(QString::number(r.left())).arg(QString::number(r.width())).arg(QString::number(r.height()));
    m_printBuffer.append(output);
  }

  m_ReverseZones.clear();

  QString output;
  QString storeMask = ReportPrinter::getStoreMask(m_parentPrinter->getParams());
  if(storeMask.length()==3) {
    QChar memory = storeMask[0];
    if(memory=='1' || memory=='2') {
      output += m_CmdPrefix + "CC" + memory;
      output += m_CmdPrefix + "&S" + "," + storeMask.right(2);
      output += "\n";
    }
  }
  if(output.isEmpty()) {
    output = QString(m_CmdPrefix + "Q%1").arg(m_parentPrinter->numCopies());
  }

  output += QString(m_CmdPrefix + "Z");
  m_printBuffer.append(output);
}

void SatoPaintEngine::drawText ( const QPointF &p, const QString & text, const QFont &font )
{
  Q_UNUSED(p);
  QTransform transform = painter()->worldTransform();

  int xInDots = (int)(transform.dx());
  int yInDots = (int)(transform.dy());

  // 1 font point = 1/72 inches
  int fontSizeInPixels = (font.pointSize() * resolution()) / 72;
  const int vectorFontSizeMin = 22;
  const int vectorFontSizeMinPrintable = 24;

  bool nonProportional = !isProportionnal(font);
  bool narrow = nonProportional || font.family().contains("narrow", Qt::CaseInsensitive);
  QString satoFont;
  if(fontSizeInPixels < vectorFontSizeMin) {
    QString fC = fontSizeInPixels<12 ? "U" : "S";
    satoFont = nonProportional ? fC : ("P" + fC + m_CmdPrefix + "X" + fC);
  }
  else {
    if(fontSizeInPixels < vectorFontSizeMinPrintable) {
      fontSizeInPixels = vectorFontSizeMinPrintable;
    }
    char fontType = nonProportional ? 'B' : 'A';
    int variation = font.italic() ? 8 : 0;
    satoFont = QString().sprintf("$%c,%03d,%03d,%d", fontType, fontSizeInPixels, fontSizeInPixels, variation) + m_CmdPrefix + "$=";
  }

  int pitch = (narrow | nonProportional) ? 2 : fontSizeInPixels/8;
  QString pitchCmd = pitch<=3 ? "" : m_CmdPrefix + QString().sprintf("P%02d", pitch);
  m_printBuffer.append(pitchCmd);

  QString output = QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2" + m_CmdPrefix + "%3" + m_CmdPrefix + "%4" + "%5\n")
                        .arg(QString::number(yInDots), QString::number(xInDots), transformRotationCmd(), satoFont, text);

  QTextCodec *codec = QTextCodec::codecForName("IBM 850");
  m_printBuffer.append(codec->fromUnicode(output));
}


void SatoPaintEngine::drawBarcode ( const QPointF & p, const QString &format, int height, int width, int narrowBar, QString barcodeData )
{
  Q_UNUSED(width);

  QString barcodeFontCmd = QString().sprintf("%02d%03d", narrowBar, height);

  QString barcodeFont;
  if(format == "3of9" || format == "3of9+") {
    barcodeFont = "B1";
    if(!barcodeData.startsWith('*')) {
      barcodeData = "*" + barcodeData + "*";
    }
  }
  else if(format == "128") {
    barcodeFont = "BG";
    barcodeData = addCode128Subset(barcodeData);
  }
  else if(format == "ean13" || format == "upc-a")
    barcodeFont = "BD3";
  else if(format == "ean8")
    barcodeFont = "BD4";
  else if(format == "upc-e")
    barcodeFont = "BD3";
  else if(format == "i2of5")
    barcodeFont = "BD2";
  else if(format.contains("datamatrix"))
  {
    DmtxInfos dmtxInfos = extractInfosDtmx(format);
    int eltSize = qRound (qBound((qreal)2.0, (qreal)height / (qreal)dmtxInfos.ySize, (qreal)20.0));
    barcodeFont = QString("BX0620%1%2%3%4")
        .arg(eltSize,2,10,QLatin1Char('0'))
        .arg(eltSize,2,10,QLatin1Char('0'))
        .arg(dmtxInfos.xSize,3,10,QLatin1Char('0'))
        .arg(dmtxInfos.ySize,3,10,QLatin1Char('0'))
        + "001";

    barcodeFontCmd = m_CmdPrefix + "DC";
  }
  else {
    drawText(p, "ERR: " + format);
  }

  QTransform transform = painter()->worldTransform();

  int xInDots = (int)(transform.dx());
  int yInDots = (int)(transform.dy());

  QString output = QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2" + m_CmdPrefix + "%3" + m_CmdPrefix + "%4" + "%5\n")
                        .arg(QString::number(yInDots), QString::number(xInDots), transformRotationCmd(), barcodeFont+barcodeFontCmd, barcodeData);
  m_printBuffer.append(output);
}


void SatoPaintEngine::drawImage ( const QRectF & rectangle, const QImage & image, const QRectF & sr, Qt::ImageConversionFlags flags )
{
  Q_UNUSED(sr);
  Q_UNUSED(flags);
  QTransform transform = painter()->worldTransform();
  int xInDots = (int)(rectangle.left() + transform.dx());
  int yInDots = (int)(rectangle.top() + transform.dy());

  QImage monoImage = image.convertToFormat(QImage::Format_Mono);
  int missingLines = (8 - (monoImage.height() % 8)) % 8;
  int width = monoImage.width();
  int nbOfLines = monoImage.height() + missingLines;
  int blockSize = 32; // vertical block of lines, output on a single command line

  for (int x=0; x<width; x+=8) {

    for(int line=0; line < nbOfLines; line++) {

      QString output;
      if(line % blockSize == 0) {
        if(line > 0) {
          output.append('\n');
        }
        output += QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2").arg(QString::number(yInDots+line), QString::number(xInDots+x));
        int nbOfBlocks = qMin(nbOfLines-line, blockSize)/8;
        output += m_CmdPrefix + QString().sprintf("GH%03d%03d", 1, nbOfBlocks);
      }

      if(line<monoImage.height()) {
        uchar imageByte = monoImage.scanLine(line)[x/8];
        int validPix = width-x;
        if(validPix<8) { // set out-of-bounds pixels to zero
          imageByte >>= 8-validPix;
          imageByte <<= 8-validPix;
        }
        output.append(QString().sprintf("%02X",imageByte).toUpper());
      }
      else {
        output.append("00");
      }

      m_printBuffer.append(output);

    }
    m_printBuffer.append('\n');
  }
}


void 	SatoPaintEngine::drawLines ( const QLineF * lines, int lineCount )
{
  for (int i=0; i< lineCount; i++) {

    QTransform transform = painter()->worldTransform();

    int xInDots = (int)(lines[i].x1() + transform.dx());
    int yInDots = (int)(lines[i].y1() + transform.dy());

    QString rotation;
    qreal angle = lines[i].angle();
    if(angle==0) {
      rotation = rotation0Cmd();
    }
    else if(angle<=90) {
      rotation = rotation270Cmd();
    }
    else if(angle<=180) {
      rotation = rotation180Cmd();
    }
    else {
      rotation = rotation90Cmd();
    }

    int thickness = painter()->pen().width();
    if(thickness<=0) {
      thickness = 1;
    }
    int length = (int) lines[i].length();
    QString end = QString().sprintf("FW%02dH%04d", thickness, length);

    QString output = QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2" + m_CmdPrefix + "%3" + m_CmdPrefix + "%4\n")
                          .arg(QString::number(yInDots), QString::number(xInDots), rotation, end);
    m_printBuffer.append(output);
  }
}


void 	SatoPaintEngine::drawRects ( const QRectF * rects, int rectCount )
{
  for (int i=0; i< rectCount; i++) {

    QTransform transform = painter()->worldTransform();

    int xInDots = (int)(rects[i].left() + transform.dx());
    int yInDots = (int)(rects[i].top() + transform.dy());
    int width = (int)(rects[i].width());
    int height = (int)(rects[i].height());

    if(painter()->brush().style() != Qt::NoBrush && painter()->brush().color()==Qt::black) {
      m_ReverseZones.append(QRect(xInDots, yInDots, width, height));
    }
    else {
      QString rotation = rotation0Cmd(); // rotation not supported

      int thickness = painter()->pen().width();
      if(thickness<=0) {
        thickness = 1;
      }
      QString end = QString().sprintf("FW%02d%02dV%04dH%04d", thickness, thickness, height, width);

      QString output = QString(m_CmdPrefix + "V%1" + m_CmdPrefix + "H%2" + m_CmdPrefix + "%3" + m_CmdPrefix + "%4\n")
                            .arg(QString::number(yInDots), QString::number(xInDots), rotation, end);
      m_printBuffer.append(output);
    }
  }
}



