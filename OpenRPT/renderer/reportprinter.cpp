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

#include "reportprinter.h"
#include "labelpaintengine.h"
#include "satopaintengine.h"
#include "satoprintengine.h"
#include "zebrapaintengine.h"
#include "zebraprintengine.h"


static QString getParamValue(QString key, QList<QPair<QString,QString> > list)
{
  for(QList<QPair<QString,QString> >::iterator it = list.begin(); it != list.end(); it++) {
     if((*it).first == key) {
      return (*it).second;
    }
  }
  return "";
}

QString ReportPrinter::getRecallMask(QList<QPair<QString,QString> > params)
{
  QString id = getParamValue("recall", params);
  return id;
}

QString ReportPrinter::getStoreMask (QList<QPair<QString,QString> > params)
{
  QString id = getParamValue("store", params);
  return id;
}

QString ReportPrinter::getParam(QString key) const
{
  return getParamValue(key, _params);
}


ReportPrinter::ReportPrinter(PrinterMode mode, ReportPrinter::type type) : QPrinter(mode), m_paintEngine(NULL), m_printEngine(NULL), m_printerType(Standard), m_printToBuffer(false)
{
  setPrinterType(type);
}

ReportPrinter::~ReportPrinter()
{
  releaseEngines();
}

bool ReportPrinter::isLabelPrinter() const
{
  return m_printerType!=Standard;
}

void ReportPrinter::releaseEngines()
{
  if (m_paintEngine) delete m_paintEngine;
  if (m_printEngine) delete m_printEngine;
  m_paintEngine = 0;
  m_printEngine = 0;
}

void ReportPrinter::setPrintToBuffer() {
    m_printToBuffer = true;
	if (m_paintEngine != 0)
        m_paintEngine->setPrintToBuffer();
}

void ReportPrinter::setPrinterType(type type)
{
  if(outputFormat() != QPrinter::NativeFormat) {
    type = Standard;
  }

  if(type == m_printerType) {
    return;
  }

  // NB engines switch resets printer properties, we have to save them
  // (and also note that the print dialog can't support and custom printers, so it has to be called before switching to a custom type)
  QString savePrinterName = printerName();

  releaseEngines();

  switch (type) {
  case Sato:
    m_paintEngine = new SatoPaintEngine(this);
    if (m_printToBuffer)
      m_paintEngine->setPrintToBuffer();

    m_printEngine = new SatoPrintEngine(m_paintEngine, this);
    break;

  case Zebra:
    m_paintEngine = new ZebraPaintEngine(this);
    if (m_printToBuffer)
      m_paintEngine->setPrintToBuffer();

    m_printEngine = new ZebraPrintEngine(m_paintEngine, this);
    break;

  default:
    break;
  }

  if(m_paintEngine && m_printEngine) {
    setEngines(m_printEngine, m_paintEngine);
  }
  m_printerType = type;
  setPrinterName(savePrinterName);
}


QByteArray ReportPrinter::getBuffer() const {
	if (m_paintEngine == 0)
		return QByteArray();
	return m_paintEngine->getBuffer();
}

