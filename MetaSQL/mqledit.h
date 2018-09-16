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

#ifndef MQLEDIT_H
#define MQLEDIT_H

class LogOutput;
class ParameterEdit;
class ResultsOutput;
class QTextDocument;

#include <QWidget>
#include <QTimer>

#include "ui_mqledit.h"

#include "metasqlhighlighter.h"
#include "selectmql.h"

class MQLEdit : public QWidget, public Ui::MQLEdit
{
    Q_OBJECT

  public:
    MQLEdit(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~MQLEdit();

                static QString       name();
    Q_INVOKABLE        QString       getMetaSQLText();
    Q_INVOKABLE        ParameterEdit*getParameterEdit();
    Q_INVOKABLE        QStringList   getParamsFromMetaSQLText();
    Q_INVOKABLE static QStringList   getParamsFromMetaSQLText(const QString p);

  public slots:
    virtual void clear();
    virtual void editFind();
    virtual void execQuery();
    virtual void fileDatabaseConnect();
    virtual void fileDatabaseDisconnect();
    virtual void fileDatabaseOpen();
    virtual void fileDatabaseOpen(const int id);
    virtual bool fileDatabaseSaveAs();
    virtual void fileExit();
    virtual void fileNew();
    virtual void fileOpen();
    virtual void filePrint();
    virtual bool fileSave();
    virtual void fileSaveAs();
    virtual void forceTestMode(bool p);
    virtual void helpAbout();
    virtual void helpContents();
    virtual void helpIndex();
    virtual bool isReadOnly();
    virtual void parseQuery();
    virtual void populateParameterEdit();
    virtual void sMQLSelected(int id);
    virtual void setReadOnly(bool ro);
    virtual void showExecutedSQL();
    virtual void showLog();
    virtual void showResults();

  protected:
    enum DestType { MQLUnknown, MQLFile, MQLDatabase }; // Report, Widget, ???

    ResultsOutput *_results;
    QString        _fileName;
    int            _mqlGrade;
    QString        _mqlGroup;
    QString        _mqlName;
    QString        _mqlNotes;
    QString        _mqlSchema;
    SelectMQL     *_mqlSelector;
    ParameterEdit *_pEdit;
    LogOutput     *_log;
    LogOutput     *_sql;
    QTimer         _tick;

    virtual bool askSaveIfModified();
    virtual bool databaseSave();
    virtual QString getTitleString(DestType type);
    virtual bool save();
    virtual bool saveAs();
    virtual void sTick();
    virtual void setDestType(DestType type);

  protected slots:
    virtual void languageChange();

    virtual void showParamList();

  private:
    QTextDocument      *_document;
    MetaSQLHighlighter *_highlighter;
    DestType            _dest;
};

#endif // MQLEDIT_H
