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

#ifndef LABELPRINTENGINE_H
#define LABELPRINTENGINE_H

#include <QPrintEngine>

class LabelPaintEngine;
class ReportPrinter;

class LabelPrintEngine : public QPrintEngine
{
public:
  LabelPrintEngine(LabelPaintEngine *paintEngine, ReportPrinter *printer, int resolution);

  virtual bool	abort () { return true; }
  virtual int	metric ( QPaintDevice::PaintDeviceMetric id ) const;
  virtual bool	newPage ();
  virtual QPrinter::PrinterState	printerState () const;
  virtual QVariant	property ( PrintEnginePropertyKey key ) const;
  virtual void	setProperty ( PrintEnginePropertyKey key, const QVariant & value );

private:
  LabelPaintEngine *m_paintEngine;
  ReportPrinter   *m_printer;
  QString       m_printerName;
  QString       m_docName;
  QSizeF        m_paperSize;
  QRect         m_paperRect;
  int           m_resolution;
};

#endif // LABELPRINTENGINE_H
