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

#include "labelsizeinfo.h"
#include <QMessageBox>
#include <QSqlError>
#include <xsqlquery.h>
#include <parameter.h>

static LabelSizeInfo __label;

static LabelSizeInfo __labels[] = {
//"LABEL_NAME","Paper Size","COLUMNS","ROWS","WIDTH","HEIGHT","STARTXOFFSET","STARTYOFFSET","HORIZONTALGAP","VERTICALGAP"
  LabelSizeInfo("Avery 5263", "Letter", 2, 5, 400, 200, 25, 50, 0, 0),
  LabelSizeInfo("Avery 5264", "Letter", 2, 3, 400, 333, 25, 75, 0, 0),
  LabelSizeInfo("Avery 8460", "Letter", 3, 10, 262, 100, 32, 50, 0, 0),
  LabelSizeInfo("CILS ALP1-9200-1", "Letter", 3, 7, 200, 100, 62, 62, 81, 50),
  LabelSizeInfo()               // Null Label Size
};

const LabelSizeInfo & LabelSizeInfo::getByNameNoDatabase(const QString & name)
{
  int i = 0;
  while (!__labels[i].isNull() && __labels[i]._name != name)
    i++;
  return __labels[i];
}

QStringList LabelSizeInfo::getLabelNamesNoDatabase()
{
  QStringList l;
  for (int i = 0; !__labels[i].isNull(); i++)
    l.append(__labels[i]._name);
  return l;
}

const LabelSizeInfo & LabelSizeInfo::getByName(const QString & name)
{
  XSqlQuery xqry;
  xqry.prepare("SELECT * FROM labeldef WHERE labeldef_name=:labelName");
  xqry.bindValue(":labelName", name);
  xqry.exec();

  if (xqry.first())
  {
    __label._null = false;
    __label._name    = xqry.value("labeldef_name").toString();
    __label._paper   = xqry.value("labeldef_papersize").toString();
    __label._columns = xqry.value("labeldef_columns").toInt();
    __label._rows    = xqry.value("labeldef_rows").toInt();
    __label._width   = xqry.value("labeldef_width").toInt();
    __label._height  = xqry.value("labeldef_height").toInt();
    __label._startx  = xqry.value("labeldef_start_offset_x").toInt();
    __label._starty  = xqry.value("labeldef_start_offset_y").toInt();
    __label._xgap    = xqry.value("labeldef_horizontal_gap").toInt();
    __label._ygap    = xqry.value("labeldef_vertical_gap").toInt();
  }
  else
  {
    __label = LabelSizeInfo::getByNameNoDatabase(name);
  }

  return __label;
}

QStringList LabelSizeInfo::getLabelNames()
{
  QStringList l;
  XSqlQuery xqry("SELECT labeldef_name FROM labeldef ORDER BY labeldef_name");

  xqry.exec();

  if (xqry.lastError().type() != QSqlError::NoError)
  {
    l = LabelSizeInfo::getLabelNamesNoDatabase();
  }
  else
  {
    while (xqry.next())
    {
      l.append(xqry.value("labeldef_name").toString());
    }
  }
  return l;
}

bool LabelSizeInfo::areLabelsEditable()
{
  bool editable = false;
  XSqlQuery xqry("SELECT labeldef_id FROM labeldef LIMIT 1");
  xqry.exec();

  if (xqry.lastError().type() == QSqlError::NoError)
  {
    editable = true;
  }
  return editable;
}

LabelSizeInfo::LabelSizeInfo(const QString & n, const QString & p, int c,
                             int r, int w, int h, int sx, int sy, int xg,
                             int yg)
{
  _name = n;
  _paper = p;

  _columns = c;
  _rows = r;

  _width = w;
  _height = h;

  _startx = sx;
  _starty = sy;

  _xgap = xg;
  _ygap = yg;

  _null = false;
}

LabelSizeInfo::LabelSizeInfo()
{
  _name = QString::null;
  _paper = QString::null;

  _columns = 0;
  _rows = 0;

  _width = 0;
  _height = 0;

  _startx = 0;
  _starty = 0;

  _xgap = 0;
  _ygap = 0;

  _null = true;
}

LabelSizeInfo::~LabelSizeInfo()
{
}

const QString & LabelSizeInfo::name() const
{
  return _name;
}

const QString & LabelSizeInfo::paper() const
{
  return _paper;
}

int LabelSizeInfo::columns() const
{
  return _columns;
}
int LabelSizeInfo::rows() const
{
  return _rows;
}

int LabelSizeInfo::width() const
{
  return _width;
}
int LabelSizeInfo::height() const
{
  return _height;
}

int LabelSizeInfo::startX() const
{
  return _startx;
}
int LabelSizeInfo::startY() const
{
  return _starty;
}

int LabelSizeInfo::xGap() const
{
  return _xgap;
}
int LabelSizeInfo::yGap() const
{
  return _ygap;
}

bool LabelSizeInfo::isNull() const
{
  return _null;
}
