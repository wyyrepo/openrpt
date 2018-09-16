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

#include "parsexmlutils.h"
#include "xsqlquery.h"
#include "parameteredit.h"
#include "paramlistedit.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>

#include "parameterproperties.h"

ParameterEdit::ParameterEdit(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
  setupUi(this);

  // OK and Cancel are only appropriate if the widget is a stand-alone window
  if (parent)
  {
    _ok->hide();
    _cancel->hide();
    _buttonDiv->hide();
  }

  _list->hide(); // parameter lists come from document definitions, so are only visible after a call to setDocument()
  _list->setDisabled(true);
  _edit->setDisabled(true);

  connect(_new, SIGNAL(clicked()), this, SLOT(newItem()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(edit()));
  connect(_list, SIGNAL(clicked()), this, SLOT(editItemList()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(deleteItem()));
  connect(_table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(doubleClick(int, int)));
  connect(_table, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChange()));
}

ParameterEdit::~ParameterEdit()
{
    // no need to delete child widgets, Qt does it all for us
}

void ParameterEdit::languageChange()
{
    retranslateUi(this);
}

bool ParameterEdit::setDocument(const QDomDocument & doc)
{
  QDomElement root = doc.documentElement();
  if(root.tagName() != "report")
  {
    QMessageBox::critical(this, tr("Not a Valid Report"),
      tr("The report definition does not appear to be a valid report."
         "\n\nThe root node is not 'report'."));
    return false;
  }

  _list->show();	
  _new->hide();	
  _delete->hide();	

  for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
  {
    if(n.nodeName() == "parameter")
    {
      QDomElement elemSource = n.toElement();
      ORParameter param;

      param.name = elemSource.attribute("name");
      if(param.name.isEmpty())
        continue;
    
      param.type = elemSource.attribute("type");
      param.defaultValue  = elemSource.attribute("default");
      param.active = (elemSource.attribute("active") == "true");
      param.listtype = elemSource.attribute("listtype");
      
      QList<QPair<QString,QString> > pairs;
      if(param.listtype.isEmpty())
        param.description = elemSource.text();
      else
      {
        QDomNodeList section = elemSource.childNodes();
        for(int nodeCounter = 0; nodeCounter < section.count(); nodeCounter++)
        {
          QDomElement elemThis = section.item(nodeCounter).toElement();
          if(elemThis.tagName() == "description")
            param.description = elemThis.text();
          else if(elemThis.tagName() == "query")
            param.query = elemThis.text();
          else if(elemThis.tagName() == "item")
            param.values.append(qMakePair(elemThis.attribute("value"), elemThis.text()));
          else
            qDebug("While parsing parameter encountered an unknown element: %s",(const char*)elemThis.tagName().toLatin1().data());
        }
      }
      QVariant defaultVar;
      if(!param.defaultValue.isEmpty())
        defaultVar = QVariant(param.defaultValue);
      if("integer" == param.type)
        defaultVar = defaultVar.toInt();
      else if("double" == param.type)
        defaultVar = defaultVar.toDouble();
      else if("bool" == param.type)
        defaultVar = QVariant(defaultVar.toBool());
      else
        defaultVar = defaultVar.toString();
      updateParam(param.name, defaultVar, param.active);
      QList<QPair<QString, QString> > list;
      if("static" == param.listtype)
        list = param.values;
      else if("dynamic" == param.listtype && !param.query.isEmpty())
      {
        QSqlQuery qry(param.query);
        while(qry.next())
          list.append(qMakePair(qry.value(0).toString(), qry.value(1).toString()));
      }
      if(!list.isEmpty())
        _lists.insert(param.name, list);
    }
  }

  if(_lists.isEmpty())
    return false; // no defined parameters
  else 
    return true;
}

void ParameterEdit::updateParam(const QString & name, const QVariant & value, bool active)
{
  _params[name] = value;
  int r;
  for(r = 0; r < _table->rowCount(); r++) {
    if(_table->item(r, 1)->text() == name)
    {
      _table->item(r, 0)->setCheckState((active ? Qt::Checked : Qt::Unchecked));
      _table->item(r, 2)->setText(value.typeName());
      _table->item(r, 3)->setText(value.toString());
      return;
    }
  }

  // If we didn't find an existing parameter by the name specified add one
  r = _table->rowCount();
  _table->setRowCount(r+1);
  QTableWidgetItem * ctItem = 0;
  ctItem = new QTableWidgetItem();
  ctItem->setFlags(Qt::ItemIsUserCheckable);
  ctItem->setCheckState((active ? Qt::Checked : Qt::Unchecked));
  _table->setItem(r, 0, ctItem);
  _table->setItem(r, 1, new QTableWidgetItem(name));
  _table->setItem(r, 2, new QTableWidgetItem(value.typeName()));
  _table->setItem(r, 3, new QTableWidgetItem(value.toString()));
}

void ParameterEdit::newItem()
{
  ParameterProperties pedit(this);
  if (pedit.exec() == QDialog::Accepted)
  {
    QString name = pedit.name();

    if(_params.contains(name))
      QMessageBox::warning(this, tr("Name already exists"),
                           tr("<p>The name for the parameter you specified "
                              "already exists in the list."));

    _params[name] = pedit.value();

    int r = _table->rowCount();
    _table->setRowCount(r+1);
    QTableWidgetItem * ctItem = 0;
    ctItem = new QTableWidgetItem();
    ctItem->setFlags(Qt::ItemIsUserCheckable);
    ctItem->setCheckState((pedit.active() ? Qt::Checked : Qt::Unchecked));
    _table->setItem(r, 0, ctItem);
    _table->setItem(r, 1, new QTableWidgetItem(name));
    _table->setItem(r, 2, new QTableWidgetItem(pedit.value().typeName()));
    _table->setItem(r, 3, new QTableWidgetItem(pedit.value().toString()));
  }
}

void ParameterEdit::edit()
{
    if(_table->currentRow() != -1) 
    {
	editItem(_table->currentRow());
    }
}

void ParameterEdit::editItem(int row)
{
  QString name = _table->item(row, 1)->text();

  ParameterProperties pedit(this);
  pedit.setName(name);
  pedit.setValue(_params[name]);
  pedit.setActive(_table->item(row, 0)->checkState() == Qt::Checked);

  if (pedit.exec() == QDialog::Accepted)
  {
    _params[name] = pedit.value();
    _table->item(row, 0)->setCheckState((pedit.active() ? Qt::Checked
                                                        : Qt::Unchecked));
    _table->item(row, 1)->setText(pedit.name());
    _table->item(row, 2)->setText(pedit.value().typeName());
    _table->item(row, 3)->setText(pedit.value().toString());
  }
}

void ParameterEdit::doubleClick(int row, int /*col*/)
{
    editItem(row);
}

void ParameterEdit::selectionChange()
{
    if(_table->currentRow() != -1)
    {
        _edit->setDisabled(false);

        int r = _table->currentRow();
        QString name = _table->item(r, 1)->text();
        QList<QPair<QString, QString> > list = _lists[name];
        if(list.size()>0) 
             _list->setDisabled(false); // the selected parameter is associated with a list
        else _list->setDisabled(true); 
    }
    else 
    {
        _list->setDisabled(true);
        _edit->setDisabled(true);
    }
}

void ParameterEdit::editItemList()
{
    if(_table->currentRow() == -1)
        return;

    int r = _table->currentRow();
    QString name = _table->item(r, 1)->text();
    QVariant var = _params[name];
    QList<QPair<QString, QString> > list = _lists[name];

    ParamListEdit newdlg(this);
    for(QList<QPair<QString, QString> >::iterator it = list.begin();
        it != list.end(); it++ )
    {
        QListWidgetItem * item = new QListWidgetItem((*it).second, newdlg._list);
        if((*it).first == var.toString())
            newdlg._list->setCurrentItem(item);
    }

    if(newdlg.exec() == QDialog::Accepted)
    {
        QVariant tmp(list[newdlg._list->currentRow()].first);
        if(tmp.convert(var.type()))
        {
            _params[name] = tmp;
            _table->item(r, 3)->setText(tmp.toString());
        }
    }
}


void ParameterEdit::deleteItem()
{
    if(_table->currentRow() != -1) {
        QString name = _table->item(_table->currentRow(), 1)->text();
        _params.remove(name);
        _table->removeRow(_table->currentRow());
    }
}


ParameterList ParameterEdit::getParameterList()
{
    ParameterList plist;

    QString name;
    QVariant value;
    for(int r = 0; r < _table->rowCount(); r++) {
        if(_table->item(r, 0)->checkState() == Qt::Checked) {
            name = _table->item(r, 1)->text();
            value = _params[name];
            plist.append(name, value);
        }
    }

    return plist;
}

void ParameterEdit::clear()
{
  while (_table->rowCount() > 0)
    _table->removeRow(0);
}

QDialog *ParameterEdit::ParameterEditDialog(ParameterEdit *p, QWidget *parent, Qt::WindowFlags f)
{
  QDialog *dlg = new QDialog(parent, f);
  QGridLayout *lyt = new QGridLayout(dlg);
  dlg->setLayout(lyt);
  lyt->addWidget(p);

  p->_ok->show();
  p->_cancel->show();
  p->_buttonDiv->show();

  connect(p->_cancel, SIGNAL(clicked()), dlg, SLOT(reject()));
  connect(p->_ok,     SIGNAL(clicked()), dlg, SLOT(accept()));

  return dlg;
}
