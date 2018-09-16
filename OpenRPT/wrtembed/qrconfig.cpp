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

#include "qrconfig.h"
#include "ui_qrconfig.h"

#include <QDebug>
#include <QCheckBox>
QRConfig::QRConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRConfig)
{
    ui->setupUi(this);
}

QRConfig::~QRConfig()
{
    delete ui;
}

void QRConfig::setAutoSize(const bool &p) {
    ui->_autosize->setChecked(p);
}

void QRConfig::setQRSize(const int &p) {
    ui->_qrSize->setCurrentIndex(p-1);
    ui->_qrsizeRad->setChecked(true);
}

void QRConfig::setErrorCorrection(const int &p) {
    if (p >= 0) {
        ui->_errorCorrection->setCurrentIndex(p-1);
        ui->_errorCorrectionChk->setChecked(true);
    }
}

QString QRConfig::format(const QString &align) {
    QString format = QString("{"
                             "\"size\": %1,"
                             "\"autosize\": \"%2\","
                             "\"errorCorrection\": %3,"
                             "\"alignment\": \"%4\","
                             "\"barcode\": \"QR\""
                             "}"
                             ).arg(ui->_qrSize->currentIndex()+1)
                              .arg(ui->_autosize->isChecked() ? "true" : "false")
                              .arg(ui->_errorCorrectionChk->isChecked() ? ui->_errorCorrection->currentIndex() + 1 : -1)
                              .arg(align);
    return format;
}
