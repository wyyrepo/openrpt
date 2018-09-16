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
#include "labelprintengine.h"
#include "labelpaintengine.h"
#include "reportprinter.h"


LabelPrintEngine::LabelPrintEngine(LabelPaintEngine *paintEngine, ReportPrinter *printer, int resolution) : m_paintEngine(paintEngine), m_printer(printer), m_resolution(resolution)
{
  QString customResolution = printer->getParam("dpi");
  if(customResolution.toInt() > 0) {
    m_resolution = customResolution.toInt();
  }
}


QPrinter::PrinterState	LabelPrintEngine::printerState () const
{
  if(m_printer->paintingActive()) {
    return QPrinter::Active;
  }
  else {
    return QPrinter::Idle;
  }
}

int	LabelPrintEngine::metric ( QPaintDevice::PaintDeviceMetric id ) const
{
  int val = 1;

  switch (id) {
  case QPaintDevice::PdmWidth:
      break;
  case QPaintDevice::PdmHeight:
      break;
  case QPaintDevice::PdmDpiX:
  case QPaintDevice::PdmDpiY:
  case QPaintDevice::PdmPhysicalDpiX:
  case QPaintDevice::PdmPhysicalDpiY:
    val = m_resolution;
    break;

  case QPaintDevice::PdmWidthMM:
      break;
  case QPaintDevice::PdmHeightMM:
    break;
  case QPaintDevice::PdmNumColors:
    val = 2;
    break;
  case QPaintDevice::PdmDepth:
    val = 1;
    break;
  default:
      qWarning("QPrinter::metric: Invalid metric command");
      val = 0;
  }
  return val;
}

void	LabelPrintEngine::setProperty ( PrintEnginePropertyKey key, const QVariant & value )
{
  switch(key) {
  case  QPrintEngine::PPK_PrinterName:
    m_printerName = value.toString();
    break;
  case  QPrintEngine::PPK_DocumentName:
    m_docName = value.toString();
    break;
  case  QPrintEngine::PPK_CustomPaperSize:
    m_paperSize = value.toSizeF();
    m_paperRect = QRect(QPoint(0,0),m_paperSize.toSize());
    break;
  case  QPrintEngine::PPK_PaperRect:
  case  QPrintEngine::PPK_PageRect:
    m_paperSize = value.toRect().size();
    m_paperRect = value.toRect();
    break;
  case  QPrintEngine::PPK_PageSize:
    // not implemented
    break;
  case  QPrintEngine::PPK_Resolution:
    m_resolution = value.toInt();
    break;
  default: break;
  }
}

QVariant	LabelPrintEngine::property ( PrintEnginePropertyKey key ) const
{
  switch(key) {
  case  QPrintEngine::PPK_PrinterName:
    return QVariant(m_printerName);

  case  QPrintEngine::PPK_DocumentName:
    return QVariant(m_docName);

  case  QPrintEngine::PPK_PaperRect:
  case  QPrintEngine::PPK_PageRect:
    return QVariant(m_paperRect);

  case  QPrintEngine::PPK_CustomPaperSize:
    return QVariant(m_paperSize);

  case  QPrintEngine::PPK_Resolution:
    return QVariant(m_resolution);

  // case  QPrintEngine::PPK_CopyCount:
  case  QPrintEngine::PPK_NumberOfCopies:
    return QVariant(1);

  default: return QVariant();
  }
}

bool	LabelPrintEngine::newPage ()
{
  return m_paintEngine->newPage();
}

