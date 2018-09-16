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

#include "builtinformatfunctions.h"

static const int __fieldTag = 0;
static const int __fieldName = 1;
static const int __fieldFunction = 2;

static const int __fieldCount = 3;

static const char* __builtinTable[][__fieldCount] = {
//{"tag",        "Name",           "sqlFunctionName"}
  {"qty",        "Quantity",       "formatQty"},
  {"qtyper",     "Quantity Per",   "formatQtyPer"},

  {"extprice",   "Ext. Price",     "formatExtPrice"},
  {"purchprice", "Purchase Price", "formatPurchPrice"},
  {"salesprice", "Sales Price",    "formatSalesPrice"},
  {"price",      "Price",          "formatPrice"},
  {"cost",       "Cost",           "formatCost"},
  {"money",      "Money",          "formatMoney"},

  {"percent",    "Percent",        "formatPrcnt"},
  {"weight",     "Weight",         "formatWeight"},
  {"ratio",      "Ratio",          "formatRatio"},
  {"uomratio",   "UOM Ratio",      "formatUOMRatio"},
  {"scrap",      "Scrap",          "formatScrap"},

  {0,            0,                0} // DO NOT REMOVE -- null record
};



QStringList getTagList()
{
  QStringList tags;

  for (int i = 0; __builtinTable[i][__fieldTag] != 0; i++)
    tags.append(__builtinTable[i][__fieldTag]);

  return tags;
}

QStringList getNameList()
{
  QStringList names;

  for (int i = 0; __builtinTable[i][__fieldTag] != 0; i++)
    names.append(__builtinTable[i][__fieldName]);

  return names;
}


QString getNameFromTag(const QString & str)
{
  for (int i = 0; __builtinTable[i][__fieldTag] != 0; i++)
    if (__builtinTable[i][__fieldTag] == str)
      return __builtinTable[i][__fieldName];

  return QString::null;
}

QString getFunctionFromTag(const QString & str)
{
  for (int i = 0; __builtinTable[i][__fieldTag] != 0; i++)
    if (__builtinTable[i][__fieldTag] == str)
      return __builtinTable[i][__fieldFunction];

  return QString::null;
}

QString getTagFromName(const QString & str)
{
  for (int i = 0; __builtinTable[i][__fieldTag] != 0; i++)
    if (__builtinTable[i][__fieldName] == str)
      return __builtinTable[i][__fieldTag];

  return QString::null;
}

