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

#ifndef LABELPAINTENGINE_H
#define LABELPAINTENGINE_H

#include <QPaintEngine>
#include "reportprinter.h"

class LabelPaintEngine : public QPaintEngine
{
public:
  LabelPaintEngine(ReportPrinter *parentPrinter, QString cmdPrefix);

  virtual bool 	begin ( QPaintDevice * pdev ) = 0;

  virtual void 	drawImage ( const QRectF & rectangle, const QImage & image, const QRectF & sr, Qt::ImageConversionFlags flags = Qt::AutoColor ) = 0;
  virtual void 	drawLines ( const QLineF * lines, int lineCount ) = 0;
  virtual void 	drawRects ( const QRectF * rects, int rectCount ) = 0;
  virtual void 	addEndMessage () = 0;

  virtual void 	drawEllipse ( const QRectF & rect );
  virtual void 	drawEllipse ( const QRect & rect );
  virtual void 	drawPath ( const QPainterPath & path );
  virtual void 	drawPixmap ( const QRectF & r, const QPixmap & pm, const QRectF & sr );
  virtual void 	drawPoints ( const QPointF * points, int pointCount );
  virtual void 	drawPoints ( const QPoint * points, int pointCount );
  virtual void 	drawPolygon ( const QPointF * points, int pointCount, PolygonDrawMode mode );
  virtual void 	drawPolygon ( const QPoint * points, int pointCount, PolygonDrawMode mode );
  virtual void 	drawTextItem ( const QPointF & p, const QTextItem & textItem );
  virtual void 	drawTiledPixmap ( const QRectF & rect, const QPixmap & pixmap, const QPointF & p );

  virtual bool 	end ();
  virtual Type 	type () const;
  virtual void 	updateState ( const QPaintEngineState & state );

  virtual bool newPage();

  void			setPrintToBuffer() {m_printToBuffer = true;}
  QByteArray	getBuffer() const {return m_printBuffer;}

protected:
  virtual void  drawBarcode ( const QPointF & p, const QString &format, int height, int width, int narrowBar, QString barcodeData ) = 0;
  virtual void  drawText ( const QPointF &p, const QString & text, const QFont &font = QFont()) = 0;
  virtual QString rotation0Cmd() const  = 0;
  virtual QString rotation90Cmd() const = 0;
  virtual QString rotation180Cmd() const  = 0;
  virtual QString rotation270Cmd() const  = 0;

  bool    isProportionnal ( const QFont &font ) const;
  QString transformRotationCmd();
  int     resolution() const { return (qreal)m_parentPrinter->resolution(); }  // resolution in points per inches
  int     density() const { return qRound (resolution() / 25.4); }  // density in points per mm
  QString customInitString() const { return m_CustomInitString; }

  ReportPrinter *m_parentPrinter;
  QByteArray m_printBuffer;
  bool		m_printToBuffer;
  QString   m_CmdPrefix;

private:

  QString   m_CustomInitString;
  QTransform m_Rotation90;
  QTransform m_Rotation180;
  QTransform m_Rotation270;
  QString   m_OutputFile;
};

#endif // LABELPAINTENGINE_H
