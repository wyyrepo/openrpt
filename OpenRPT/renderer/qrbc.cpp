/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2016 by OpenMFG, LLC
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

#include <QString>
#include <QRectF>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QDebug>
#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#endif

#include "parsexmlutils.h"
#include "renderobjects.h"
#include "barcodeitem.h"
#include "zint.h"

void renderQR(QPainter *painter, int /*dpi*/, const QRectF &r, const QString &_str, OROBarcode *bc)
{
  #if QT_VERSION >= 0x050000
  QByteArray ba = QByteArray(bc->format().toStdString().c_str(), bc->format().toStdString().length());
  QJsonDocument doc = QJsonDocument::fromJson(ba);
  QJsonObject obj = doc.object();
  QJsonValue size = obj.value("size");
  QJsonValue autosize = obj.value("autosize");
  QJsonValue errorCorrection = obj.value("errorCorrection");

  QPen pen(Qt::NoPen);
  QBrush brush(QColor("black"));
  painter->save();
  painter->setPen(pen);
  painter->setBrush(brush);

  QString str = _str;
  BarcodeItem bci;
  bci.ar=(Zint::QZint::AspectRatioMode)0;
  bci.bc.setText(str);
  if (errorCorrection.toInt() >= 0)
      bci.bc.setSecurityLevel(errorCorrection.toInt());
  if (!autosize.toBool())
      bci.bc.setWidth(size.toInt());

  bci.bc.setInputMode(UNICODE_MODE);
  bci.bc.setSymbol(BARCODE_QRCODE);
  bci.bc.setWhitespace(0);
  bci.bc.setFgColor(QColor("black"));
  bci.bc.setBgColor(QColor("white"));
  bci.update();

  bci.paint(painter, r);
  bci.update();

  painter->restore();
  #endif
  return;
}
