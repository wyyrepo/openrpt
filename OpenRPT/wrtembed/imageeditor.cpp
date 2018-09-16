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

#include "imageeditor.h"

#include <QVariant>
#include <QImage>
#include <QPainter>
#include <QFile>
#include <QFileDialog>
#include <QValidator>

#include <quuencode.h>

ImageEditor::ImageEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(cbStatic, SIGNAL(toggled(bool)), this, SLOT(cbStatic_toggled(bool)));
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(btnLoad, SIGNAL(clicked()), this, SLOT(btnLoad_clicked()));

    leXPos->setValidator(new QDoubleValidator(0.0,100.0,3,leXPos));
    leYPos->setValidator(new QDoubleValidator(0.0,100.0,3,leYPos));
    
    leWidth->setValidator(new QDoubleValidator(0.01,100.0,3,leWidth));
    leHeight->setValidator(new QDoubleValidator(0.01,100.0,3,leHeight));
}

ImageEditor::~ImageEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void ImageEditor::languageChange()
{
    retranslateUi(this);
}

void ImageEditor::cbStatic_toggled( bool yes )
{
    gbDynamic->setEnabled(!yes);
   
    gbStatic->setEnabled(yes);
}

QString ImageEditor::getImageData()
{
    return uudata;
}

void ImageEditor::setImageData( QString dat )
{
    uudata = dat;
    QByteArray bytes = QUUDecode(dat);
    QImage i;
    i.loadFromData(bytes);
    frmPreview->setPixmap(QPixmap::fromImage(i));
}

QString ImageEditor::getMode()
{
    if(rbClip->isChecked()) return "clip";
    if(rbStretch->isChecked()) return "stretch";
    return "clip";
}

bool ImageEditor::isInline()
{
    return cbStatic->isChecked();
}

void ImageEditor::setMode( QString m)
{
    if(m == "clip") rbClip->setChecked(true);
    else if(m == "stretch") rbStretch->setChecked(true);
    else rbClip->setChecked(true);
}

void ImageEditor::setInline( bool yes )
{
    cbStatic->setChecked(yes);
}

void ImageEditor::btnLoad_clicked()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Choose a file"), QString::null, tr("Images(*.png *.jpg *.xpm)"));
    if(!fn.isEmpty()) {
	QFile file(fn);
	setImageData(QUUEncode(file));
    }
}

