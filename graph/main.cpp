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

#include <qapplication.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>

#include "graph.h"
#include "graphwindow.h"

#ifdef Q_WS_WIN
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef XQ_WS_WIN
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(1, 1), &wsaData)) {
        qDebug("Error starting up Windows Socket system... libpq will not work.");
    }
#endif

    QApplication app(argc, argv);

    bool haveDB = false;

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    if(db.isValid()) {
        db.setDatabaseName("test");
        db.setUserName("cryan");
        db.setPassword("password");
        db.setHostName("localhost");
        if(db.open()) {
            haveDB = true;
        } else {
            QSqlError err = db.lastError();
            qDebug("Driver:   %s", err.driverText().toLatin1().data());
            qDebug("Database: %s", err.databaseText().toLatin1().data());
            qDebug("Could not make connection to database: sql query will be disabled");
        }
    } else {
        qDebug("Could not load the database driver: sql query will be disabled");
    }

    GraphWindow gw;
    gw.init(haveDB);
    gw.show();

    int ret = app.exec();

#ifdef XQ_WS_WIN
    WSACleanup();
#endif

    return ret;
}

