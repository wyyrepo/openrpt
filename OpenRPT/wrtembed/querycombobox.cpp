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

#include "querycombobox.h"

QueryComboBox::QueryComboBox(QWidget *parent)
  : QComboBox(parent)
{
}

QueryComboBox::~QueryComboBox()
{
}

void QueryComboBox::init (QuerySourceList * qsl, const QString &query, bool paramAndContextQueries)
{
  // populate the combo item and set it to an appropriate value
  clear();
  int selected_index = -1;
  if(paramAndContextQueries) {
    addItem(tr("Context Query"));
    if(query == "Context Query") selected_index = 0;
    addItem(tr("Parameter Query"));
    if(query == "Parameter Query") selected_index = 1;
  }
  if(qsl != NULL) {
    for(unsigned int i = 0; i < qsl->size(); i++) {
      addItem(qsl->get(i)->name());
      if(query == qsl->get(i)->name()) {
        selected_index = i + (paramAndContextQueries ? 2 : 0);
      }
    }
  } else {
    qDebug("QuerySourceList is null");
  }
  if(selected_index == -1) {
    addItem(QObject::tr("-- Select Query --"),0);
    selected_index = 0;
  }
  setCurrentIndex(selected_index);  
}

QString QueryComboBox::currentQuery ()
{
  QString qrystr = currentText();
  if(qrystr == tr("-- Select Query --")) qrystr = QString::null;
  if(qrystr == tr("Parameter Query")) qrystr = "Parameter Query";
  if(qrystr == tr("Context Query")) qrystr = "Context Query";

  return qrystr;
}

