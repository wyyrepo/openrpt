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

#include "mqlutil.h"

#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

#include "xsqlquery.h"

static QRegExp groupRE    = QRegExp("(^\\s*--\\s*GROUP:\\s*)(.*)",Qt::CaseInsensitive);
static QRegExp nameRE     = QRegExp("(^\\s*--\\s*NAME:\\s*)(.*)", Qt::CaseInsensitive);
static QRegExp notesRE    = QRegExp("(^\\s*--\\s*NOTES:\\s*)(.*)",Qt::CaseInsensitive);
static QRegExp dashdashRE = QRegExp("(^\\s*--\\s{0,8})(.*)");

bool MQLUtil::rewriteMetadata(QString &query,      const QString &group,
                              const QString &name, const QString &notes)
{
  QStringList lines = query.split("\n");
  QString groupline;
  QString nameline;

  for (int i = 0; i < lines.size(); i++)
  {
    if (groupRE.indexIn(lines.at(i)) >= 0)
    {
      if (groupRE.cap(2).trimmed().compare(group.trimmed(), Qt::CaseInsensitive) == 0)
        groupline = lines.takeAt(i--);
      else
        lines.removeAt(i--);
    }
    else if (nameRE.indexIn(lines.at(i)) >= 0)
    {
      if (nameRE.cap(2).trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0)
        nameline = lines.takeAt(i--);
      else
        lines.removeAt(i--);
    }
    else if (notesRE.indexIn(lines.at(i)) >= 0)
    {
      lines.removeAt(i--);
      while (dashdashRE.indexIn(lines.at(++i)) >= 0)
        lines.removeAt(i--);
    }
  }

  QStringList noteslines = notes.split("\n");
  if (noteslines.size() > 0)
  {
    if (! noteslines.at(0).contains(notesRE))
      noteslines.replace(0, "-- Notes: " + noteslines.at(0));

    for (int i = 1; i < noteslines.size(); i++)
      if (dashdashRE.indexIn(noteslines.at(i)) < 0)
        noteslines.replace(i, "--        " + noteslines.at(i));
  }

  if (groupline.isEmpty())
    lines.insert(0, "-- Group: " + group);
  else
    lines.insert(0, groupline);

  if (nameline.isEmpty())
    lines.insert(1, "-- Name:  " + name);
  else
    lines.insert(1, nameline);

  for (int i = 0; i < noteslines.size(); i++)
    lines.insert(i + 2, noteslines.at(i));

  query = lines.join("\n");

  return true;
}

bool MQLUtil::extractMetadata(const QString &query,
                              QString &group, QString &name, QString &notes)
{
  group = QString::null;
  name  = QString::null;
  notes = QString::null;

  if(! query.isEmpty())
  {
    QStringList lines  = query.split("\n");

    for (int i = 0; i < lines.size(); i++)
    {
      if (groupRE.indexIn(lines.at(i)) >= 0)
        group = groupRE.cap(2).trimmed();
      else if (nameRE.indexIn(lines.at(i)) >= 0)
        name = nameRE.cap(2).trimmed();
      else if (notesRE.indexIn(lines.at(i)) >= 0)
      {
        notes = notesRE.cap(2).trimmed();
        while (dashdashRE.indexIn(lines.at(++i)) >= 0)
          notes += "\n" + dashdashRE.cap(2);
      }
    }
  }
  return !(group.isEmpty() || name.isEmpty());
}

QString MQLUtil::mqlLoad(const QString &name, QString &errmsg, bool *valid)
{
  if (valid)
    *valid = false;

  QString fsrc = QString::null;
  QFile fin(name);
  if (fin.open(QIODevice::ReadOnly))
  {
    QTextStream sin(&fin);
    fsrc = sin.readAll();
    if (valid)
      *valid = true;
  }
  else
  {
    errmsg = QObject::tr("Could not open file '%1': %2")
                    .arg(name, fin.errorString());
    qWarning("%s", qPrintable(errmsg));
  }

  return fsrc;
}

QString MQLUtil::mqlLoad(const QString &group, const QString &name, QString &errmsg, bool *valid)
{
  if (valid)
    *valid = false;

  QString fsrc = QString::null;
  errmsg = QString::null;

  XSqlQuery qry;
  QString sql;
  sql = QString("SELECT metasql_query "
                "  FROM metasql "
                 "WHERE ((metasql_group='%1') "
                "    AND (metasql_name='%2'))"
                " ORDER BY metasql_grade DESC"
                " LIMIT 1;").arg(group).arg(name);
  qry.exec(sql);
  if (qry.first())
  {
    fsrc = qry.value("metasql_query").toString();
    if (valid)
      *valid = true;
  }
  else
  {
    errmsg = QString("Could not open query '%1': %2").arg(group).arg(name);
    qWarning("%s", qPrintable(errmsg));
  }

  return fsrc;
}

// backwards compatibility for xtuple application sources /////////////////////

static QString _lastError;

QString mqlLastError() { return _lastError; }
void    mqlClearLastError() { _lastError = QString::null; }

MetaSQLQuery mqlLoad(const QString &name, bool *valid)
{
  return MetaSQLQuery(MQLUtil::mqlLoad(name, _lastError, valid));
}

MetaSQLQuery mqlLoad(const QString &group, const QString &name, bool *valid)
{
  return MetaSQLQuery(MQLUtil::mqlLoad(group, name, _lastError, valid));
}
