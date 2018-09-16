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

#include "mqledit.h"

#include <QApplication>
#include <QDomDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlRecord>
#include <QTextDocument>
#include <QTextStream>
#include <QSettings>

#include <parameter.h>
#include <xsqlquery.h>
#include <login.h>

#include "data.h"
#include "logoutput.h"
#include "metasql.h"
#include "metasqlsaveparameters.h"
#include "mqlutil.h"
#include "parameteredit.h"
#include "resultsoutput.h"

#define DEBUG false

static QString lastSaveDir = QString();

MQLEdit::MQLEdit(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
  setupUi(this);

  if (OpenRPT::name.isEmpty())
    OpenRPT::name = tr("MetaSQL Editor");

  _mqlSelector = 0;
  _document = _text->document();
  _document->setDefaultFont(QFont("Courier"));

  connect(_document,     SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
  connect(editFindAction,              SIGNAL(triggered()), this, SLOT(editFind()));
  connect(fileDatabaseConnectAction,   SIGNAL(triggered()), this, SLOT(fileDatabaseConnect()));
  connect(fileDatabaseDisconnectAction,SIGNAL(triggered()), this, SLOT(fileDatabaseDisconnect()));
  connect(fileDatabaseOpenAction,      SIGNAL(triggered()), this, SLOT(fileDatabaseOpen()));
  connect(fileDatabaseSaveAsAction,    SIGNAL(triggered()), this, SLOT(fileDatabaseSaveAs()));
  connect(fileExitAction,              SIGNAL(triggered()), this, SLOT(fileExit()));
  connect(fileNewAction,               SIGNAL(triggered()), this, SLOT(fileNew()));
  connect(fileOpenAction,              SIGNAL(triggered()), this, SLOT(fileOpen()));
  connect(filePrintAction,             SIGNAL(triggered()), this, SLOT(filePrint()));
  connect(fileSaveAction,              SIGNAL(triggered()), this, SLOT(fileSave()));
  connect(fileSaveAsAction,            SIGNAL(triggered()), this, SLOT(fileSaveAs()));
  connect(helpAboutAction,             SIGNAL(triggered()), this, SLOT(helpAbout()));
  connect(helpContentsAction,          SIGNAL(triggered()), this, SLOT(helpContents()));
  connect(helpIndexAction,             SIGNAL(triggered()), this, SLOT(helpIndex()));
  connect(searchForParametersAction,   SIGNAL(triggered()), this, SLOT(populateParameterEdit()));
  connect(toolsExecute_QueryAction,    SIGNAL(triggered()), this, SLOT(execQuery()));
  connect(toolsParse_QueryAction,      SIGNAL(triggered()), this, SLOT(parseQuery()));
  connect(viewExecuted_SQLAction,      SIGNAL(triggered()), this, SLOT(showExecutedSQL()));
  connect(viewLog_OutputAction,        SIGNAL(triggered()), this, SLOT(showLog()));
  connect(viewParameter_ListAction,    SIGNAL(triggered()), this, SLOT(showParamList()));
  connect(viewResultsAction,           SIGNAL(triggered()), this, SLOT(showResults()));

  QSqlDatabase db = QSqlDatabase().database();
  if(db.isValid() && db.isOpen())
    OpenRPT::loggedIn = true;
  else
  {
    OpenRPT::loggedIn = false;
    db = QSqlDatabase();
  }

  if (parent) // then must be embedded
  {
    if (DEBUG)
      qDebug("MQLEdit::MQLEdit(%p) OpenRPT::loggedIn = %d",
             parent, OpenRPT::loggedIn);
    fileDatabaseConnectAction->setVisible(! OpenRPT::loggedIn);
    fileDatabaseDisconnectAction->setVisible(! OpenRPT::loggedIn);

    fileExitAction->setText(tr("Close"));

    QToolBar *menuproxy = new QToolBar(this);
    menuproxy->setObjectName("menuproxy");
    menuproxy->setOrientation(Qt::Horizontal);
    verticalLayout->insertWidget(0, menuproxy);

    menuproxy->addAction(fileMenu->menuAction());
    menuproxy->addAction(editMenu->menuAction());
    menuproxy->addAction(ViewMenu->menuAction());
    menuproxy->addAction(ToolsMenu->menuAction());
    menuproxy->addAction(helpMenu->menuAction());
  }
  
  fileDatabaseConnectAction->setEnabled(!OpenRPT::loggedIn);
  fileDatabaseDisconnectAction->setEnabled(OpenRPT::loggedIn);
  fileDatabaseOpenAction->setEnabled(OpenRPT::loggedIn);
  fileDatabaseSaveAsAction->setEnabled(OpenRPT::loggedIn);
  
  _pEdit   = new ParameterEdit(this, Qt::Window);
  _log     = new LogOutput(this);
  _sql     = new LogOutput(this);
  _results = new ResultsOutput(this);

  _highlighter = new MetaSQLHighlighter(_document);

  QSettings settings("xTuple.com", "mqlEditor");
  lastSaveDir = settings.value("LastDirectory").toString();

  clear();

  setDestType(MQLUnknown);
}

MQLEdit::~MQLEdit()
{
  QSettings settings("xTuple.com", "mqlEditor");
  settings.setValue("LastDirectory",lastSaveDir);
  // no need to delete child widgets, Qt does it all for us
}

void MQLEdit::languageChange()
{
  retranslateUi(this);
}

void MQLEdit::fileNew()
{
  if(askSaveIfModified())
    clear();
}

void MQLEdit::fileOpen()
{
  if(askSaveIfModified())
  {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                  lastSaveDir, tr("MetaSQL (*.mql)"));
    if(!fileName.isEmpty())
    {
      QFile file(fileName);
      if(file.open(QIODevice::ReadOnly))
      {
        QTextStream stream(&file);
        _text->setText(stream.readAll());
        _text->document()->setModified(false);
        _fileName = fileName;
        _mqlGrade = -1;
        _mqlGroup = QString::null;
        _mqlName  = QString::null;
        _mqlNotes = QString::null;
        _mqlSchema= QString::null;
        setWindowTitle(getTitleString(MQLFile));
        setDestType(MQLFile);
      }
      QFileInfo fi(file);
      lastSaveDir = fi.absolutePath();
    }
  }
}

bool MQLEdit::fileSave()
{
  if (_dest == MQLFile || (_dest == MQLUnknown && ! OpenRPT::loggedIn))
  {
    if (_fileName.isEmpty())
      return saveAs();
    else
      return save();
  }
  else if (_dest == MQLDatabase)
  {
    if (_mqlGroup.isEmpty() || _mqlName.isEmpty())
      return fileDatabaseSaveAs();
    else
      return databaseSave();
  }
  else if (_dest == MQLUnknown)
  {
    QMessageBox save;
    save.setText("How do you want to save your changes?");
    QPushButton *cancel = save.addButton(QMessageBox::Cancel);
    QPushButton *db     = save.addButton(tr("Database only"),    QMessageBox::AcceptRole);
    QPushButton *file   = save.addButton(tr("File only"),        QMessageBox::AcceptRole);
    QPushButton *both   = save.addButton(tr("Database and File"),QMessageBox::AcceptRole);
    save.setEscapeButton((QAbstractButton*)cancel);
    save.exec();

    // since _dest == MQUnknown, we can assume we don't have file/group-name
    if (save.clickedButton() == (QAbstractButton*)db)
      return fileDatabaseSaveAs();
    else if (save.clickedButton() == (QAbstractButton*)file)
      return saveAs();
    else if (save.clickedButton() == (QAbstractButton*)both)
    {
      // save to db first to get group and name
      // then reset the title and type which are changed by saveAs()
      bool returnVal = fileDatabaseSaveAs() && saveAs();
      setWindowTitle(getTitleString(MQLDatabase));
      setDestType(MQLDatabase);
      return returnVal;
    }
  }

  return false;
}

void MQLEdit::fileSaveAs()
{
  saveAs();
}

void MQLEdit::filePrint()
{
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog printdlg(&printer, this);
  if (printdlg.exec() == QDialog::Accepted)
    _text->print(&printer);
}

void MQLEdit::fileExit()
{
  if(askSaveIfModified())
  {
    close();
  }
}

void MQLEdit::clear()
{
  _text->clear();
  _text->document()->setModified(false);
  setWindowModified(false);
  _fileName = QString::null;
  _mqlGrade   = -1;
  _mqlGroup = QString::null;
  _mqlName  = QString::null;
  _mqlNotes = QString::null;
  _mqlSchema= QString::null;
}

void MQLEdit::editFind()
{
  QMessageBox::information(this, tr("Not Yet Implemented"),
             tr("This function has not been implemented."));
}

void MQLEdit::helpIndex()
{
  QMessageBox::information(this, tr("Not Yet Implemented"),
             tr("This function has not been implemented."));
}

void MQLEdit::helpContents()
{
  QMessageBox::information(this, tr("Not Yet Implemented"),
             tr("This function has not been implemented."));
}

void MQLEdit::helpAbout()
{
  QMessageBox::about(this, tr("About %1").arg(OpenRPT::name),
                     tr("%1 version %2\n%3\nBuild: %4\n"
                        "\n%5 is a tool for editing and testing "
                        "MetaSQL statements.")
                     .arg(OpenRPT::name, OpenRPT::version, OpenRPT::copyright,
                          OpenRPT::build, OpenRPT::name));
}

//
// Checks to see if the document has been modified and asks the
// user if they would like to save the document before they continue
// with the operation they are trying to perform.
//
// Returns true if the operation can continue otherwise returns false
// and the calling process should not perform any actions.
//
bool MQLEdit::askSaveIfModified()
{
  if(_text->document()->isModified())
  {
    int ret = QMessageBox::question(this, tr("Document Modified!"),
                                    tr("Would you like to save your changes before continuing?"),
                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                          QMessageBox::Yes);
    switch(ret)
    {
      case QMessageBox::Yes:
        return fileSave();
        break;
      case QMessageBox::No:
        break;
      case QMessageBox::Cancel:
        return false;
      default:
        QMessageBox::warning(this, tr("Warning"),
             tr("Encountered an unknown response. No action will be taken."));
        return false;
    
    }
  }
  return true;
}

/** Save MetaSQL to a file.

    \return true if the MetaSQL statement was saved successfully,
            otherwise false.
*/
bool MQLEdit::save()
{
  if(_fileName.isEmpty())
  {
    QMessageBox::warning(this, tr("No file Specified"),
               tr("No file was specified to save to."));
    return false;
  }

  QFile file(_fileName);
  QFileInfo fi(file);
  if (file.open(QIODevice::WriteOnly))
  {
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
#ifdef Q_OS_WIN
    // bug #15457 Saving UTF files on windows does not write BOM
    stream.setGenerateByteOrderMark(true);
#endif
    stream << _text->toPlainText();
  }
  else
  {
    QMessageBox::warning(this, tr("Error Saving file"),
      tr("There was an error while trying to save the file."));
    return false;
  }
  _text->document()->setModified(false);
  lastSaveDir = fi.absolutePath();
  return true;
}

/** Ask the user for a filename, then save the MetaSQL statement to this file.

    \return true if the MetaSQL statement was saved successfully,
            otherwise false.
*/
bool MQLEdit::saveAs()
{
  QString tmpfilename = _fileName;

  if (_fileName.isEmpty() && ! _mqlGroup.isEmpty() && !_mqlName.isEmpty())
    tmpfilename = _mqlGroup + "-" + _mqlName + ".mql";

  tmpfilename = QFileDialog::getSaveFileName(this, tr("Save MetaSQL File"),
                                             lastSaveDir +
                                             QDir::separator() +
                                             tmpfilename,
                                             tr("MetaSQL Files (*.mql);;"
                                                "Text Files (*.txt)"));
  if(tmpfilename.isEmpty())
    return false;
  
  _fileName = tmpfilename;
  QFileInfo fi(tmpfilename);
  setWindowTitle(tr("[*]%1 - File: %2 (%3)")
                 .arg(OpenRPT::name, fi.fileName(), fi.absolutePath()));
  return save();
}

void MQLEdit::fileDatabaseConnect()
{
  if (!OpenRPT::loggedIn)
  {
    ParameterList params;
    params.append("name",      MQLEdit::name());
    params.append("copyright", OpenRPT::copyright);
    params.append("version",   OpenRPT::version);
    params.append("build",     OpenRPT::build);

    if(!OpenRPT::databaseURL.isEmpty())
      params.append("databaseURL", OpenRPT::databaseURL);

    login newdlg(0, "", true);
    newdlg.set(params, 0);

    if (newdlg.exec() != QDialog::Rejected)
    {
      OpenRPT::databaseURL = newdlg._databaseURL;
      OpenRPT::loggedIn = true;
    }
  }
  fileDatabaseConnectAction->setEnabled(!OpenRPT::loggedIn);
  fileDatabaseDisconnectAction->setEnabled(OpenRPT::loggedIn);
  fileDatabaseOpenAction->setEnabled(OpenRPT::loggedIn);
  fileDatabaseSaveAsAction->setEnabled(OpenRPT::loggedIn);
}

void MQLEdit::fileDatabaseDisconnect()
{
  QSqlDatabase db = QSqlDatabase::database();
  if(db.isValid())
    db.close();

  OpenRPT::loggedIn = false;
  fileDatabaseConnectAction->setEnabled(!OpenRPT::loggedIn);
  fileDatabaseDisconnectAction->setEnabled(OpenRPT::loggedIn);
}

void MQLEdit::fileDatabaseOpen()
{
  if (askSaveIfModified())
  { 
    _mqlSelector = new SelectMQL(0, Qt::Dialog);
    _mqlSelector->setWindowModality(Qt::WindowModal);
    connect(_mqlSelector, SIGNAL(selectedQuery(int)), this, SLOT(sMQLSelected(int)));
    _mqlSelector->setVisible(true);
  }
}

void MQLEdit::sMQLSelected(int id)
{
  if(_mqlSelector == 0)
    return;

  if (id > 0)
  {
    _text->setText(_mqlSelector->query());
    _text->document()->setModified(false);
    _fileName = QString::null;
    _mqlGrade = _mqlSelector->grade();
    _mqlGroup = _mqlSelector->group();
    _mqlName  = _mqlSelector->name();
    _mqlNotes = _mqlSelector->notes();
    _mqlSchema= _mqlSelector->schema();

    setWindowTitle(getTitleString(MQLDatabase));
    setDestType(MQLDatabase);
  }
  _mqlSelector->setVisible(false);
  _mqlSelector->deleteLater();
  _mqlSelector = 0;
}

void MQLEdit::fileDatabaseOpen(const int id)
{
  if (askSaveIfModified())
  {
    XSqlQuery getq;
    getq.prepare("SELECT metasql.*, nspname"
                 "  FROM metasql"
                 "  JOIN pg_class ON (metasql.tableoid=pg_class.oid)"
                 "  JOIN pg_namespace ON (relnamespace=pg_namespace.oid)"
                 " WHERE (metasql_id=:id);");
    getq.bindValue(":id", id);
    getq.exec();
    if (getq.first())
    {
      _text->setText(getq.value("metasql_query").toString());
      _text->document()->setModified(false);
      _fileName = QString::null;
      _mqlGrade = getq.value("metasql_grade").toInt();
      _mqlGroup = getq.value("metasql_group").toString();
      _mqlName  = getq.value("metasql_name").toString();
      _mqlNotes = getq.value("metasql_notes").toString();
      _mqlSchema= getq.value("nspname").toString();

      setWindowTitle(getTitleString(MQLDatabase));
      setDestType(MQLDatabase);
    }
    else if (getq.lastError().type() != QSqlError::NoError)
    {
      QMessageBox::warning(this, tr("Database Error"),
                            tr("<p>Trying to read the MetaSQL statement, "
                               "the database server returned an error: %1")
                              .arg(getq.lastError().text()));
      return;
    }
    else
    {
      QMessageBox::warning(this, tr("Not Found"),
                            tr("<p>Could not find the MetaSQL statement "
                               "with id %1.").arg(id));
      return;
    }
  }
}

bool MQLEdit::fileDatabaseSaveAs()
{
  if (DEBUG)
    qDebug("MQLEdit::fileDatabaseSave() entered with %s.%s-%s %d",
           qPrintable(_mqlSchema), qPrintable(_mqlGroup),
           qPrintable(_mqlName),   _mqlGrade);
  QString group;
  QString name;
  QString notes;
  QString schema;

  MQLUtil::extractMetadata(_text->toPlainText(), group, name, notes);
  MetaSQLSaveParameters newdlg(this, Qt::Dialog);

  newdlg.setSchema(_mqlSchema);

  if (! group.isEmpty())
    newdlg.setGroup(group);
  else if (! _mqlGroup.isEmpty())
    newdlg.setGroup(_mqlGroup);

  if (! name.isEmpty())
    newdlg.setName(name);
  else if (! _mqlName.isEmpty())
    newdlg.setName(_mqlName);

  newdlg.setNotes(notes);
  newdlg.setGrade(_mqlGrade);

  if (newdlg.exec() == QDialog::Rejected)
    return false;

  QString qrytxt = _text->toPlainText();
  MQLUtil::rewriteMetadata(qrytxt,        newdlg.group(),
                           newdlg.name(), newdlg.notes());
  _text->setPlainText(qrytxt);
  _mqlGrade = newdlg.grade();
  _mqlGroup = newdlg.group();
  _mqlName  = newdlg.name();
  _mqlNotes = newdlg.notes();
  _mqlSchema= newdlg.schema();

  return databaseSave();
}

bool MQLEdit::databaseSave()
{
  bool returnVal = false;

  ParameterList params;
  if (_mqlSchema == "public")
  {
    params.append("tablename", "metasql");
    params.append("trigname",  "metasqlaltertrigger");
  }
  else
  {
    params.append("tablename", _mqlSchema + ".pkgmetasql");
    params.append("trigname",  "pkgmetasqlaltertrigger");
  }
  MetaSQLQuery disablem("ALTER TABLE <? literal('tablename') ?> DISABLE TRIGGER <? literal('trigname') ?>;");
  MetaSQLQuery enablem("ALTER TABLE <? literal('tablename') ?> ENABLE TRIGGER <? literal('trigname') ?>;");

  XSqlQuery triggerq;

  if (_mqlGrade == 0)
  {
    triggerq = disablem.toQuery(params);
    if (triggerq.lastError().type() != QSqlError::NoError)
    {
      QMessageBox::critical(this, tr("Database Error"),
                            tr("<p>There was a problem saving the MetaSQL "
                               "statement. Only database administrators may "
                               "change grade 0 statements. Either %1 and "
                               "choose a different grade or log in as a "
                               "database administrator to make your changes."
                               "</p><pre>%2</pre>")
                            .arg(fileDatabaseSaveAsAction->text(),
                                 triggerq.lastError().text()));
      return false;
    }
  }

  QString group;
  QString name;
  QString notes;
  MQLUtil::extractMetadata(_text->toPlainText(), group, name, notes);

  XSqlQuery saveq;
  saveq.prepare("SELECT saveMetasql(:group, :name,"
                "                   CASE :nonotes WHEN true THEN NULL ELSE :notes END,"
                "                   :query, false, :schema, :grade) AS result;");
  if (_mqlGrade >= 0)
    saveq.bindValue(":grade", _mqlGrade);
  saveq.bindValue(":group",   group.isEmpty() ? _mqlGroup : group);
  saveq.bindValue(":name",    name.isEmpty()  ? _mqlName  : name);
  saveq.bindValue(":nonotes", QVariant(notes.isEmpty()));
  if (notes.isEmpty())
    saveq.bindValue(":notes", _mqlNotes.isEmpty() ? QString("") : _mqlNotes);
  else
    saveq.bindValue(":notes", notes);
  saveq.bindValue(":query",   _text->toPlainText());
  saveq.bindValue(":schema",  _mqlSchema);

  saveq.exec();
  if (saveq.first())
  {
    int result = saveq.value("result").toInt();
    if (result >= 0)
    {
      if (! _fileName.isEmpty() &&
          QMessageBox::question(this, tr("Save to %1?").arg(_fileName),
                                tr("<p>Do you also want to save this MetaSQL "
                                   "statement to %1?")
                                .arg(_fileName),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::Yes) == QMessageBox::Yes)
        (void)save();
      setDestType(MQLDatabase);
      returnVal = true;
    }
    else
      QMessageBox::critical(this, tr("Could not save MetaSQL statement"),
                           tr("<p>Trying to save the MetaSQL statement, "
                              "saveMetasql returned the error code %1.")
                              .arg(result));
  }
  else if (saveq.lastError().type() != QSqlError::NoError)
    QMessageBox::critical(this, tr("Could not save MetaSQL statement"),
                          tr("<p>Trying to save the MetaSQL statement, "
                             "the database server returned an error: %1")
                          .arg(saveq.lastError().text()));

  if (_mqlGrade == 0)
  {
    triggerq = enablem.toQuery(params);
    if (triggerq.lastError().type() != QSqlError::NoError)
      QMessageBox::critical(this, tr("Database Error"),
                            tr("<p>There was a problem resetting triggers."
                               "Please manually enable the %1 trigger "
                               "on the %2 table.</p><pre>%3</pre>")
                            .arg(params.value("trigname").toString(),
                                 params.value("tablename").toString(),
                                 triggerq.lastError().text()));
  }

  if (returnVal)
  {
    setWindowTitle(getTitleString(MQLDatabase));
    _text->document()->setModified(false);
  }

  return returnVal;
}

void MQLEdit::showParamList()
{
  _pEdit->show();
}

void MQLEdit::parseQuery()
{
  _sql->_log->clear();
  _log->_log->clear();
  _log->_log->append(tr("---- Parsing Query ----\n"));
  MetaSQLQuery mql(_text->toPlainText());
  _log->_log->append(mql.parseLog());
  if(mql.isValid()) {
      _log->_log->append(tr("Query parsed."));
  } else {
      _log->_log->append(tr("ERROR: Invalid query!"));
  }
  showLog();
}

void MQLEdit::execQuery()
{
  if(!OpenRPT::loggedIn)
  {
    QMessageBox::warning(this, tr("Not Connected"),
                         tr("You must be connected to a database in order "
                            "to execute a query."));
    return;
  }

  _results->_table->setRowCount(0);
  _results->_table->setColumnCount(0);
  
  _sql->_log->clear();
  _log->_log->clear();
  _log->_log->append(tr("---- Parsing Query ----\n"));
  MetaSQLQuery mql(_text->toPlainText());
  _log->_log->append(mql.parseLog());
  if(mql.isValid())
  {
    _log->_log->append(tr("Query parsed."));
    _log->_log->append(tr("---- Executing Query ----"));
    ParameterList plist = _pEdit->getParameterList();

    if (toolsTest_ModeAction->isChecked())
      XSqlQuery begin("BEGIN;");

    XSqlQuery qry = mql.toQuery(plist);
    _sql->_log->append(qry.executedQuery());
    if(qry.isActive()) {
        QSqlRecord rec = qry.record();
        int ncols = rec.count();
        _results->_table->setColumnCount(ncols);
        int c;
        for(c = 0; c < ncols; c++)
          _results->_table->setHorizontalHeaderItem(c, new QTableWidgetItem(rec.fieldName(c)));
        int nrows = 0;
        while(qry.next())
        {
          _results->_table->setRowCount(nrows + 1);
          for(c = 0; c < ncols; c++)
            _results->_table->setItem(nrows, c, new QTableWidgetItem(qry.value(c).toString()));
          nrows++;
        }
        showResults();
    }
    else
    {
      _log->_log->append(tr("Failed to execute query."));
      QSqlError err = qry.lastError();
      _log->_log->append(err.text());
    }

    if (toolsTest_ModeAction->isChecked())
      XSqlQuery begin("ROLLBACK;");
  }
  else
  {
    _log->_log->append(tr("ERROR: Invalid query!"));
    showLog();
  }   
}

void MQLEdit::showLog()
{
  _log->show();
}

void MQLEdit::showResults()
{
  _results->show();
}

void MQLEdit::showExecutedSQL()
{
  _sql->show();
}

QString MQLEdit::name()
{
#if defined Q_WS_WIN
  return QObject::tr("MQLEdit for Windows");
#elif defined Q_WS_X11
  return QObject::tr("MQLEdit for Linux");
#elif defined Q_WS_MAC
  return QObject::tr("MQLEdit for OS X");
#else
  return QObject::tr("MQLEdit");
#endif
}

ParameterEdit *MQLEdit::getParameterEdit()
{
  return _pEdit;
}

QString MQLEdit::getMetaSQLText()
{
  return _text->toPlainText();
}

QStringList MQLEdit::getParamsFromMetaSQLText()
{
  return getParamsFromMetaSQLText(getMetaSQLText());
}

// TODO: It would be better to alter the metasql parser to do this for us
QStringList MQLEdit::getParamsFromMetaSQLText(const QString p)
{
  QStringList result;

  QRegExp tagre("(<\\?[^?]*\\?>)");
  for (int i = 0; (i = tagre.indexIn(p, i)) != -1; i += tagre.matchedLength())
  {
    QString tag = tagre.cap(1);
    if (DEBUG)
      qDebug("getParamsFromMetaSQLText looking at %s", qPrintable(tag));

    QRegExp paramre("(['\\\"])([^'\\\"]+)\\1");
    for (int j = 0; (j = paramre.indexIn(tag, j)) != -1; j += paramre.matchedLength())
    {
      QString param = paramre.cap(2);   // the \w+, not the ['"]
      if (DEBUG)
        qDebug("getParamsFromMetaSQLText found %s", qPrintable(param));
      result.append(param);
    }
  }

  result.sort();
  result.removeDuplicates();

  return result;
}

void MQLEdit::populateParameterEdit()
{
  QStringList list = getParamsFromMetaSQLText();
  if (! list.isEmpty())
  {
    QString xml = "<report>";
    for (QStringList::iterator it = list.begin(); it != list.end(); it++)
      xml += "\n  <parameter name='" + *it + "'/>";
    xml += "\n</report>";

    // TODO: memory leak?
    QDomDocument doc;
    doc.setContent(xml);
    _pEdit->setDocument(doc);
  }
}

void MQLEdit::sTick()
{
  if(OpenRPT::loggedIn)
  {
    XSqlQuery tickle;
    // TODO make this work on other databases.
    tickle.exec("SELECT CURRENT_DATE AS dbdate;");
    // TODO: check for errors. Do we even care?
  }
  _tick.singleShot(60000, this, SLOT(sTick()));
}

bool MQLEdit::isReadOnly()
{
  return _text->isReadOnly();
}

void MQLEdit::setReadOnly(bool ro)
{
  _text->setReadOnly(ro);

  fileDatabaseConnectAction->setEnabled(! ro);
  fileDatabaseDisconnectAction->setEnabled(! ro);
  fileDatabaseSaveAsAction->setEnabled(! ro);
  fileNewAction->setEnabled(! ro);
  fileSaveAction->setEnabled(! ro);
  fileSaveAsAction->setEnabled(! ro);

  toolsExecute_QueryAction->setEnabled(! ro);
  toolsParse_QueryAction->setEnabled(! ro);
  viewExecuted_SQLAction->setEnabled(! ro);
  viewLog_OutputAction->setEnabled(! ro);
  viewParameter_ListAction->setEnabled(! ro);
  viewResultsAction->setEnabled(! ro);
}

void MQLEdit::setDestType(DestType type)
{
  switch (type)
  {
    case MQLFile:
      fileSaveAction->setText(tr("Save (File)"));
      _dest = type;
      break;
    case MQLDatabase:
      fileSaveAction->setText(tr("Save (Database)"));
      _dest = type;
      break;
    case MQLUnknown:
    default:
      fileSaveAction->setText(tr("Save"));
      _dest = MQLUnknown;
  }
}

QString MQLEdit::getTitleString(DestType type)
{
  switch (type)
  {
    case MQLFile:
    {
      QFileInfo fi(_fileName);
      return tr("[*]%1 - File: %2 (%3)")
              .arg(OpenRPT::name, fi.fileName(), fi.absolutePath());
    }
  case MQLDatabase:
      return tr("[*]%1 - Group: %2 Name: %3 Grade: %4 (%5)")
               .arg(OpenRPT::name, _mqlGroup, _mqlName,
                    (_mqlGrade < 0) ? QString("##") : QString::number(_mqlGrade),
                    _mqlSchema);
    case MQLUnknown:
    default:
      return tr("Save");
  }
}

void MQLEdit::forceTestMode(bool p)
{
  toolsTest_ModeAction->setChecked(true);
  toolsTest_ModeAction->setDisabled(p);
}
