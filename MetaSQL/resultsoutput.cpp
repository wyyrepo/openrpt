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

#include "resultsoutput.h"

#include <QClipboard>
#include <QMenu>
#include <QTableWidgetItem>

#define DEBUG false

ResultsOutput::ResultsOutput(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
  setupUi(this);

  connect(_table, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(createContextMenu(const QPoint &)));

  _menu = new QMenu(this);
  _copyAction = _menu->addAction(tr("Copy All"), this, SLOT(copy()));
}

ResultsOutput::~ResultsOutput()
{
    // no need to delete child widgets, Qt does it all for us
}

void ResultsOutput::languageChange()
{
    retranslateUi(this);
}

void ResultsOutput::copy()
{
  if (DEBUG)
    qDebug("ResultsOutput::copy() entered");

  QClipboard *clipboard = QApplication::clipboard();
  QAbstractItemModel *model = _table->model();
  QByteArray  bytes;
  for (int i = 0; i < model->rowCount(); i++)
  {
    for (int j = 0; j < model->columnCount(); j++)
    {
      if (j)
        bytes += ",";
      bytes += "\"" +
              model->data(model->index(i, j)).toString().replace("\"", "\"\"") +
              "\"";
    }
    bytes += "\n";
  }
  clipboard->setText(bytes);

  if (DEBUG)
    qDebug("ResultsOutput::copy() returning with bytes %s", bytes.data());
}

void ResultsOutput::createContextMenu(const QPoint & p)
{
  if (DEBUG)
    qDebug("ResultsOutput::createContextMenu(%d, %d)", p.x(), p.y());
  _copyAction->setEnabled(! _table->selectedItems().isEmpty());
  if (DEBUG)
    qDebug("_menu = %p\t_copyAction->enabled == %d",
           _menu, _copyAction->isEnabled());

  _menu->popup(mapToGlobal(p));
}
