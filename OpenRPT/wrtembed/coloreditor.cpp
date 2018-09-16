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

#include "coloreditor.h"

#include <QVariant>
#include <QColorDialog>
#include <QValidator>

ColorEditor::ColorEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_btnAccept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(_btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_btnColor, SIGNAL(clicked()), this, SLOT(_btnColor_clicked()));

    QIntValidator * intv = new QIntValidator(0, 255, this);
    _editRed->setValidator(intv);
    _editRed->setText(tr("0"));
    _editGreen->setValidator(intv);
    _editGreen->setText(tr("0"));
    _editBlue->setValidator(intv);
    _editBlue->setText(tr("0"));
}

ColorEditor::~ColorEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void ColorEditor::languageChange()
{
    retranslateUi(this);
}

void ColorEditor::_btnColor_clicked()
{
    QColor col = QColorDialog::getColor(getColor(), this);
    if(col.isValid()) {
	setColor(col);
    }
}

void ColorEditor::setColorName( QString name )
{
    _editName->setText(name);
}

void ColorEditor::setColor( const QColor & col )
{
    _editRed->setText(QString::number(col.red()));
    _editGreen->setText(QString::number(col.green()));
    _editBlue->setText(QString::number(col.blue()));
}

QColor ColorEditor::getColor()
{
    return QColor(_editRed->text().toInt(),
	            _editGreen->text().toInt(),
	            _editBlue->text().toInt());
}

QString ColorEditor::getColorName()
{
    return _editName->text();
}
