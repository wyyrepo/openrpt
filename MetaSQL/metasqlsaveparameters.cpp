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

#include "metasqlsaveparameters.h"

#include <QMessageBox>
#include <QSqlError>

#include "parameter.h"
#include "metasql.h"
#include "xsqlquery.h"

#define DEBUG false

MetaSQLSaveParameters::MetaSQLSaveParameters(QWidget *parent,
                                             Qt::WindowFlags fl)
  : QDialog(parent, fl)
{
  setupUi(this);

  _schema->addItem("");
  _schema->addItem("public");
  XSqlQuery schemaq("SELECT pkghead_name"
                    "  FROM pkghead"
                    " ORDER BY pkghead_name;");
  schemaq.exec();
  while (schemaq.next())
    _schema->addItem(schemaq.value("pkghead_name").toString());

  connect(_schema, SIGNAL(currentIndexChanged(int)), this, SLOT(sGetGroups()));

  sGetGroups();
}

MetaSQLSaveParameters::~MetaSQLSaveParameters()
{
  // no need to delete child widgets, Qt does it all for us
}

void MetaSQLSaveParameters::languageChange()
{
    retranslateUi(this);
}

int MetaSQLSaveParameters::grade() const
{
  return _grade->value();
}

QString MetaSQLSaveParameters::group() const
{
  return _group->currentText();
}

QString MetaSQLSaveParameters::name() const
{
  return _name->text();
}

QString MetaSQLSaveParameters::notes() const
{
  return _notes->toPlainText();
}

QString MetaSQLSaveParameters::schema() const
{
  return _schema->currentText();
}

void MetaSQLSaveParameters::setGrade(int p)
{
  if (p < 0)
    _grade->setValue(-1);
  else if (p >= 99)
    _grade->setValue(99);
  else
    _grade->setValue(p);
}

void MetaSQLSaveParameters::setGroup(const QString p)
{
  int idx = _group->findText(p, Qt::MatchFixedString); // case-insensitive
  if (idx < 0)
  {
    _group->addItem(p);
    idx = _group->findText(p, Qt::MatchFixedString); // case-insensitive
  }
    
  _group->setCurrentIndex(idx);
}

void MetaSQLSaveParameters::setName(const QString p)
{
  _name->setText(p);
}

void MetaSQLSaveParameters::setNotes(const QString p)
{
  _notes->setPlainText(p);
}

void MetaSQLSaveParameters::setSchema(const QString p)
{
  if (DEBUG)
    qDebug("MetaSQLSaveParameters::setSchema(%s) entered", qPrintable(p));
  int idx = _schema->findText(p, Qt::MatchFixedString); // case-insensitive
  if (idx < 0)
    idx = _schema->findText("public", Qt::MatchFixedString); // case-insensitive
    
  _schema->setCurrentIndex(idx);
  if (DEBUG)
    qDebug("MetaSQLSaveParameters::setSchema(%s) returning with %s (%d)",
           qPrintable(p), qPrintable(_schema->currentText()), idx);
}

void MetaSQLSaveParameters::sGetGroups()
{
  ParameterList params;
  if (_schema->currentText() == "public")
    params.append("public");
  else if (! _schema->currentText().isEmpty())
    params.append("schema", _schema->currentText());

  QString groups("SELECT DISTINCT metasql_group"
                 "<? if exists(\"public\") ?>"
                 "  FROM ONLY metasql"
                 "<? elseif exists(\"schema\") ?>"
                 "  FROM <? literal(\"schema\") ?>.pkgmetasql"
                 "<? else ?>"
                 "  FROM metasql"
                 "<? endif ?>"
                 " ORDER BY metasql_group;");
  MetaSQLQuery groupm(groups);
  XSqlQuery groupq = groupm.toQuery(params);
  groupq.exec();
  _group->clear();
  _group->addItem("");
  while (groupq.next())
    _group->addItem(groupq.value("metasql_group").toString());
  if (groupq.lastError().type() != QSqlError::NoError)
    QMessageBox::warning(this, tr("Database Errror"),
                         groupq.lastError().text());

  _group->setCurrentIndex(-1);
}
