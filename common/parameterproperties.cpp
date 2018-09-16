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

#include "parameterproperties.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QList>

static QString formatVariant(const QVariant & var)
{
  QString value = var.toString();
  QString type  = var.typeName();

  if(!type.isEmpty())
    value += " (" + type + ")";

  return value;
}

ParameterProperties::ParameterProperties(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  connect(_listDelete, SIGNAL(clicked()), this, SLOT(deleteItem()));
  connect(_listDown,   SIGNAL(clicked()), this, SLOT(moveItemDown()));
  connect(_listEdit,   SIGNAL(clicked()), this, SLOT(editItem()));
  connect(_listNew,    SIGNAL(clicked()), this, SLOT(newItem()));
  connect(_listUp,     SIGNAL(clicked()), this, SLOT(moveItemUp()));

  _int->setValidator(new QIntValidator(this));
  _double->setValidator(new QDoubleValidator(this));
}

ParameterProperties::~ParameterProperties()
{
  // no need to delete child widgets, Qt does it all for us
}

void ParameterProperties::languageChange()
{
  retranslateUi(this);
}

void ParameterProperties::newItem()
{
  QListWidgetItem *lwi = 0;
  QVariant var;
  ParameterProperties pedit(this);
  pedit._nameLit->hide();
  pedit._name->hide();
  if(pedit.exec() == QDialog::Accepted)
  {
    var = pedit.value();

    lwi = new QListWidgetItem(formatVariant(var), _list);
    lwi->setData(Qt::UserRole, var);
  }
}

void ParameterProperties::editItem()
{
  QVariant var;
  QListWidgetItem * item = _list->currentItem();
  if (item)
  {
    ParameterProperties pedit(this);
    pedit._nameLit->hide();
    pedit._name->hide();

    QVariant var = item->data(Qt::UserRole);
    QString typeName;

    switch(var.type())
    {
      case QVariant::Bool:
        typeName = tr("Bool");
        pedit._bool->setCurrentIndex(var.toBool() ? 1 : 0);
        break;
      case QVariant::Int:
        typeName = tr("Int");
        pedit._int->setText(QString::number(var.toInt()));
        break;
      case QVariant::Double:
        typeName = tr("Double");
        pedit._double->setText(QString::number(var.toDouble()));
        break;
      case QVariant::String:
        typeName = tr("String");
        pedit._string->setText(var.toString());
        break;
      case QVariant::List:
        typeName = tr("List");
        pedit.setList(var.toList());
        break;
      default:
        QMessageBox::warning(this, tr("Warning"),
                             tr("I do not know how to edit QVariant type %1.")
                               .arg(var.typeName()));
        return;
    };

    pedit._type->setCurrentIndex(_type->findText(typeName));

    if (pedit.exec() == QDialog::Accepted)
    {
      var = pedit.value();

      item->setData(Qt::UserRole, var);
      item->setText(formatVariant(var));
    }
  }
}

void ParameterProperties::deleteItem()
{
  QListWidgetItem * item = _list->currentItem();
  if(item)
    delete item;  
}

void ParameterProperties::moveItemUp()
{
  int row = _list->currentRow();
  if(row <= 1)
    return;
  QListWidgetItem * item = _list->takeItem(row);
  if (item)
    _list->insertItem(row-1, item);
}

void ParameterProperties::moveItemDown()
{
  int row = _list->currentRow();
  if(row >= _list->count())
    return;
  QListWidgetItem * item = _list->takeItem(row);
  if(item)
    _list->insertItem(row+1, item);
}

void ParameterProperties::setActive(bool p)
{
  _active->setChecked(p);
}

void ParameterProperties::setList( const QList<QVariant> & l )
{
  _list->clear();
  
  QList<QVariant>::const_iterator it;
  for(it = l.begin(); it != l.end(); ++it) {
      new QListWidgetItem(formatVariant(*it), _list);
  }
}

void ParameterProperties::setName(QString p)
{
  _name->setText(p);
}

void ParameterProperties::setType(QString p)
{
  int idx = _type->findText(p);
  if (idx >= 0)
    _type->setCurrentIndex(idx);
  else if (QVariant::nameToType(qPrintable(p)) != QVariant::Invalid)
    setType(QVariant::nameToType(qPrintable(p)));
  else
    QMessageBox::warning(this, tr("Warning"),
                         tr("I do not recognize type %1.") .arg(p));
}

void ParameterProperties::setType(QVariant::Type p)
{
  QString typeName;
  switch(p)
  {
    case QVariant::Bool:
      typeName = tr("Bool");
      break;
    case QVariant::Int:
      typeName = tr("Int");
      break;
    case QVariant::Double:
      typeName = tr("Double");
      break;
    case QVariant::String:
      typeName = tr("String");
      break;
    case QVariant::List:
      typeName = tr("List");
      break;
    default:
      QMessageBox::warning(this, tr("Warning"),
                           tr("I do not recognize type %1.")
                           .arg(QVariant::typeToName(p)));
  }
  int idx = _type->findText(typeName);
  if (idx >= 0)
    _type->setCurrentIndex(idx);
}

void ParameterProperties::setValue(QVariant p)
{
  setType(p.type());
  switch (p.type())
  {
    case QVariant::Bool:
      _bool->setCurrentIndex(p.toBool() ? 1 : 0);
      break;
    case QVariant::Int:
      _int->setText(QString::number(p.toInt()));
      break;
    case QVariant::Double:
      _double->setText(QString::number(p.toDouble()));
      break;
    case QVariant::String:
      _string->setText(p.toString());
    break;
    case QVariant::List:
      setList(p.toList());
      break;
    default:
      _string->setText(p.toString());
  }
}

bool ParameterProperties::active()
{
  return _active->isChecked();
}

QList<QVariant> ParameterProperties::list()
{
  QList<QVariant> varlist;
  QListWidgetItem * item = 0;
  for(int row = 0; row < _list->count(); row++)
  {
    item = _list->item(row);
    varlist.append(item->data(Qt::UserRole));
  }
  return varlist;
}

QString ParameterProperties::name()
{
  return _name->text();
}

QVariant::Type ParameterProperties::type()
{
  QVariant value;
  QString valueType = _type->currentText();

  if (valueType == tr("String"))
    value = QVariant(_string->text());
  else if (valueType == tr("Int"))
    value = QVariant(_int->text().toInt());
  else if (valueType == tr("Double"))
    value = QVariant(_double->text().toDouble());
  else if (valueType == tr("Bool"))
    value = QVariant(_bool->currentText().toLower() == "true");
  else if (valueType == tr("List"))
    value = QVariant(list());
  else
    QMessageBox::warning(this, tr("Unknown Type"),
                         QString(tr("I do not understand the type %1."))
                             .arg(valueType));
  return value.type();
}

QString ParameterProperties::typeName()
{
  return _type->currentText();
}

QVariant ParameterProperties::value()
{
  QVariant value;
  QString valueType = _type->currentText();

  if (valueType == tr("String"))
    value = QVariant(_string->text());
  else if (valueType == tr("Int"))
    value = QVariant(_int->text().toInt());
  else if (valueType == tr("Double"))
    value = QVariant(_double->text().toDouble());
  else if (valueType == tr("Bool"))
    value = QVariant(_bool->currentText().toLower() == "true");
  else if (valueType == tr("List"))
    value = QVariant(list());
  else
    QMessageBox::warning(this, tr("Unknown Type"),
                         QString(tr("I do not understand the type %1."))
                             .arg(valueType));
  return value;
}
