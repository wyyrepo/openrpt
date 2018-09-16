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
#include "mqlutil.h"

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

  _reports->clear();

  QSqlQuery qpop;
  QSqlError err;
  if(!qpop.exec(getSqlFromTag("fmt19", QSqlDatabase::database().driverName())))
  {
    err = qpop.lastError();
    _log->append(tr("<font color=red>The following error was encountered "
                    "retrieving available schemas:\n"
                    "\t%1\n\t%2\n</font>")
                  .arg(err.driverText())
                  .arg(err.databaseText()));
    return;
  } else
    while (qpop.next())
      _schema->addItem(qpop.value(0).toString(), qpop.value(0));

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
  QMessageBox::about(this, tr("MetaSQL Import Tool"),
                     tr("%1 version %2\n%3, All Rights Reserved\nBuild: %4\n"
                        "\n%5 is a tool for importing MetaSQL files into a database.")
                     .arg(OpenRPT::name, OpenRPT::version, OpenRPT::copyright,
                          OpenRPT::build, OpenRPT::name));
}


void ImportWindow::fileExit()
{
  qApp->closeAllWindows();
}


void ImportWindow::sAdd()
{
  QStringList files = QFileDialog::getOpenFileNames(this, tr("Select one or more MetaSQL files to open"), QString::null,
                                                    tr("MetSQL (*.mql)"));
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
  QList<QListWidgetItem *> list = _reports->selectedItems();
  if(list.empty())
  {
    if(QMessageBox::question(this, tr("No Reports Selected"),
      tr("You have not selected any reports to import. Would you like to select all loaded reports now?"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
      _reports->selectAll();
      list = _reports->selectedItems();
    }
  }
  QSqlQuery qtrig;
  QSqlError err;
  if(!qtrig.exec(getSqlFromTag("fmt16", QSqlDatabase::database().driverName())))
  {
    err = qtrig.lastError();
    _log->append(tr("<font color=red>The following error was encountered disabling the trigger:\n"
                    "\t%1\n\t%2\n</font>")
                  .arg(err.driverText())
                  .arg(err.databaseText()));
    return;
  }

  for(int i = 0; i < list.count(); i++)
  {
    QListWidgetItem * item = list.at(i);
    QString xml_file = item->text();

    QString metasql_name  = QString::null;
    QString metasql_group = QString::null;
    QString metasql_desc  = QString::null;
    QString metasql_src   = QString::null;

    if(!xml_file.isEmpty())
    {
      QFile file(xml_file);
      if(file.open(QIODevice::ReadOnly))
      {
        QByteArray contents = file.readAll();
        metasql_src = contents;
        if(!contents.isEmpty())
        {
          MQLUtil::extractMetadata(contents, metasql_group,
                                   metasql_name, metasql_desc);

          if(!metasql_name.isEmpty() && !metasql_group.isEmpty())
          {
            QSqlQuery qry;
            QSqlQuery query;

            query.prepare(getSqlFromTag("fmt17", QSqlDatabase::database().driverName()));
            query.bindValue(":name",  metasql_name);
            query.bindValue(":group", metasql_group);
            query.bindValue(":notes", metasql_desc);
            query.bindValue(":query", metasql_src);
            query.bindValue(":system",QVariant(false)); 
            query.bindValue(":schema",(_schema->currentText().isEmpty() ? "public" : _schema->currentText()));
            if(!query.exec())
            {
              err = query.lastError();
              _log->append(tr("<font color=red>The following error was encountered while trying to import %1 into the database:\n"
                              "\t%2\n\t%3\n</font>")
                            .arg(xml_file)
                            .arg(err.driverText())
                            .arg(err.databaseText()));
            }
            else
            {
              if (query.first())
              {
                int metasqlid = query.value(0).toInt();
                if (metasqlid < 0)
                {
                  _log->append(tr("The saveMetasql stored procedure failed for %1, returning %2.")
                            .arg(xml_file).arg(metasqlid));
                }
                else
                  _log->append(tr("Import successful of %1").arg(xml_file));
              }
              else
                _log->append(tr("No results returned from query for %1").arg(xml_file));
            }
          }
          else
            _log->append(tr("<font color=orange>The document %1 does not have a name and/or group defined\n</font>")
                          .arg(xml_file));
        }
        else
          _log->append(tr("<font color=red>Error reading file %1 or file was empty\n</font>")
                        .arg(xml_file));
      }
      else
        _log->append(tr("<font color=red>Could not open the specified file: %1\n</font>")
                      .arg(xml_file));
    }
    else
      _log->append("<font color=red>Encountered and empty entry: No file name was given.\n</font>");
  }
  if(!qtrig.exec(getSqlFromTag("fmt18", QSqlDatabase::database().driverName())))
  {
    err = qtrig.lastError();
    _log->append(tr("<font color=red>The following error was encountered re-enabling the trigger:\n"
                    "\t%1\n\t%2\n</font>")
                  .arg(err.driverText())
                  .arg(err.databaseText()));
  }
  _log->append(tr("Import complete!\n\n\n"));
}


void ImportWindow::timerEvent( QTimerEvent * e )
{
  if(e->timerId() == _dbTimerId)
  {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if(db.isValid())
      QSqlQuery qry(getSqlFromTag("fmt07", db.driverName()));
    // if we are not connected then we have some problems!
  }
}
