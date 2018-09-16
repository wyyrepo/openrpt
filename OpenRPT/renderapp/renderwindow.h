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

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include <QMainWindow>
#include <QMap>

#include "tmp/ui_renderwindow.h"

class RenderWindow : public QMainWindow, public Ui::RenderWindow
{
    Q_OBJECT

public:
    RenderWindow(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~RenderWindow();

    QString _printerName;
    bool _autoPrint;                //AUTOPRINT

    virtual ParameterList getParameterList();
    static QString name();

public slots:
    virtual void helpAbout();
    virtual void fileOpen();
    virtual void fileOpen( const QString & filename );
    virtual void fileLoad();
    virtual void fileLoad( const QString & reportname );
    virtual void filePreview( int numCopies = 1 );
    virtual void filePrint( int numCopies = 1 );
    virtual void filePrintToPDF();
    // BVI::Sednacom
    // declare the new member
    virtual void filePrintToPDF( QString & pdfFileName );
    // BVI::Sednacom
    virtual void fileExit();
    virtual void updateParam( const QString & name, const QVariant & value, bool active );
    virtual void setDocument( const QDomDocument & doc);

protected:
    QMap<QString, QList<QPair<QString,QString> > > _lists;
    QDomDocument _doc;
    QMap<QString,QVariant> _params;

protected slots:
    virtual void languageChange();

    virtual void sAdd();
    virtual void sEdit();
    virtual void sDelete();
    virtual void sSelectionChanged();
    virtual void sList();

private:
    void print (bool showPreview, int numCopies);
};

#endif // RENDERWINDOW_H
