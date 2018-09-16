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

#include "texteditor.h"

#include <QVariant>
#include <QFontDialog>
#include <QValidator>

TextEditor::TextEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(btnFont, SIGNAL(clicked()), this, SLOT(btnFont_clicked()));
    connect(rbVAlignBottom, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(tbColumn, SIGNAL(textChanged(const QString&)), this, SLOT(tbText_textChanged(const QString&)));
    connect(rbHAlignNone, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbHAlignLeft, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbHAlignCenter, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbHAlignRight, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbVAlignNone, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbVAlignTop, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));
    connect(rbVAlignMiddle, SIGNAL(clicked()), this, SLOT(rbAlign_changed()));

    leXPos->setValidator(new QDoubleValidator(0.0,100.0,3,leXPos));
    leYPos->setValidator(new QDoubleValidator(0.0,100.0,3,leYPos));
    
    leWidth->setValidator(new QDoubleValidator(0.01,100.0,3,leWidth));
    leHeight->setValidator(new QDoubleValidator(0.01,100.0,3,leHeight));
}

TextEditor::~TextEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void TextEditor::languageChange()
{
    retranslateUi(this);
}

void TextEditor::rbAlign_changed()
{
    // ok one of the radio buttons was click.    
    // there are 8 radio buttons to check.
    // 4 in 2 groups: vertical and horizonatal group
    Qt::Alignment f = 0;
    if(rbHAlignLeft->isChecked()) f |= Qt::AlignLeft;
    if(rbHAlignCenter->isChecked()) f |= Qt::AlignHCenter;
    if(rbHAlignRight->isChecked()) f |= Qt::AlignRight;
    if(rbVAlignTop->isChecked()) f |= Qt::AlignTop;
    if(rbVAlignMiddle->isChecked()) f |= Qt::AlignVCenter;
    if(rbVAlignBottom->isChecked()) f |= Qt::AlignBottom;
    labelPreview->setAlignment(f);
}

void TextEditor::btnFont_clicked()
{
    // pop up a font dialog
    bool ok;
    QFont font = QFontDialog::getFont(&ok, labelPreview->font(), this);
    if(ok) {
        labelPreview->setFont(font);
    }
}

void TextEditor::tbText_textChanged( const QString & Str )
{
    // ok update the preview
    labelPreview->setText(Str + ":" + cbQuery->currentText());
}

void TextEditor::setLabelFlags( int f )
{
    // set the label flags
    //qDebug("TextEditor::setLabelFlags( 0x%X )",f);
    labelPreview->setAlignment((Qt::Alignment)f);
    if((f & Qt::AlignLeft) == Qt::AlignLeft) {
        //qDebug("HAlignLeft 0x%X", AlignLeft);
        rbHAlignLeft->setChecked(true);
    } else if((f & Qt::AlignHCenter) == Qt::AlignHCenter) {
        //qDebug("HAlignCenter 0x%X", AlignHCenter);
        rbHAlignCenter->setChecked(true);
    } else if((f & Qt::AlignRight) == Qt::AlignRight) {
        //qDebug("HAlignRight 0x%X", AlignRight);
        rbHAlignRight->setChecked(true);
    } else {
        //qDebug("HAlignNone");
        rbHAlignNone->setChecked(true);
    }
    if((f & Qt::AlignTop) == Qt::AlignTop) {
        //qDebug("VAlignTop 0x%X", AlignTop);
        rbVAlignTop->setChecked(true);
    } else if((f & Qt::AlignVCenter) == Qt::AlignVCenter) {
        //qDebug("VAlignCenter 0x%X", AlignVCenter);
        rbVAlignMiddle->setChecked(true);
    } else if((f & Qt::AlignBottom) == Qt::AlignBottom) {
        //qDebug("VAlignBottom 0x%X", AlignBottom);
        rbVAlignBottom->setChecked(true);
    } else {
        //qDebug("VAlignNone");
        rbVAlignNone->setChecked(true);
    }
}


void TextEditor::rbHAlignNone_clicked()
{

}

