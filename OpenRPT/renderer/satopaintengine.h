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

#ifndef SATOPAINTENGINE_H
#define SATOPAINTENGINE_H

#include "labelpaintengine.h"
#include "reportprinter.h"

class SatoPaintEngine : public LabelPaintEngine
{
public:
  SatoPaintEngine(ReportPrinter *parentPrinter);

  virtual bool 	begin ( QPaintDevice * pdev );

  virtual void 	drawImage ( const QRectF & rectangle, const QImage & image, const QRectF & sr, Qt::ImageConversionFlags flags = Qt::AutoColor );
  virtual void 	drawLines ( const QLineF * lines, int lineCount );
  virtual void 	drawRects ( const QRectF * rects, int rectCount );

  virtual void 	addEndMessage ();


protected:
  virtual void  drawBarcode ( const QPointF & p, const QString &format, int height, int width, int narrowBar, QString barcodeData );
  virtual void  drawText ( const QPointF &p, const QString & text, const QFont &font = QFont());
  virtual QString rotation0Cmd() const { return "%0"; }
  virtual QString rotation90Cmd() const { return "%3"; }
  virtual QString rotation180Cmd() const { return "%2"; }
  virtual QString rotation270Cmd() const { return "%1"; }


private:

  QList<QRect> m_ReverseZones;

};

#endif // SATOPAINTENGINE_H
