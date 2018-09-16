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

#include "reportproperties.h"

#include <QVariant>
#include <QFontDialog>
#include <QFileDialog>
#include <QValidator>
#include <QPainter>
#include <QImage>
#include <QPixmap>

#include <querysource.h>
#include <quuencode.h>

ReportProperties::ReportProperties(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_btnAccept, SIGNAL(clicked()), this, SLOT(accept()));
    connect(_btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_slWmOpacity, SIGNAL(valueChanged(int)), this, SLOT(setWatermarkOpacity(int)));
    connect(_cbWmStatic, SIGNAL(toggled(bool)), _gbWmDynamic, SLOT(setDisabled(bool)));
    connect(_cbWmStatic, SIGNAL(toggled(bool)), _leWmText, SLOT(setEnabled(bool)));
    connect(_cbWmUseBest, SIGNAL(toggled(bool)), _gbWmFont, SLOT(setDisabled(bool)));
    connect(_btnWmChangeFont, SIGNAL(clicked()), this, SLOT(changeWmFont()));
    connect(_cbBgEnable, SIGNAL(toggled(bool)), _tabBg, SLOT(setEnabled(bool)));
    connect(_rbBgStatic, SIGNAL(toggled(bool)), _gbBgDynamic, SLOT(setDisabled(bool)));
    connect(_rbBgStatic, SIGNAL(toggled(bool)), _gbStatic, SLOT(setEnabled(bool)));
    connect(_slBgOpacity, SIGNAL(valueChanged(int)), this, SLOT(setBgOpacity(int)));
    connect(_btnLoad, SIGNAL(clicked()), this, SLOT(sLoadBgImage()));

    _tabBg->setEnabled(false);
    _uudata = QString::null;
    
    QDoubleValidator * dblval = new QDoubleValidator(0.0, 100.0, 2, this);
    _leBgX->setValidator(dblval);
    _leBgY->setValidator(dblval);
    _leBgWidth->setValidator(dblval);
    _leBgHeight->setValidator(dblval);
}

ReportProperties::~ReportProperties()
{
    // no need to delete child widgets, Qt does it all for us
}

void ReportProperties::languageChange()
{
    retranslateUi(this);
}

//
// Info
//
QString ReportProperties::getReportDescription()
{
    return _leDescription->toPlainText();
}

QString ReportProperties::getReportName()
{
    return _leName->text();
}

QString ReportProperties::getReportTitle()
{
    return _leTitle->text();
}

void ReportProperties::setReportDescription(QString str)
{
    _leDescription->setText(str);
}

void ReportProperties::setReportName(QString str)
{
    _leName->setText(str);
}

void ReportProperties::setReportTitle(QString str)
{
    _leTitle->setText(str);
}

//
// Watermark
//
int ReportProperties::getWatermarkOpacity()
{
    return _slWmOpacity->value();
}

void ReportProperties::setWatermarkOpacity( int i )
{
    if(_slWmOpacity->value() != i) {
	_slWmOpacity->setValue(i);
    }
    double d = i / 2.55;
    _lblWmOpacity->setText(QString("%1%").arg(QString::number((int)d)));
}

void ReportProperties::changeWmFont()
{
    bool valid = false;
    QFont fnt = QFontDialog::getFont(&valid, getWmFont(), this);
    if(valid) {
	setWmFont(fnt);
    }
}

void ReportProperties::setWmFont( QFont fnt )
{
    _leWmFont->setFont(fnt);
    _leWmFont->setText(fnt.family());
}


QFont ReportProperties::getWmFont()
{
    return _leWmFont->font();
}

void ReportProperties::SetUseBestWMFont( bool b )
{
    _cbWmUseBest->setChecked(b);
}

bool ReportProperties::getUseBestWmFont()
{
    return _cbWmUseBest->isChecked();
}

bool ReportProperties::isWmTextStatic()
{
    return _cbWmStatic->isChecked();
}

void ReportProperties::setWmTextStatic( bool b )
{
    _cbWmStatic->setChecked(b);
}

QString ReportProperties::getWmText()
{
    return _leWmText->text();
}

void ReportProperties::setWmText( QString str )
{
    _leWmText->setText(str);
}


void ReportProperties::setWmColumn( QString str )
{
    _leWmColumn->setText(str);
}


QString ReportProperties::getWmColumn()
{
    return _leWmColumn->text();
}


void ReportProperties::setWmQuery( QuerySourceList * qsl, QString query)
{
    _cbWmQuery->init(qsl,query);
}


QString ReportProperties::getWmQuery()
{
    return _cbWmQuery->currentQuery();
}

//
// Background
//
QString ReportProperties::getBgColumn()
{
    return _leBgColumn->text();
}

void ReportProperties::setBgColumn( QString str )
{
    _leBgColumn->setText(str);
}

QString ReportProperties::getBgQuery()
{
    return _cbBgQuery->currentQuery();
}

void ReportProperties::setBgQuery( QuerySourceList * qsl, QString query )
{
    _cbBgQuery->init(qsl,query);
}

int ReportProperties::getBgOpacity()
{
    return _slBgOpacity->value();
}

void ReportProperties::setBgOpacity( int i )
{
    if(_slBgOpacity->value() != i) {
	_slBgOpacity->setValue(i);
    }
    double d = i / 2.55;
    _lblBgOpacity->setText(QString("%1%").arg(QString::number((int)d)));
}


bool ReportProperties::isBgEnabled()
{
    return _cbBgEnable->isChecked();
}


void ReportProperties::setBgEnabled( bool b )
{
    _cbBgEnable->setChecked(b);
}


bool ReportProperties::isBgStatic()
{
    return _rbBgStatic->isChecked();
}


void ReportProperties::setBgStatic( bool b )
{
    if(b) {
	_rbBgStatic->setChecked(true);
    } else {
	_rbBgDynamic->setChecked(true);
    }
}


QString ReportProperties::getBgResizeMode()
{
    if(_rbBgClip->isChecked()) return "clip";
    if(_rbBgStretch->isChecked()) return "stretch";
    return "clip";
}


void ReportProperties::setBgResizeMode( QString m )
{
    if(m == "stretch") {
	_rbBgStretch->setChecked(true);
    } else /* if(m == "clip") */ {
	_rbBgClip->setChecked(true);
    }
    
}


QString ReportProperties::getBgImageData()
{
    return _uudata;
}


void ReportProperties::setBgImageData( QString dat )
{
    _uudata = dat;
    if(!_uudata.isEmpty()) {
        QByteArray bytes = QUUDecode(dat);
        QImage i;
        i.loadFromData(bytes);
        _pixmap->setPixmap(QPixmap::fromImage(i));
    } else {
        _pixmap->setPixmap(QPixmap());
    }
}

void ReportProperties::sLoadBgImage() {
    QString fn = QFileDialog::getOpenFileName(this, tr("Choose a file"), QString::null, tr("Images(*.png *.jpg *.xpm)"));
    if(!fn.isEmpty()) {
        QFile file(fn);
        setBgImageData(QUUEncode(file));
    }
}

void ReportProperties::setBgAlign( int f )
{
    if((f & Qt::AlignLeft) == Qt::AlignLeft) {
        _rbBgHAlignLeft->setChecked(true);
    } else if((f & Qt::AlignHCenter) == Qt::AlignHCenter) {
        _rbBgHAlignCenter->setChecked(true);
    } else if((f & Qt::AlignRight) == Qt::AlignRight) {
        _rbBgHAlignRight->setChecked(true);
    } else {
	_rbBgHAlignLeft->setChecked(true);
    }
    
    if((f & Qt::AlignTop) == Qt::AlignTop) {
        _rbBgVAlignTop->setChecked(true);
    } else if((f & Qt::AlignVCenter) == Qt::AlignVCenter) {
        _rbBgVAlignMiddle->setChecked(true);
    } else if((f & Qt::AlignBottom) == Qt::AlignBottom) {
        _rbBgVAlignBottom->setChecked(true);
    } else {
	_rbBgVAlignTop->setChecked(true);
    }
}

int ReportProperties::getBgAlign()
{
    int f = 0;
    if(_rbBgHAlignRight->isChecked()) {
	f = Qt::AlignRight;
    } else if(_rbBgHAlignCenter->isChecked()) {
	f = Qt::AlignHCenter;
    } else /*if(_rbBgHAlignLeft->isChecked())*/ {
	f = Qt::AlignLeft;
    }
    if(_rbBgVAlignBottom->isChecked()) {
	f |= Qt::AlignBottom;
    } else if(_rbBgVAlignMiddle->isChecked()) {
	f |= Qt::AlignVCenter;
    } else /*if(_rbBgVAlignMiddle->isChecked())*/ {
	f |= Qt::AlignTop;
    }
    return f;
}


int ReportProperties::getBgBoundsX()
{
    return (int)(_leBgX->text().toDouble() * 100.0);
}


int ReportProperties::getBgBoundsY()
{
    return (int)(_leBgY->text().toDouble() * 100.0);
}


int ReportProperties::getBgBoundsWidth()
{
    return (int)(_leBgWidth->text().toDouble() * 100.0);
}


int ReportProperties::getBgBoundsHeight()
{
    return (int)(_leBgHeight->text().toDouble() * 100.0);
}


void ReportProperties::setBgBoundsX( int i )
{
    _leBgX->setText(QString::number((double)i/100.0, 'g', 2));
}


void ReportProperties::setBgBoundsY( int i )
{
    _leBgY->setText(QString::number((double)i/100.0, 'g', 2));
}


void ReportProperties::setBgBoundsWidth( int i )
{
    _leBgWidth->setText(QString::number((double)i/100.0, 'g', 2));
}


void ReportProperties::setBgBoundsHeight( int i )
{
    _leBgHeight->setText(QString::number((double)i/100.0, 'g', 2));
}
