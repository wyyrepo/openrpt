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

#ifndef __BARCODES_H__
#define __BARCODES_H__

#include <QRect>
#include <QString>

class ORBarcodeData;
class OROBarcode;
class OROPage;

//
// 3of9
//
void render3of9(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);

//
// 3of9+
//
void renderExtended3of9(QPainter *, int, const QRectF &, const QString &, OROBarcode *bc);

//
// Interleaved 2 of 5
//
void renderI2of5(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);

//
// Code 128
//
void renderCode128(QPainter *, int, const QRectF &r, const QString &_str, OROBarcode *bc);

//
// Code EAN/UPC
//
void renderCodeEAN13(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);
void renderCodeEAN8(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);
void renderCodeUPCA(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);
void renderCodeUPCE(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);

//
// CodeDataMatrix
//

void renderCodeDatamatrix(QPainter *, const QRectF &, const QString &, OROBarcode * bc);

//
// PDF417
//

void renderPDF417(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);

//
// QR
//

void renderQR(QPainter *, int, const QRectF &, const QString &, OROBarcode * bc);

typedef struct DmtxInfos_struct {
   int type;
   int xSize;
   int ySize;
   QString align;
} DmtxInfos;

DmtxInfos extractInfosDtmx(const QString &s);

#endif

