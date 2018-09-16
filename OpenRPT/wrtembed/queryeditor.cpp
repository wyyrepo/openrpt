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

#include "queryeditor.h"
#include <xsqlquery.h>

#include <QVariant>
#include <QKeyEvent>
#include <QMessageBox>

QueryEditor::QueryEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_metasql, SIGNAL(toggled(bool)), this, SLOT(mqlToggled(bool)));
  connect(_mqlGroup, SIGNAL(editTextChanged(QString)), this, SLOT(groupEditTextChanged(QString)));
  connect(_mqlName, SIGNAL(editTextChanged(QString)), this, SLOT(nameEditTextChanged(QString)));

  _mqlGroup->clear();
  _mqlGroup->addItem("");
  XSqlQuery xqry;
  if(xqry.exec("SELECT distinct metasql_group FROM metasql ORDER BY metasql_group;"))
  {
    while(xqry.next())
      _mqlGroup->addItem(xqry.value(0).toString());
  }
}

QueryEditor::~QueryEditor()
{
  // no need to delete child widgets, Qt does it all for us
}

void QueryEditor::languageChange()
{
  retranslateUi(this);
}

void QueryEditor::keyPressEvent( QKeyEvent * e)
{
  if(Qt::Key_Escape == e->key())
  {
    e->accept();
  }
}

void QueryEditor::mqlToggled(bool b)
{
  if(b && isVisible())
  {
    if(QMessageBox::question(this, tr("Change may be lost"),
                             tr("<p>Any changes you have made to the sql "
                                "query may be lost by turning this option on. "
                                "Are you sure you want to continue?"),
       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
    {
      _metasql->setChecked(false);
    }
  }
}

void QueryEditor::groupEditTextChanged(const QString & t)
{
  XSqlQuery xqry;
  xqry.prepare("SELECT DISTINCT metasql_name FROM metasql WHERE metasql_group=:group ORDER BY metasql_name;");
  xqry.bindValue(":group", t);
  if(xqry.exec())
  {
    _mqlName->clear();
    while(xqry.next())
      _mqlName->addItem(xqry.value(0).toString());
  }

}

void QueryEditor::nameEditTextChanged(const QString & t)
{
  if(_metasql->isChecked())
  {
    XSqlQuery xqry;
    xqry.prepare("SELECT metasql_query"
                 "  FROM metasql"
                 " WHERE ((metasql_group=:group)"
                 "    AND (metasql_name=:name))"
                 " ORDER BY metasql_grade DESC"
                 " LIMIT 1;");
    xqry.bindValue(":group", _mqlGroup->currentText());
    xqry.bindValue(":name", t);
    xqry.exec();
    xqry.first();
    tbQuery->setText(xqry.value(0).toString());
  }
}
