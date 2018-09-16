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

#ifndef REPORTPRINTER_H
#define REPORTPRINTER_H

#include <QPrinter>
#include <QPair>
#include <QList>


class LabelPaintEngine;

class ReportPrinter : public QPrinter
{
public:

  enum type {Standard, Sato, Zebra};
  enum barcode {Code3of9, Code128, EAN13, EAN8, UPCA, UPCE, DataMatrix};
  static const QString barcodePrefix() { return "<bc>"; }

  ReportPrinter(PrinterMode mode = ScreenResolution, ReportPrinter::type type=Standard);
  ~ReportPrinter();

  bool isLabelPrinter() const;
  void setPrinterType(type type);
  void			setPrintToBuffer();
  QByteArray	getBuffer() const;
  void setParams(QList<QPair<QString,QString> > params) { _params = params; }
  QList<QPair<QString,QString> > getParams() const { return _params; }
  QString getParam(QString key) const;

  int printerMetric(QPaintDevice::PaintDeviceMetric id) const { return metric(id); }


  static   QString getRecallMask(QList<QPair<QString,QString> > params);
  static   QString getStoreMask(QList<QPair<QString,QString> > params);

private:
  LabelPaintEngine*     m_paintEngine;
  QPrintEngine*         m_printEngine;
  type                  m_printerType;
  bool					m_printToBuffer;
  QString				m_cmdPrefix;
  QList<QPair<QString,QString> >  _params;

  void releaseEngines();
};

#endif // REPORTPRINTER_H
