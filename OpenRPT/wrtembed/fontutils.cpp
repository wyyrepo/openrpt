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

/*
*     This is a collection of various functions used to parse the
* different xml structures used in the Report definitions.  Most
* of these are structures that are common to several or the complex
* structures.
*/

#include <QFont>
#include "graphicsitems.h"
#include "fontutils.h"


QFont getItemFont(QGraphicsItem *gi)
{
    QFont font;
    font.setFamily("");

    if(gi->type() == ORGraphicsFieldItem::Type)
    {
        ORGraphicsFieldItem * item = static_cast<ORGraphicsFieldItem*>(gi);
        font = item->font();
    }
    else if(gi->type() == ORGraphicsTextItem::Type)
    {
        ORGraphicsTextItem * item = static_cast<ORGraphicsTextItem*>(gi);
        font = item->font();
    }
    else if(gi->type() == ORGraphicsLabelItem::Type)
    {
        ORGraphicsLabelItem * item = static_cast<ORGraphicsLabelItem*>(gi);
        font = item->font();
    }

    return font;
}


void setItemFont(QGraphicsItem *gi, const QFont &font)
{
    if(gi->type() == ORGraphicsFieldItem::Type)
    {
        ORGraphicsFieldItem * item = static_cast<ORGraphicsFieldItem*>(gi);
        item->setFont(font);
    }
    else if(gi->type() == ORGraphicsTextItem::Type)
    {
        ORGraphicsTextItem * item = static_cast<ORGraphicsTextItem*>(gi);
        item->setFont(font);
    }
    else if(gi->type() == ORGraphicsLabelItem::Type)
    {
        ORGraphicsLabelItem * item = static_cast<ORGraphicsLabelItem*>(gi);
        item->setFont(font);
    }
}


QString getItemFontFamily(QGraphicsItem *gi)
{
    return getItemFont(gi).family();
}

QString getItemFontSize(QGraphicsItem *gi)
{
    QFont font = getItemFont(gi);

    if(font.family().isEmpty()) 
        return "";

    return QString("%1").arg(font.pointSize());
}

bool getItemFontWeight(QGraphicsItem *gi)
{
    return getItemFont(gi).bold();
}

bool getItemFontStyle(QGraphicsItem *gi)
{
    return getItemFont(gi).italic();
}


void setItemFontFamily(QGraphicsItem *gi, const QString s)
{
    QFont font = getItemFont(gi);
    font.setFamily(s);
    setItemFont(gi, font);
}

void setItemFontSize(QGraphicsItem *gi, const QString s)
{
    int size = s.toInt();
    if(s<=0) 
        return;

    QFont font = getItemFont(gi);
    font.setPointSize(size);
    setItemFont(gi, font);
}

void setItemFontWeight(QGraphicsItem *gi, bool v)
{
    QFont font = getItemFont(gi);
    font.setBold(v);
    setItemFont(gi, font);
}

void setItemFontStyle(QGraphicsItem *gi, bool v)
{
    QFont font = getItemFont(gi);
    font.setItalic(v);
    setItemFont(gi, font);
}
