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

#include "documentwindow.h"
#include "documentview.h"
#include "documentscene.h"
#include "reportgridoptions.h"
#include "reportpageoptions.h"
#include "reporthandler.h"
#include "graphicsitems.h"
#include "graphicssection.h"

// common
#include <xsqlquery.h>

// qt
#include <QtGui>
#include <QMessageBox>
#include <QSqlError>
#include <QWheelEvent>

// images
#include <document.xpm>

DocumentWindow::DocumentWindow(bool newDoc, ReportGridOptions * rgo, ReportHandler * handler, QWidget * parent)
  : QMainWindow(parent), _handler(handler)
{
  setAttribute(Qt::WA_DeleteOnClose, true);
  setWindowIcon(QPixmap(document_xpm));

  _scene = new DocumentScene(newDoc, _handler, this);
  _scene->setGridOptions(rgo);
  _scene->insertSection(0, new ORGraphicsSectionDetail(_scene, _scene));

  _scene->setModified(false);
  connect(_scene, SIGNAL(modified()), this, SLOT(setCaption()));

  _view = new DocumentView(_scene, this);
  _view->setDragMode(QGraphicsView::RubberBandDrag);
  _view->setResizeAnchor(QGraphicsView::AnchorViewCenter);

  setCentralWidget(_view);
  setCaption();
}

void DocumentWindow::setCaption()
{
  QString cap = QString();
  if(_scene->reportTitle().isEmpty()) cap = tr("Untitled Document");
  else cap = _scene->reportTitle();

  if(_handler && _handler->databaseElt().hasChildNodes()) {
          cap += " (m)";
  }

  if(_scene->isModified())
    cap += tr(" *");
  QMainWindow::setWindowTitle(cap);
}

void DocumentWindow::closeEvent(QCloseEvent * e)
{
  if(_scene->isModified())
  {
    switch(QMessageBox::information(this,tr("Report Writer"),tr("The document '%1' contains unsaved changes.\nDo you want to save the changes before closing?").arg(windowTitle()),tr("Save"),tr("Discard"),tr("Cancel"), 0, 2))
    {
      case 0:
        // save the doc...
        // if we get a not save result we'll bail so the
        // user doesn't loose any work.
        if(!_scene->save(this))
        {
          e->ignore();
          return;
        }
      case 1:
        // all we have to do is just accept the close event
        break;
      case 2:
        e->ignore();
        return;
      default:
        qDebug("Encountered a problem in the close event handler....");
        // should we just go ahead and close??? or should we not close???
    }
  }
  e->accept();
}
