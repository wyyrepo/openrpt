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

#include "grapheditor.h"
#include "colorlist.h"

#include <QVariant>
#include <QValidator>
#include <QFontDialog>
#include <QList>

GraphEditor::GraphEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_btnBaseFont, SIGNAL(clicked()), this, SLOT(_btnBaseFont_clicked()));
    connect(_cbTitleUseBase, SIGNAL(toggled(bool)), _gbTitleFont, SLOT(setDisabled(bool)));
    connect(_btnTitleFont, SIGNAL(clicked()), this, SLOT(_btnTitleFont_clicked()));
    connect(_cbDataTitleUseBase, SIGNAL(toggled(bool)), _gbDataTitleFont, SLOT(setDisabled(bool)));
    connect(_btnDataTitleFont, SIGNAL(clicked()), this, SLOT(_btnDataTitleFont_clicked()));
    connect(_cbDataUseBase, SIGNAL(toggled(bool)), _gbDataFont, SLOT(setDisabled(bool)));
    connect(_btnDataFont, SIGNAL(clicked()), this, SLOT(_btnDataFont_clicked()));
    connect(_cbValueTitleUseBase, SIGNAL(toggled(bool)), _gbValueTitleFont, SLOT(setDisabled(bool)));
    connect(_btnValueTitleFont, SIGNAL(clicked()), this, SLOT(_btnValueTitleFont_clicked()));
    connect(_cbValueUseBase, SIGNAL(toggled(bool)), _gbValueFont, SLOT(setDisabled(bool)));
    connect(_btnValueFont, SIGNAL(clicked()), this, SLOT(_btnValueFont_clicked()));
    connect(_btnNewSeries, SIGNAL(clicked()), this, SLOT(_btnNewSeries_clicked()));
    connect(_btnRemoveSeries, SIGNAL(clicked()), this, SLOT(_btnRemoveSeries_clicked()));
    connect(_btnEditColors, SIGNAL(clicked()), this, SLOT(_btnEditColors_clicked()));
    connect(_cbSeriesStyleBars, SIGNAL(toggled(bool)), this, SLOT(_cbSeriesStyleBars_toggled(bool)));
    connect(_cbSeriesStyleLines, SIGNAL(toggled(bool)), this, SLOT(_cbSeriesStyleLines_toggled(bool)));
    connect(_cbSeriesStylePoints, SIGNAL(toggled(bool)), this, SLOT(_cbSeriesStylePoints_toggled(bool)));
    connect(_leSeriesColumn, SIGNAL(textChanged(const QString&)), this, SLOT(_leSeriesColumn_textChanged(const QString&)));
    connect(_leSeriesName, SIGNAL(textChanged(const QString&)), this, SLOT(_leSeriesName_textChanged(const QString&)));
    connect(_cbSeries, SIGNAL(activated(const QString&)), this, SLOT(_cbSeries_activated(const QString&)));
    connect(_cbColors, SIGNAL(activated(const QString&)), this, SLOT(_cbColors_activated(const QString&)));

    _colorMap = 0;
    _seriesData = 0;

    leXPos->setValidator(new QDoubleValidator(0.0,100.0,3,leXPos));
    leYPos->setValidator(new QDoubleValidator(0.0,100.0,3,leYPos));
    
    leWidth->setValidator(new QDoubleValidator(0.01,100.0,3,leWidth));
    leHeight->setValidator(new QDoubleValidator(0.01,100.0,3,leHeight));
    
    _leMin->setValidator(new QDoubleValidator(-1000000000.0, 0.0, 2, _leMin));
    _leMax->setValidator(new QDoubleValidator(1.0, 1000000000.0, 2, _leMax));

    _gbSeriesProperties->setEnabled(false);
}

GraphEditor::~GraphEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void GraphEditor::languageChange()
{
    retranslateUi(this);
}

void GraphEditor::_btnBaseFont_clicked()
{
    bool valid;
   QFont fnt = QFontDialog::getFont(&valid, getBaseFont(), this);
    if(valid) {
	setBaseFont(fnt);
    }
}

void GraphEditor::setBaseFont( QFont fnt)
{
    _leBaseFont->setFont(fnt);
    _leBaseFont->setText(fnt.toString());
}

QFont GraphEditor::getBaseFont()
{
    return _leBaseFont->font();
}

void GraphEditor::_btnTitleFont_clicked()
{
    bool valid;
    QFont fnt = QFontDialog::getFont(&valid, getTitleFont(), this);
    if(valid) {
	setTitleFont(fnt);
    }
}

QFont GraphEditor::getTitleFont()
{
    return _leTitleFont->font();
}

void GraphEditor::setTitleFont( QFont fnt)
{
    _leTitleFont->setFont(fnt);
    _leTitleFont->setText(fnt.toString());
}

bool GraphEditor::getUseTitleFont()
{
    return !_cbTitleUseBase->isChecked();
}

void GraphEditor::setUseTitleFont( bool yes )
{
    _cbTitleUseBase->setChecked( !yes );
}

QString GraphEditor::getTitle()
{
    return _leTitle->text();
}

void GraphEditor::setTitle( QString str )
{
    _leTitle->setText(str);
}

QString GraphEditor::getDataTitle()
{
    return _leDataTitle->text();
}

void GraphEditor::setDataTitle( QString str )
{
    _leDataTitle->setText(str);
}

bool GraphEditor::getUseDataTitleFont()
{
    return !_cbDataTitleUseBase->isChecked();
}

void GraphEditor::setUseDataTitleFont( bool yes )
{
    _cbDataTitleUseBase->setChecked(!yes);
}

QFont GraphEditor::getDataTitleFont()
{
    return _leDataTitleFont->font();
}

void GraphEditor::setDataTitleFont( QFont fnt )
{
    _leDataTitleFont->setFont(fnt);
    _leDataTitleFont->setText(fnt.toString());
}

void GraphEditor::_btnDataTitleFont_clicked()
{
    bool valid;
    QFont fnt = QFontDialog::getFont(&valid, getDataTitleFont(), this);
    if(valid) {
	setDataTitleFont(fnt);
    }
}


QString GraphEditor::getDataColumn()
{
    return _leData->text();
}

void GraphEditor::setDataColumn( QString str)
{
    _leData->setText(str);
}

bool GraphEditor::getUseDataFont()
{
    return !_cbDataUseBase->isChecked();
}

void GraphEditor::setUseDataFont( bool yes )
{
    _cbDataUseBase->setChecked(!yes);
}

void GraphEditor::_btnDataFont_clicked()
{
    bool valid;
    QFont fnt = QFontDialog::getFont(&valid, getDataFont(), this);
    if(valid) {
	setDataFont(fnt);
    }
}

QFont GraphEditor::getDataFont()
{
    return _leDataFont->font();
}

void GraphEditor::setDataFont( QFont fnt )
{
    _leDataFont->setFont(fnt);
    _leDataFont->setText(fnt.toString());
}

QString GraphEditor::getValueTitle()
{
    return _leValueTitle->text();
}

void GraphEditor::setValueTitle( QString str )
{
    _leValueTitle->setText(str);
}

bool GraphEditor::getUseValueTitleFont()
{
    return !_cbValueTitleUseBase->isChecked();
}

void GraphEditor::setUseValueTitleFont( bool yes )
{
    _cbValueTitleUseBase->setChecked(!yes);
}

void GraphEditor::_btnValueTitleFont_clicked()
{
    bool valid;
    QFont fnt = QFontDialog::getFont(&valid, getValueTitleFont(), this);
    if(valid) {
	setValueTitleFont(fnt);
    }
}

QFont GraphEditor::getValueTitleFont()
{
    return _leValueTitleFont->font();
}

void GraphEditor::setValueTitleFont( QFont fnt )
{
    _leValueTitleFont->setFont(fnt);
    _leValueTitleFont->setText(fnt.toString());
}

void GraphEditor::_btnValueFont_clicked()
{
    bool valid;
    QFont fnt = QFontDialog::getFont(&valid, getValueFont(), this);
    if(valid) {
	setValueFont(fnt);
    }
}

QFont GraphEditor::getValueFont()
{
    return _leValueFont->font();
}

void GraphEditor::setValueFont( QFont fnt )
{
    _leValueFont->setFont(fnt);
    _leValueFont->setText(fnt.toString());
}

bool GraphEditor::getUseValueFont()
{
    return !_cbValueUseBase->isChecked();
}

void GraphEditor::setUseValueFont( bool yes )
{
    _cbValueUseBase->setChecked(!yes);
}

int GraphEditor::getHPadding()
{
    return _hPadSpinbox->value();
}

void GraphEditor::setHPadding( int hPadding )
{
    _hPadSpinbox->setValue(hPadding);
}

int GraphEditor::getVPadding()
{
    return _vPadSpinbox->value();
}

void GraphEditor::setVPadding( int vPadding )
{
    _vPadSpinbox->setValue(vPadding);
}

bool GraphEditor::getAutoMinMax()
{
    return _cbAutoMinMax->isChecked();
}

void GraphEditor::setAutoMinMax( bool yes )
{
    _cbAutoMinMax->setChecked(yes);
}

double GraphEditor::getMinValue()
{
    return _leMin->text().toDouble();
}

double GraphEditor::getMaxValue()
{
    return _leMax->text().toDouble();
}

void GraphEditor::setMaxValue( double val)
{
    _leMax->setText(QString::number(val));
}

void GraphEditor::setMinValue( double val)
{
    _leMin->setText(QString::number(val));
}


void GraphEditor::_btnNewSeries_clicked()
{
    QString sname = "New Series";
    int counter = 0;
    
    bool exitLoop = false;
    while(!exitLoop) {
        exitLoop = true;
        for(int i = 0; i < _seriesList.count(); i++) {
	        if(_seriesList.at(i)->name == sname) {
	            counter++;
	            sname = QString().sprintf("New Series %d", counter);
	            exitLoop = false;
                break;
	        }
        }
    }
    
    ORSeriesData * sd = new ORSeriesData();
    sd->name = sname;
    sd->color = QString::null;
    sd->column = QString::null;
    sd->style.bar = true;
    sd->style.line = false;
    sd->style.point = false;
    
    _seriesList.append(sd);
    _cbSeries->addItem(sname);
    _cbSeries->setCurrentIndex(_cbSeries->count() - 1);
    _cbSeries_activated(sname);
}

void GraphEditor::_btnRemoveSeries_clicked()
{
    int current_item = _cbSeries->currentIndex();
    if(current_item != -1) {
        QString sname = _cbSeries->itemText(current_item);
        for(int i = 0; i < _seriesList.count(); i++) {
            if(_seriesList.at(i)->name == sname) {
                _seriesList.removeAt(i);
                break;
            }
        }
        _cbSeries->removeItem(current_item);
    }
    _cbSeries_activated(_cbSeries->itemText(_cbSeries->currentIndex()));
}

void GraphEditor::_btnEditColors_clicked()
{
    if(_colorMap) {
	ColorList cl(this);
	cl.init(_colorMap);
	cl.exec();
	setColorMap(_colorMap);
    }
}

void GraphEditor::_cbSeriesStyleBars_toggled( bool yes )
{
    if(_seriesData) {
	_seriesData->style.bar = yes;
    }
}

void GraphEditor::_cbSeriesStyleLines_toggled( bool yes )
{
    if(_seriesData) {
	_seriesData->style.line = yes;
    }
}

void GraphEditor::_cbSeriesStylePoints_toggled( bool yes )
{
    if(_seriesData) {
	_seriesData->style.point = yes;
    }
}

void GraphEditor::_cbColors_activated( const QString & str)
{
    if(_seriesData) {
	_seriesData->color = str;
    }
}

void GraphEditor::_leSeriesColumn_textChanged( const QString & str )
{
    if(_seriesData) {
	_seriesData->column = str;
    }
}

void GraphEditor::_leSeriesName_textChanged( const QString & str )
{
    if(_seriesData) {
	_seriesData->name = str;
	_cbSeries->setItemText(_cbSeries->currentIndex(), str);
    }
}

void GraphEditor::_cbSeries_activated( const QString & str )
{
    // look for the series selected and make that the current series
    _seriesData = 0;

    for(int i = 0; i < _seriesList.count(); i++) {
        if(_seriesList.at(i)->name == str) {
            _seriesData = _seriesList.at(i);
            break;
        }
    }

    if(_seriesData) {
	_gbSeriesProperties->setEnabled(true);
	_leSeriesName->setText(_seriesData->name);
	_leSeriesColumn->setText(_seriesData->column);
        for(int c = 0; c < _cbColors->count(); c++) {
            if(_cbColors->itemText(c) == _seriesData->color) {
                _cbColors->setCurrentIndex(c);
                break;
            }
        }
	_cbSeriesStyleBars->setChecked(_seriesData->style.bar);
	_cbSeriesStyleLines->setChecked(_seriesData->style.line);
	_cbSeriesStylePoints->setChecked(_seriesData->style.point);
    } else {
	_gbSeriesProperties->setEnabled(false);
    }
}


QList<ORSeriesData*>& GraphEditor::getSeriesList()
{
    return _seriesList;
}

void GraphEditor::setColorMap( QMap<QString, QColor> * cmap)
{
    _colorMap = cmap;
    if(_colorMap) {
        QString col = _cbColors->currentText();
        _cbColors->clear();
        QMapIterator<QString,QColor> mit(*_colorMap);
        while(mit.hasNext())
        {
            mit.next();
    	    _cbColors->addItem(mit.key());
        }
        for( int i = 0; i < _cbColors->count(); i++) {
	    if(_cbColors->itemText(i) == col) {
	        _cbColors->setCurrentIndex(i);
	        break;
	    }
        }
        if(col.length() < 1) {
            _cbColors_activated(_cbColors->currentText());
        }
    }
}
