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

#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QMainWindow>

#include "ui_graphwindow.h"

class GraphWindow : public QMainWindow, public Ui::GraphWindow
{
    Q_OBJECT

public:
    GraphWindow(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~GraphWindow();

public slots:
    virtual void setColCount(int);
    virtual void setRowCount(int);
    virtual void _dataTable_valueChanged( int r, int s );
    virtual void _labelTable_valueChanged( int r, int s );
    virtual void _btnClear_clicked();
    virtual void init( bool haveDB );
    virtual void _btnSql_clicked();

protected:
    bool _noUpdate;

protected slots:
    virtual void languageChange();
};

#endif // GRAPHWINDOW_H
