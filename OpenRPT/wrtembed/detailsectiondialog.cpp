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

#include "detailsectiondialog.h"
#include "detailgroupsectiondialog.h"
#include "graphicssection.h"

#include <QVariant>
#include <QMessageBox>
#include <QCheckBox>

DetailSectionDialog::DetailSectionDialog(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnAdd, SIGNAL(clicked()), this, SLOT(btnAdd_clicked()));
  connect(btnEdit, SIGNAL(clicked()), this, SLOT(btnEdit_clicked()));
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(btnRemove_clicked()));
  connect(btnMoveUp, SIGNAL(clicked()), this, SLOT(btnMoveUp_clicked()));
  connect(brnMoveDown, SIGNAL(clicked()), this, SLOT(brnMoveDown_clicked()));

  _gsd = 0;
  lbGroups->clear();
}

DetailSectionDialog::~DetailSectionDialog()
{
  // no need to delete child widgets, Qt does it all for us
}

void DetailSectionDialog::languageChange()
{
  retranslateUi(this);
}

void DetailSectionDialog::btnAdd_clicked()
{
  // lets add a new section
  QString name = tr("unnamed");
  int i = 0;
  if(_gsd)
  {
    while(i < 100 && _gsd->findSection(name) != -1)
    {
      i++;
      name = tr("unnamed") + QString().sprintf("%d", i);
    }
    if(_gsd->findSection(name) != -1)
    {
      QMessageBox::warning(this, tr("Error Encountered"), tr("Unable to add a new group because a non-unique name could be generated."));
      return;
    }
    ORGraphicsSectionDetailGroup * rsdg = new ORGraphicsSectionDetailGroup(name, _gsd, _gsd);
    _gsd->insertSection(_gsd->groupSectionCount(), rsdg);
  }
  else
  {
    QMessageBox::warning(this, tr("Error Encountered"), tr("No document specified."));
    return;
  }
  lbGroups->addItem(name);
  lbGroups->setCurrentRow(lbGroups->count()-1);
  btnEdit_clicked();
}


void DetailSectionDialog::btnEdit_clicked()
{
  int idx = lbGroups->currentRow();
  if(idx < 0)
    return;
  DetailGroupSectionDialog * dgsd = new DetailGroupSectionDialog(this);

  if(_gsd)
  {
    ORGraphicsSectionDetailGroup * rsdg = _gsd->getSection(idx);
    dgsd->tbName->setText(rsdg->getTitle());
    dgsd->tbColumn->setText(rsdg->column());
    dgsd->breakAfterFooter->setChecked(rsdg->pageBreak()==ORGraphicsSectionDetailGroup::BreakAfterGroupFooter);
    dgsd->cbHead->setChecked(rsdg->isGroupHeadShowing());
    dgsd->cbFoot->setChecked(rsdg->isGroupFootShowing());

    bool exitLoop = false;
    while(!exitLoop)
    {
      if(dgsd->exec() == QDialog::Accepted)
      {
        QString name = dgsd->tbName->text();
        QString column = dgsd->tbColumn->text();
        bool showgh = dgsd->cbHead->isChecked();
        bool showgf = dgsd->cbFoot->isChecked();
        bool breakafterfoot = dgsd->breakAfterFooter->isChecked();

        if(name != rsdg->getTitle() && _gsd->findSection(name) != -1)
        {
          QMessageBox::warning(this, tr("Error Encountered"),
              tr("Tried to add a new Group section with a non-unique name."));
        }
        else
        {
          if(name != rsdg->getTitle())
          {
            rsdg->setTitle(name);
            lbGroups->item(idx)->setText(name);
          }
          rsdg->setColumn(column);
          rsdg->showGroupHead(showgh);
          rsdg->showGroupFoot(showgf);
          if(breakafterfoot)
            rsdg->setPageBreak(ORGraphicsSectionDetailGroup::BreakAfterGroupFooter);
          else
            rsdg->setPageBreak(ORGraphicsSectionDetailGroup::BreakNone);

          exitLoop = true;
        }
      }
      else
      {
        exitLoop = true;
      }
    }
  }

  if(dgsd)
    delete dgsd;
}


void DetailSectionDialog::btnRemove_clicked()
{
  int idx = lbGroups->currentRow();
  if(idx < 0)
    return;

  lbGroups->takeItem(idx);
  if(_gsd)
    _gsd->removeSection(idx, true);
}


void DetailSectionDialog::btnMoveUp_clicked()
{
  int idx = lbGroups->currentRow();
  if(idx <= 0)
    return;
  QString s = lbGroups->item(idx)->text();
  lbGroups->takeItem(idx);
  lbGroups->insertItem(idx-1, s);
  if(_gsd)
  {
    ORGraphicsSectionDetailGroup * rsdg = _gsd->getSection(idx);
    bool showgh = rsdg->isGroupHeadShowing();
    bool showgf = rsdg->isGroupFootShowing();
    _gsd->removeSection(idx);
    _gsd->insertSection(idx-1,rsdg);
    rsdg->showGroupHead(showgh);
    rsdg->showGroupFoot(showgf);
  }
}


void DetailSectionDialog::brnMoveDown_clicked()
{
  int idx = lbGroups->currentRow();
  if(idx == (int)(lbGroups->count() - 1))
    return;
  QString s = lbGroups->item(idx)->text();
  lbGroups->takeItem(idx);
  lbGroups->insertItem(idx+1, s);
  if(_gsd)
  {
    ORGraphicsSectionDetailGroup * rsdg = _gsd->getSection(idx);
    bool showgh = rsdg->isGroupHeadShowing();
    bool showgf = rsdg->isGroupFootShowing();
    _gsd->removeSection(idx);
    _gsd->insertSection(idx+1, rsdg);
    rsdg->showGroupHead(showgh);
    rsdg->showGroupFoot(showgf);
  }
}


void DetailSectionDialog::setReportSectionDetail( ORGraphicsSectionDetail * gsd)
{
  _gsd = gsd;
  lbGroups->clear();
  if(_gsd)
  {
    for(int i = 0; i < _gsd->groupSectionCount(); i++)
    {
      lbGroups->addItem(_gsd->getSection(i)->getTitle());
    }
  }
}

