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

#include "pagesizeinfo.h"

#include <qprinter.h>

static PageSizeInfo __pages[] = {
  PageSizeInfo("A0", QPrinter::A0, 3311, 4681),
  PageSizeInfo("A1", QPrinter::A1, 2340, 3311),
  PageSizeInfo("A2", QPrinter::A2, 1654, 2340),
  PageSizeInfo("A3", QPrinter::A3, 1170, 1654),
  PageSizeInfo("A4", QPrinter::A4, 826, 1170),
  PageSizeInfo("A5", QPrinter::A5, 583, 826),
  PageSizeInfo("A6", QPrinter::A6, 413, 583),
  PageSizeInfo("A7", QPrinter::A7, 291, 413),
  PageSizeInfo("A8", QPrinter::A8, 205, 291),
  PageSizeInfo("A9", QPrinter::A9, 146, 205),
  PageSizeInfo("B0", QPrinter::B0, 4055, 5732),
  PageSizeInfo("B1", QPrinter::B1, 2866, 4055),
  PageSizeInfo("B2", QPrinter::B2, 2028, 2866),
  PageSizeInfo("B3", QPrinter::B3, 1433, 2028),
  PageSizeInfo("B4", QPrinter::B4, 1013, 1433),
  PageSizeInfo("B5", QPrinter::B5, 717, 1013),
  PageSizeInfo("B6", QPrinter::B6, 504, 717),
  PageSizeInfo("B7", QPrinter::B7, 358, 504),
  PageSizeInfo("B8", QPrinter::B8, 252, 358),
  PageSizeInfo("B9", QPrinter::B9, 177, 252),
  PageSizeInfo("B10", QPrinter::B10, 126, 177),
  PageSizeInfo("C5E", QPrinter::C5E, 642, 902),
  PageSizeInfo("Comm10E", QPrinter::Comm10E, 413, 950),
  PageSizeInfo("DLE", QPrinter::DLE, 433, 866),
  PageSizeInfo("Executive", QPrinter::Executive, 750, 1000),
  PageSizeInfo("Folio", QPrinter::Folio, 827, 1300),
  PageSizeInfo("Ledger", QPrinter::Ledger, 1701, 1098),
  PageSizeInfo("Legal", QPrinter::Legal, 850, 1400),
  PageSizeInfo("Letter", QPrinter::Letter, 850, 1100),
  PageSizeInfo("Tabloid", QPrinter::Tabloid, 1098, 1701),
  PageSizeInfo()                // null page size
};


const PageSizeInfo & PageSizeInfo::getByName(const QString & name)
{
  int i = 0;
  while (!__pages[i].isNull() && __pages[i]._name != name)
    i++;
  return __pages[i];
}
const PageSizeInfo & PageSizeInfo::getByValue(int value)
{
  int i = 0;
  while (!__pages[i].isNull() && __pages[i]._qpValue != value)
    i++;
  return __pages[i];
}

const PageSizeInfo & PageSizeInfo::findNearest(int w, int h)
{
  int p = -1;
  int d = 99999;
  for(int i = 0; !__pages[i].isNull(); i++)
  {
    if(w <= __pages[i].width() && h <= __pages[i].height())
    {
      int t = (__pages[i].width() - w) + (__pages[i].height() - h);
      if(t < d)
      {
        p = i;
        d = t;
      }
    }
  }

  if(p == -1)
    return getByName("Letter");

  return __pages[p];
}

// Added this function to obtain the entire list of page size names
QStringList PageSizeInfo::getPageNames()
{
  QStringList l;
  for (int i = 0; !__pages[i].isNull(); i++)
    l.append(__pages[i]._name);
  return l;
}

PageSizeInfo::PageSizeInfo(const QString & n, int qpv, int w, int h)
{
  _name = n;
  _qpValue = qpv;
  _width = w;
  _height = h;

  _area = w * h;

  _null = false;
}

PageSizeInfo::PageSizeInfo()
{
  _name = QString::null;
  _qpValue = QPrinter::Custom;
  _width = 0;
  _height = 0;
  _area = 0;
  _null = true;
}

PageSizeInfo::~PageSizeInfo()
{
}

const QString & PageSizeInfo::name() const
{
  return _name;
}

int PageSizeInfo::qpValue() const
{
  return _qpValue;
}
int PageSizeInfo::width() const
{
  return _width;
}
int PageSizeInfo::height() const
{
  return _height;
}

int PageSizeInfo::area() const
{
  return _area;
}

bool PageSizeInfo::isNull() const
{
  return _null;
}
