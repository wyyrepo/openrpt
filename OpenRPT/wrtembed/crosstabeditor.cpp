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

#include "crosstabeditor.h"

#include <QVariant>
#include <QFontDialog>
#include <QValidator>

#include <builtinformatfunctions.h>

/*
 *  Constructs a CrossTabEditor as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
CrossTabEditor::CrossTabEditor(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(btnFont, SIGNAL(clicked()), this, SLOT(btnFont_clicked()));
}

CrossTabEditor::~CrossTabEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void CrossTabEditor::languageChange()
{
    retranslateUi(this);
}

void CrossTabEditor::btnFont_clicked()
{
    // pop up a font dialog
    bool ok;
    QFont font = QFontDialog::getFont(&ok, _font, this);
    if(ok) {
        _font = font;
    }
}

void CrossTabEditor::on_valueQueryGroupBox_toggled(bool)
{
}
