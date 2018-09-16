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
#include "labelpaintengine.h"





LabelPaintEngine::LabelPaintEngine(ReportPrinter *parentPrinter, QString cmdPrefix) : QPaintEngine(QPaintEngine::AllFeatures), m_parentPrinter(parentPrinter), m_printToBuffer(false), m_CmdPrefix(cmdPrefix)
{
  m_Rotation90.rotate(90);
  m_Rotation180.rotate(180);
  m_Rotation270.rotate(270);

  QString customCmdPrefix = m_parentPrinter->getParam("cmdprefix");
  if(!customCmdPrefix.isEmpty()) {
    m_CmdPrefix = customCmdPrefix;
  }

  m_CustomInitString = m_parentPrinter->getParam("init");
  m_OutputFile =  m_parentPrinter->getParam("tofile");
}

QString LabelPaintEngine::transformRotationCmd()
{
  QString rotation = rotation0Cmd();
  QTransform transform = painter()->transform();
  if(transform.m21()==m_Rotation90.m21() && transform.m12()==m_Rotation90.m12()) {
    rotation = rotation90Cmd();
  }
  else if(transform.m11()==m_Rotation180.m11() && transform.m22()==m_Rotation180.m22()) {
    rotation = rotation180Cmd();
  }
  else if(transform.m21()==m_Rotation270.m21() && transform.m12()==m_Rotation270.m12()) {
    rotation = rotation270Cmd() ;
  }

  return rotation;
}

void LabelPaintEngine::drawTextItem ( const QPointF & p, const QTextItem & textItem )
{
  if(textItem.text().startsWith(ReportPrinter::barcodePrefix())) {

    QStringList textElts = textItem.text().mid(ReportPrinter::barcodePrefix().length()).split(';');
    QString format = textElts.value(0);
    int	height =  (int) (textElts.value(1).toDouble() * resolution());
    int	narrowBar = (int) (textElts.value(2).toDouble() * resolution());
    if (narrowBar <=1) narrowBar = 2;
    QString barcodeData = textElts.value(3);

    drawBarcode(p, format, height, textItem.width(), narrowBar, barcodeData);
  }
  else {
    drawText(p, textItem.text(), textItem.font());
  }
}


bool LabelPaintEngine::isProportionnal ( const QFont &font ) const
{
  QFontMetrics fm(font);
  return fm.width("i") < fm.width("w");
}


QPaintEngine::Type 	LabelPaintEngine::type () const
{
    return QPaintEngine::User;
}


void 	LabelPaintEngine::updateState ( const QPaintEngineState & state )
{
  Q_UNUSED(state);
}


bool	LabelPaintEngine::newPage ()
{
    return true;
}


bool 	LabelPaintEngine::end ()
{
  addEndMessage();

  if (m_printToBuffer)
    return true;

  QString outPutName;

  if(!m_OutputFile.isEmpty()) {
    outPutName = m_OutputFile;
  }
  else {
    outPutName = m_parentPrinter->printerName();
#ifdef Q_WS_WIN
    if(!outPutName.startsWith("\\")) {
      outPutName = "\\\\" + QHostInfo::localHostName() + "\\"+ outPutName.remove("/");
    }
#endif
  }

  QFile f(outPutName);
  bool ok = f.open(QIODevice::WriteOnly);
  if(!ok) {
    qWarning() << "Invalid printer name:" << f.fileName();
    return false;
  }

  qint64 bytesWritten = f.write(m_printBuffer);
  if(bytesWritten==0) {
    qWarning() << "Failed to print to:" << f.fileName();
    return false;
  }

  return true;
}


void 	LabelPaintEngine::drawEllipse ( const QRectF & rect )
{
  Q_UNUSED(rect);
}


void 	LabelPaintEngine::drawEllipse ( const QRect & rect )
{
  Q_UNUSED(rect);
}


void 	LabelPaintEngine::drawPixmap ( const QRectF & r, const QPixmap & pm, const QRectF & sr )
{
  Q_UNUSED(r);
  Q_UNUSED(pm);
  Q_UNUSED(sr);
}


void 	LabelPaintEngine::drawPath ( const QPainterPath & path )
{
  Q_UNUSED(path);
}


void 	LabelPaintEngine::drawPoints ( const QPointF * points, int pointCount )
{
  Q_UNUSED(points);
  Q_UNUSED(pointCount);
}


void 	LabelPaintEngine::drawPoints ( const QPoint * points, int pointCount )
{
  Q_UNUSED(points);
  Q_UNUSED(pointCount);
}


void 	LabelPaintEngine::drawPolygon ( const QPointF * points, int pointCount, PolygonDrawMode mode )
{
  Q_UNUSED(points);
  Q_UNUSED(pointCount);
  Q_UNUSED(mode);
}


void 	LabelPaintEngine::drawPolygon ( const QPoint * points, int pointCount, PolygonDrawMode mode )
{
  Q_UNUSED(points);
  Q_UNUSED(pointCount);
  Q_UNUSED(mode);
}


void 	LabelPaintEngine::drawTiledPixmap ( const QRectF & rect, const QPixmap & pixmap, const QPointF & p )
{
  Q_UNUSED(rect);
  Q_UNUSED(pixmap);
  Q_UNUSED(p);
}

