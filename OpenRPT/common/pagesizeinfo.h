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

#ifndef __PAGESIZEINFO_H__
#define __PAGESIZEINFO_H__

#include <qstring.h>
#include <qstringlist.h>

class PageSizeInfo
{
  public:
    PageSizeInfo(const QString&, int, int, int);
    PageSizeInfo();
    virtual ~PageSizeInfo();

    const QString & name() const;
    int qpValue() const;
    int width() const;
    int height() const;
    int area() const;

    bool isNull() const;

    static const PageSizeInfo & getByName(const QString &);
    static const PageSizeInfo & getByValue(int);
    static const PageSizeInfo & findNearest(int, int);
    static QStringList getPageNames();

  protected:
    QString _name;
    int _qpValue;
    int _width;
    int _height;

    int _area;
    bool _null;
};

#endif
