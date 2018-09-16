/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2011 by OpenMFG, LLC
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

#include "patheditor.h"

#include <QColorDialog>
#include <QValidator>
#include <QVariant>

PathEditor::PathEditor(QWidget* parent, QPen initPen, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  _pen = initPen;
  _sbWidth->setValue(_pen.width());
  switch(_pen.style())
  {
    case Qt::DotLine:
      _rbDottedStyle->setChecked(true);
    break;
    case Qt::DashLine:
      _rbDashedStyle->setChecked(true);
    break;
    default:
      _rbSolidStyle->setChecked(true);
  }

  // signals and slots connections
  connect(_btnAccept, SIGNAL(clicked()), this, SLOT(accept()));
  connect(_btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_btnColor, SIGNAL(clicked()), this, SLOT(_btnColor_clicked()));

}

PathEditor::~PathEditor()
{
}

void PathEditor::languageChange()
{
  retranslateUi(this);
}

void PathEditor::_btnColor_clicked()
{
  QColor col = QColorDialog::getColor(_pen.color(), this);
  if(col.isValid()) {
    _pen.setColor(col);
  }
}

void PathEditor::accept()
{
  _pen.setWidth(_sbWidth->value());

  if(_rbDottedStyle->isChecked())
    _pen.setStyle(Qt::DotLine);
  else if (_rbDashedStyle->isChecked())
    _pen.setStyle(Qt::DashLine);
  else
    _pen.setStyle(Qt::SolidLine);

  QDialog::accept();
}

