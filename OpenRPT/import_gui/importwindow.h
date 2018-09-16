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

#ifndef IMPORTWINDOW_H
#define IMPORTWINDOW_H

#include <QMainWindow>

#include "ui_importwindow.h"

class ImportWindow : public QMainWindow, public Ui::ImportWindow
{
    Q_OBJECT

  public:
    ImportWindow(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~ImportWindow();

  public slots:
    virtual void helpIndex();
    virtual void helpContents();
    virtual void helpAbout();
    virtual void fileExit();
    virtual void sAdd();
    virtual void sRemove();
    virtual void sImport();

  protected:
    virtual void timerEvent( QTimerEvent * e );

  protected slots:
    virtual void languageChange();

    virtual void reportsDoubleClicked( QListWidgetItem * item );

  private:
    int _dbTimerId;

};

#endif // IMPORTWINDOW_H
