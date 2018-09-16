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

#include "importwindow.h"

#include <QInputDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimerEvent>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QStringList>
#include <QDomNode>

#include "data.h"

#include "builtinSqlFunctions.h"


ImportWindow::ImportWindow(QWidget* parent, Qt::WindowFlags fl)
    : QMainWindow(parent, fl)
{
  setupUi(this);

  (void)statusBar();

  // signals and slots connections
  connect(helpIndexAction, SIGNAL(triggered()), this, SLOT(helpIndex()));
  connect(helpContentsAction, SIGNAL(triggered()), this, SLOT(helpContents()));
  connect(helpAboutAction, SIGNAL(triggered()), this, SLOT(helpAbout()));
  connect(fileExitAction, SIGNAL(triggered()), this, SLOT(fileExit()));
  connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(sAdd()));
  connect(_add, SIGNAL(clicked()), this, SLOT(sAdd()));
  connect(_import, SIGNAL(clicked()), this, SLOT(sImport()));
  connect(_remove, SIGNAL(clicked()), this, SLOT(sRemove()));
  connect(_reports, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(reportsDoubleClicked(QListWidgetItem*)));

  _reports->clear();
  _dbTimerId = startTimer(60000);
}

ImportWindow::~ImportWindow()
{
    // no need to delete child widgets, Qt does it all for us
}

void ImportWindow::languageChange()
{
    retranslateUi(this);
}

void ImportWindow::helpIndex()
{
  QMessageBox::information(this, tr("Not Yet Implemented"), tr("This function has not been implemented."));
}


void ImportWindow::helpContents()
{
  QMessageBox::information(this, tr("Not Yet Implemented"), tr("This function has not been implemented."));
}


void ImportWindow::helpAbout()
{
  QMessageBox::about(this, tr("Report Import Tool"),
                     tr("%1 version %2\n%3, All Rights Reserved\nBuild: %4\n"
                        "\n%5 is a tool for importing report definition files "
                        "into a database.")
                     .arg(OpenRPT::name, OpenRPT::version, OpenRPT::copyright,
                          OpenRPT::build, OpenRPT::name));
}


void ImportWindow::fileExit()
{
  qApp->closeAllWindows();
}


void ImportWindow::sAdd()
{
  QStringList files = QFileDialog::getOpenFileNames(this, tr("Select one or more reports to open"), QString::null,
                                                    tr("Report Definitions (*.xml)"));
  if(!files.isEmpty())
    for(QStringList::Iterator it = files.begin(); it != files.end(); ++it)
      _reports->addItem(*it);
}


void ImportWindow::sRemove()
{
  QList<QListWidgetItem *> list = _reports->selectedItems();
  for(int i = 0; i < list.count(); i++)
  {
    _reports->removeItemWidget(list.at(i));
    delete list.at(i);
  }
}


void ImportWindow::sImport()
{
  _log->append(tr("Import Started..."));
  QListWidgetItem * item = 0;
  QList<QListWidgetItem *> list = _reports->selectedItems();
  for(int i = 0; i < list.count(); i++)
  {
    item = list.at(i);
    QString xml_file = item->text();

    QString report_name  = QString::null;
    QString report_desc  = QString::null;
    QString report_src   = QString::null;
    int     report_grade = item->data(Qt::UserRole).toInt();

    if(!xml_file.isEmpty())
    {
      QFile file(xml_file);
      if(file.open(QIODevice::ReadOnly))
      {
        QDomDocument doc;
        QString errMsg;
        int errLine, errCol;
        if(doc.setContent(&file, &errMsg, &errLine, &errCol))
        {
          QDomElement root = doc.documentElement();
          if(root.tagName() == "report")
          {
            for(QDomNode n = root.firstChild();
                  !n.isNull(); n = n.nextSibling())
            {
              if(n.nodeName() == "name")
                report_name = n.firstChild().nodeValue();
              else if(n.nodeName() == "description")
                report_desc = n.firstChild().nodeValue();
            }
            report_src = doc.toString();

            if(!report_name.isEmpty())
            {
              QSqlQuery qry;
              QSqlQuery query;

              qry.prepare(getSqlFromTag("fmt09", QSqlDatabase::database().driverName()));	// MANU
              qry.bindValue(":report_name",  report_name);	// MANU
              qry.bindValue(":report_grade", report_grade);	// MANU
              qry.exec();
              if(qry.first())
              {
                // update
                query.prepare(getSqlFromTag("fmt10", QSqlDatabase::database().driverName()));	// MANU
                query.bindValue(":report_desc", report_desc);		// MANU
                query.bindValue(":report_src",   report_src);		// MANU
                query.bindValue(":report_id", qry.value(0));		// MANU
                query.bindValue(":report_name",  report_name);	// MANU
              }
              else
              {
                // insert
                query.prepare(getSqlFromTag("fmt11", QSqlDatabase::database().driverName()));	// MANU
                query.bindValue(":report_name",  report_name);	// MANU
                query.bindValue(":report_desc", report_desc);		// MANU
                query.bindValue(":report_src",   report_src);		// MANU
                query.bindValue(":report_grade", report_grade);	// MANU
              }
              
              if(!query.exec())
              {
                QSqlError err = query.lastError();
                _log->append(tr("<font color=red>The following error was encountered while trying to import %1 into the database:\n"
                                "\t%2\n\t%3\n</font>")
                              .arg(xml_file)
                              .arg(err.driverText())
                              .arg(err.databaseText()));
              }
              else
                _log->append(tr("Import successful of %1").arg(xml_file));
            }
            else
              _log->append(tr("<font color=orange>The document %1 does not have a report name defined\n</font>")
                            .arg(xml_file));
          }
          else
            _log->append(tr("<font color=red>XML Document %1 does not have root node of report\n</font>")
                          .arg(xml_file));
        }
        else
          _log->append(tr("<font color=red>Error parsing file %1: %2 on line %3 column %4\n</font>")
                        .arg(xml_file).arg(errMsg).arg(errLine).arg(errCol));
      }
      else
        _log->append(tr("<font color=red>Could not open the specified file: %1\n</font>")
                      .arg(xml_file));
    }
    else
      _log->append("<font color=red>Encountered and empty entry: No file name was given.\n</font>");
  }
  _log->append(tr("Import complete!\n\n\n"));
}


void ImportWindow::timerEvent( QTimerEvent * e )
{
  if(e->timerId() == _dbTimerId)
  {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if(db.isValid())
    {
      QSqlQuery qry(getSqlFromTag("fmt07", db.driverName()));		// MANU
    }
    // if we are not connected then we have some problems!
  }
}


void ImportWindow::reportsDoubleClicked( QListWidgetItem * item )
{
  if(!item)
    return;

  bool ok = false;
  int grade = QInputDialog::getInteger(this, tr("Edit Grade"), tr("Grade: "), item->data(Qt::UserRole).toInt(), 0, 99, 1, &ok);
  if(ok)
    item->setData(Qt::UserRole, grade);
}

