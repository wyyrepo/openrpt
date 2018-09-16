/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2011 by OpenMFG, LLC
 *
 * This file (C) 2011 by Paladin Logic, Ltd.
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

#include "labeldefinitioneditor.h"
#include <pagesizeinfo.h>
#include <xsqlquery.h>

#include <QVariant>
#include <QKeyEvent>
#include <QMessageBox>

LabelDefinitionEditor::LabelDefinitionEditor(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(saveLabelDef()));

  paperSize->clear();
  QStringList pageNames = PageSizeInfo::getPageNames();

  for (QStringList::Iterator name = pageNames.begin(); name != pageNames.end(); name++)
  {
    paperSize->addItem(*name);
    paperDefs.append(*name);
  }
}

LabelDefinitionEditor::~LabelDefinitionEditor()
{
  // no need to delete child widgets, Qt does it all for us
}

void LabelDefinitionEditor::init(LabelSizeInfo labelDef)
{
  if (!labelDef.isNull())
  {
    labelName->setText(labelDef.name());
    paperSize->setCurrentIndex(paperDefs.indexOf(labelDef.paper()));
    labelColumns->setValue(labelDef.columns());
    labelRows->setValue(labelDef.rows());
    labelWidth->setValue(labelDef.width());
    labelHeight->setValue(labelDef.height());
    labelStartXOffset->setValue(labelDef.startX());
    labelStartYOffset->setValue(labelDef.startY());
    labelHorizontalGap->setValue(labelDef.xGap());
    labelVerticalGap->setValue(labelDef.yGap());
  }

  labelDefinition = labelDef;
}

void LabelDefinitionEditor::saveLabelDef()
{
  LabelSizeInfo labelDef(
                  labelName->text(),
                  paperSize->currentText(),
                  labelColumns->value(),
                  labelRows->value(),
                  labelWidth->value(),
                  labelHeight->value(),
                  labelStartXOffset->value(),
                  labelStartYOffset->value(),
                  labelHorizontalGap->value(),
                  labelVerticalGap->value()
                );

  QString name = labelName->text();
  QString errorText = "";
  bool hasError = false;

  if (name.length() > 0)
  {
    if (name.compare(labelDefinition.name()) == 0 || !nameExists(name))
    {
      labelDefinition = labelDef;
    }
    else
    {
      errorText = "This label name is already taken.  Please enter a new, unique name.";
      hasError = true;
    }
  }
  else
  {
    errorText = "You must give this label definition a name.";
    hasError = true;
  }

  if (hasError)
  {
    QMessageBox::warning(this, tr("Warning"), errorText);
  }
  else
  {
    accept();
  }
}

bool LabelDefinitionEditor::nameExists(QString name)
{
  bool exists = false;
  XSqlQuery xqry;
  xqry.prepare("SELECT labeldef_id FROM labeldef WHERE labeldef_name=:name");
  xqry.bindValue(":name", name);
  xqry.exec();

  if (xqry.first())
  {
    exists = true;
  }
  return exists;
}

void LabelDefinitionEditor::languageChange()
{
  retranslateUi(this);
}
