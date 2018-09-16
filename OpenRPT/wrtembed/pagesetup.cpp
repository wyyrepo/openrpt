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

#include "pagesetup.h"

#include <QVariant>
#include <QPushButton>
#include <QComboBox>

#include <labelsizeinfo.h>

const char PageSetup::strLetter[]      = "Letter";
const char PageSetup::strLegal[]       = "Legal";
const char PageSetup::strA4[]          = "A4";
const char PageSetup::strCustom[]      = "Custom";
const char PageSetup::strLabel[]       = "Labels";


PageSetup::PageSetup(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    QDoubleValidator* dblval = new QDoubleValidator(0.01, 1000, 2, this);
    lineTop->setValidator(dblval);
    lineBottom->setValidator(dblval);
    lineLeft->setValidator(dblval);
    lineRight->setValidator(dblval);
    linePaperWidth->setValidator(dblval);
    linePaperHeight->setValidator(dblval);

    QPushButton* help = buttonBox->button(QDialogButtonBox::Help);
    help->setDisabled(true);

    comboPaperSize->clear();
    comboPaperSize->addItem(tr("Letter"), QString(strLetter));
    comboPaperSize->addItem(tr("Legal"), QString(strLegal));
    comboPaperSize->addItem(tr("A4"), QString(strA4));
    comboPaperSize->addItem(tr("Custom"), QString(strCustom));
    comboPaperSize->addItem(tr("Label"), QString(strLabel));
    connect(comboPaperSize, SIGNAL(currentIndexChanged(int)), SLOT(onPaperSize(int)));

    comboLabelType->clear();

    // Obtain the label names and build the comboLabelType list from these.
    QStringList labelNames = LabelSizeInfo::getLabelNames();
    for (QStringList::Iterator label = labelNames.begin(); label != labelNames.end(); ++label)
    {
      comboLabelType->addItem(*label, *label);
    }
}

PageSetup::~PageSetup()
{
    // no need to delete child widgets, Qt does it all for us
}

void PageSetup::languageChange()
{
    retranslateUi(this);
}

void PageSetup::onPaperSize(int idx)
{
    QString ps = comboPaperSize->itemData(idx).toString();
    enableCustom(ps==strCustom);
    grpLabels->setEnabled(ps==strLabel);
}


void PageSetup::enableCustom( bool yes )
{
    labelCustom->setEnabled(yes);
    labelWidth->setEnabled(yes);
    labelHeight->setEnabled(yes);
    labelInInches->setEnabled(yes);
    linePaperWidth->setEnabled(yes);
    linePaperHeight->setEnabled(yes);
}

void PageSetup::setData(const ReportPageOptions& rpo)
{
    lineTop->setText(QString::number(rpo.getMarginTop(),'f',3));
    lineBottom->setText(QString::number(rpo.getMarginBottom(),'f',3));
    lineLeft->setText(QString::number(rpo.getMarginLeft(),'f',3));
    lineRight->setText(QString::number(rpo.getMarginRight(),'f',3));
    if( rpo.isPortrait() )
        radioPortrait->setChecked(true);
    else
        radioLandscape->setChecked(true);
    linePaperWidth->setText(QString::number(rpo.getCustomWidth(),'f',3));
    linePaperHeight->setText(QString::number(rpo.getCustomHeight(),'f',3));

    int ps = comboPaperSize->findData(rpo.getPageSize());
    if( ps<0 )
        ps = 3;
    comboPaperSize->setCurrentIndex(ps);
    onPaperSize(ps);
    int lt = comboLabelType->findData(rpo.getLabelType());
    if( lt>=0 )
        comboLabelType->setCurrentIndex(lt);
}

void PageSetup::getData(ReportPageOptions& rpo)
{
    rpo.setMarginTop(lineTop->text().toDouble());
    rpo.setMarginBottom(lineBottom->text().toDouble());
    rpo.setMarginLeft(lineLeft->text().toDouble());
    rpo.setMarginRight(lineRight->text().toDouble());
    rpo.setPortrait(radioPortrait->isChecked());
    rpo.setPageSize(comboPaperSize->itemData(comboPaperSize->currentIndex()).toString());
    rpo.setLabelType(comboLabelType->itemData(comboLabelType->currentIndex()).toString());
    rpo.setCustomWidth(linePaperWidth->text().toDouble());
    rpo.setCustomHeight(linePaperHeight->text().toDouble());
}

