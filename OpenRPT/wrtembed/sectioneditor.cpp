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

#include "sectioneditor.h"
#include "detailsectiondialog.h"
#include "documentscene.h"
#include "graphicssection.h"

#include <QVariant>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>

#include <querysource.h>

SectionEditor::SectionEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    scene = 0;
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(btnAdd, SIGNAL(clicked()), this, SLOT(btnAdd_clicked()));
    connect(btnEdit, SIGNAL(clicked()), this, SLOT(btnEdit_clicked()));
    connect(btnRemove, SIGNAL(clicked()), this, SLOT(btnRemove_clicked()));
    connect(btnMoveUp, SIGNAL(clicked()), this, SLOT(btnMoveUp_clicked()));
    connect(brnMoveDown, SIGNAL(clicked()), this, SLOT(brnMoveDown_clicked()));
    connect(cbReportHeader, SIGNAL(toggled(bool)), this, SLOT(cbReportHeader_toggled(bool)));
    connect(cbReportFooter, SIGNAL(toggled(bool)), this, SLOT(cbReportFooter_toggled(bool)));
    connect(cbHeadFirst, SIGNAL(toggled(bool)), this, SLOT(cbHeadFirst_toggled(bool)));
    connect(cbHeadLast, SIGNAL(toggled(bool)), this, SLOT(cbHeadLast_toggled(bool)));
    connect(cbHeadEven, SIGNAL(toggled(bool)), this, SLOT(cbHeadEven_toggled(bool)));
    connect(cbHeadOdd, SIGNAL(toggled(bool)), this, SLOT(cbHeadOdd_toggled(bool)));
    connect(cbFootFirst, SIGNAL(toggled(bool)), this, SLOT(cbFootFirst_toggled(bool)));
    connect(cbFootLast, SIGNAL(toggled(bool)), this, SLOT(cbFootLast_toggled(bool)));
    connect(cbFootEven, SIGNAL(toggled(bool)), this, SLOT(cbFootEven_toggled(bool)));
    connect(cbFootOdd, SIGNAL(toggled(bool)), this, SLOT(cbFootOdd_toggled(bool)));
    connect(cbHeadAny, SIGNAL(toggled(bool)), this, SLOT(cbHeadAny_toggled(bool)));
    connect(cbFootAny, SIGNAL(toggled(bool)), this, SLOT(cbFootAny_toggled(bool)));
}

SectionEditor::~SectionEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

void SectionEditor::languageChange()
{
    retranslateUi(this);
}

void SectionEditor::btnAdd_clicked()
{
  QString name = tr("unnamed");
  int i = 0;
  if(scene)
  {
    // lets add a new section
    while(i < 100 && scene->findSection(name) != -1) {
        i++;
        name = tr("unnamed") + QString().sprintf("%d", i);
    }
    if(scene->findSection(name) != -1) {
        QMessageBox::warning(this, tr("Error Encountered"), tr("Unable to add a new section because a non-unique name could be generated."));
	    return;
    }
    ORGraphicsSectionDetail * rsd = new ORGraphicsSectionDetail(scene, scene);
    rsd->setTitle(name);
    scene->insertSection(scene->detailSectionCount(), rsd);
  }
  else
  {
    QMessageBox::warning(this, tr("Error Encountered"), tr("No document was set"));
    return;
  }
  lbSections->addItem(name);
  lbSections->setCurrentRow(lbSections->count()-1);
  btnEdit_clicked();
}

void SectionEditor::btnEdit_clicked()
{
  int idx = lbSections->currentRow();
  if(idx < 0)
    return;

  DetailSectionDialog * dsd = new DetailSectionDialog(this);
  if(scene)
  {
    // get which ever item is currently selected and edit it
    ORGraphicsSectionDetail * rsd = scene->getSection(idx);
    dsd->setReportSectionDetail(rsd);
    dsd->tbName->setText(rsd->getTitle());
    dsd->breakAtEnd->setChecked(rsd->pageBreak()==ORGraphicsSectionDetail::BreakAtEnd);
    dsd->cbQuery->init(scene->qsList, rsd->query(), false);
    
    // because of some changes we take the changes regardless
    bool exitLoop = false;
    while(!exitLoop) {
        dsd->exec();
    
        QString name = dsd->tbName->text();
        QString query = dsd->cbQuery->currentQuery();
        bool breakatend = dsd->breakAtEnd->isChecked();

        if(name != rsd->getTitle() && scene->findSection(name) != -1) {
            QMessageBox::warning(this, tr("Error Encountered"),
                tr("Tried to add a new Detail section with a non-unique name."));
        } else {
            if(name != rsd->getTitle()) {
                rsd->setTitle(name);
                lbSections->item(idx)->setText(name);
            }

            rsd->setQuery(query);
            if(breakatend)
              rsd->setPageBreak(ORGraphicsSectionDetail::BreakAtEnd);
            else
              rsd->setPageBreak(ORGraphicsSectionDetail::BreakNone);

            exitLoop = true;
        }
    }
  }

  if(dsd)
    delete dsd;
}

void SectionEditor::btnRemove_clicked()
{
  // get the selected item and delete it
  int idx = lbSections->currentRow();
  if(idx == -1)
    return;

  lbSections->takeItem(idx);
  if(scene)
    scene->removeSection(idx, true);
}

void SectionEditor::btnMoveUp_clicked()
{
  // get the selected item and move it up in the list
  int idx = lbSections->currentRow();
  if(idx <= 0)
    return;
  QString s = lbSections->item(idx)->text();
  lbSections->takeItem(idx);
  lbSections->insertItem(idx-1, s);

  if(scene)
  {
    ORGraphicsSectionDetail * rsd = scene->getSection(idx);
    scene->removeSection(idx);
    scene->insertSection(idx-1,rsd);
    scene->setModified(true);
  }
}

void SectionEditor::brnMoveDown_clicked()
{
  // get the selectged item and move it down in the list
  int idx = lbSections->currentRow();
  if(idx < 0 || idx == (int)(lbSections->count() - 1))
    return;
  QString s = lbSections->item(idx)->text();
  lbSections->takeItem(idx);
  lbSections->insertItem(idx+1, s);
  if(scene)
  {
    ORGraphicsSectionDetail * rsd = scene->getSection(idx);
    scene->removeSection(idx);
    scene->insertSection(idx+1,rsd);
    scene->setModified(true);
  }
}

void SectionEditor::cbReportHeader_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertReportHead();
    else
      scene->removeReportHead();
  }
}

void SectionEditor::cbReportFooter_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertReportFoot();
    else
      scene->removeReportFoot();
  }
}

void SectionEditor::cbHeadFirst_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageHeadFirst();
    else
      scene->removePageHeadFirst();
  }
}

void SectionEditor::cbHeadLast_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageHeadLast();
    else
      scene->removePageHeadLast();
  }
}

void SectionEditor::cbHeadEven_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageHeadEven();
    else
      scene->removePageHeadEven();
  }
}

void SectionEditor::cbHeadOdd_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageHeadOdd();
    else
      scene->removePageHeadOdd();
  }
}

void SectionEditor::cbFootFirst_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageFootFirst();
    else
      scene->removePageFootFirst();
  }
}

void SectionEditor::cbFootLast_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageFootLast();
    else
      scene->removePageFootLast();
  }
}

void SectionEditor::cbFootEven_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageFootEven();
    else
      scene->removePageFootEven();
  }
}

void SectionEditor::cbFootOdd_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageFootOdd();
    else
      scene->removePageFootOdd();
  }
}


void SectionEditor::init( DocumentScene * scene )
{
    this->scene = NULL;
    // set all the properties

    cbReportHeader->setChecked(scene->getReportHead()!=NULL);
    cbReportFooter->setChecked(scene->getReportFoot()!=NULL);

    cbHeadFirst->setChecked(scene->getPageHeadFirst()!=NULL);
    cbHeadOdd->setChecked(scene->getPageHeadOdd()!=NULL);
    cbHeadEven->setChecked(scene->getPageHeadEven()!=NULL);
    cbHeadLast->setChecked(scene->getPageHeadLast()!=NULL);
    cbHeadAny->setChecked(scene->getPageHeadAny()!=NULL);

    cbFootFirst->setChecked(scene->getPageFootFirst()!=NULL);
    cbFootOdd->setChecked(scene->getPageFootOdd()!=NULL);
    cbFootEven->setChecked(scene->getPageFootEven()!=NULL);
    cbFootLast->setChecked(scene->getPageFootLast()!=NULL);
    cbFootAny->setChecked(scene->getPageFootAny()!=NULL);

    lbSections->clear();
    for(int i = 0; i < scene->detailSectionCount(); i++) {
        lbSections->addItem(scene->getSection(i)->getTitle());
    }

    // now set the scene value
    this->scene = scene;
}

void SectionEditor::cbHeadAny_toggled( bool yes )
{
  if(scene)
  {
    if(yes) {
      scene->insertPageHeadAny();
    } else {
      scene->removePageHeadAny();
    }
  }
}

void SectionEditor::cbFootAny_toggled( bool yes )
{
  if(scene)
  {
    if(yes)
      scene->insertPageFootAny();
    else
      scene->removePageFootAny();
  }
}
