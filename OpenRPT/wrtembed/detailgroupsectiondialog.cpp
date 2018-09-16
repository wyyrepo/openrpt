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

#include "detailgroupsectiondialog.h"

DetailGroupSectionDialog::DetailGroupSectionDialog(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setupUi(this);


    // signals and slots connections
    connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(bntCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

DetailGroupSectionDialog::~DetailGroupSectionDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void DetailGroupSectionDialog::languageChange()
{
    retranslateUi(this);
}

