/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2016 by OpenMFG, LLC
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

#include "pdf417config.h"
#include "ui_pdf417config.h"

#include <QDebug>
PDF417Config::PDF417Config(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PDF417Config)
{
    ui->setupUi(this);
}

PDF417Config::~PDF417Config()
{
    delete ui;
}

void PDF417Config::setCodewords(const int &p) {
    ui->_codewords->setValue(p);
}

void PDF417Config::setColumns(const int &p) {
    ui->_columns->setCurrentIndex(p);
}

void PDF417Config::setErrorCorrecton(const int &p) {
    ui->_errorCorrection->setCurrentIndex(p+1);
}

void PDF417Config ::setType(const BCType &type) {
    if (type == Truncated)
        ui->_truncated->setChecked(true);
    else
        ui->_standard->setChecked(true);
}

QString PDF417Config::format(const QString &align) {
  QString format = QString("{"
                           "\"codewords\": %1,"
                           "\"columns\": %2,"
                           "\"errorCorrection\": %3,"
                           "\"type\": \"%4\","
                           "\"alignment\": \"%5\","
                           "\"barcode\": \"PDF417\""
                           "}"
                           ).arg(ui->_codewords->value()).arg(ui->_columns->currentIndex())
                            .arg(ui->_errorCorrection->currentIndex()-1).arg(ui->_standard->isChecked() ? "standard" : "truncated")
                            .arg(align);
  return format;
}
