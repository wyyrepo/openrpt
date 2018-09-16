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

#include "reportparameterlist.h"
#include "reportparameter.h"

#include <QVariant>

ReportParameterList::ReportParameterList(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(_btnClose, SIGNAL(clicked()), this, SLOT(accept()));
    connect(_btnAdd, SIGNAL(clicked()), this, SLOT(sAdd()));
    connect(_btnEdit, SIGNAL(clicked()), this, SLOT(sEdit()));
    connect(_btnDelete, SIGNAL(clicked()), this, SLOT(sDelete()));
    connect(_lbParameters, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(sEdit(QListWidgetItem*)));

    _map = 0;
}

ReportParameterList::~ReportParameterList()
{
    // no need to delete child widgets, Qt does it all for us
}

void ReportParameterList::languageChange()
{
    retranslateUi(this);
}

void ReportParameterList::sAdd()
{
    ReportParameter rp(this);
    if(rp.exec() == QDialog::Accepted) {
	QString n = rp.paramName();
	ORParameter d = rp.paramData();
	if(!n.isEmpty() && _map) {
	    if(_map) _map->insert(n, d);
            QList<QListWidgetItem *> i = _lbParameters->findItems(n, Qt::MatchFixedString | Qt::MatchCaseSensitive);
            if(i.isEmpty()) {
                _lbParameters->addItem(n);
            }
	}
    }
}


void ReportParameterList::sEdit()
{
    sEdit(_lbParameters->currentItem());
}


void ReportParameterList::sDelete()
{
    if(_lbParameters->currentRow() != -1) {
        if(_map)
            _map->remove(_lbParameters->currentItem()->text());
        QListWidgetItem * lwi = _lbParameters->takeItem(_lbParameters->currentRow());
        if(lwi)
          delete lwi;
    }
}


void ReportParameterList::sEdit(QListWidgetItem * item)
{
    if(item) {
       int idx = _lbParameters->row(item);
       if(idx != -1) {
           QString n = _lbParameters->item(idx)->text();
           ORParameter d;
           if(_map) d = (*_map)[n];
           ReportParameter rp(this);
           rp.setParamName(n);
           rp.setParamData(d);
           if(rp.exec() == QDialog::Accepted) {
               if(rp.paramName() != n) {
                   if(_map) _map->remove(n);
               }
               n = rp.paramName();
               d = rp.paramData();
               if(_map) _map->insert(n, d);
               _lbParameters->item(idx)->setText(n);
           }
       } 
    }
}


void ReportParameterList::setList(QMap<QString, ORParameter> * m)
{
    _map = m;
    if(_map) {
        _lbParameters->clear();
        QMap<QString,ORParameter>::iterator it;
        for(it = _map->begin(); it != _map->end(); it++) {
            _lbParameters->addItem(it.key());
        }
    }
}

