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

#include "dbtools.h"

#include <QHash>
#include <QUrl>

void parseDatabaseURL(const QString &pDatabaseURL, QString & pProtocol, QString &pServer, QString &pDatabase, QString &pPort)
{
  int location;

  QUrl url(pDatabaseURL);
  pProtocol = url.scheme();
  pServer = url.host();
  pDatabase = url.path();
  if(pDatabase.startsWith("/"))
    pDatabase = pDatabase.mid(1);
  int port = url.port();
  if(port <= 0)
  {
    // If the port is not specified it may be the old style so lets check
    // for it in the database path like we used to do
    location = pDatabase.indexOf(":");
    if(-1 != location)
    {
      port = pDatabase.right(pDatabase.length() - location - 1).toInt();
      pDatabase = pDatabase.left(location);
    }
  }
  if(port <= 0)
    port = 5432;
  pPort = QString().setNum(port);
}

void buildDatabaseURL(QString &pTarget, const QString & pProtocol, const QString &pServer, const QString &pDatabase, const QString &pPort)
{
  pTarget = pProtocol + "://" + pServer + ":" + pPort + "/" + pDatabase;
}

QString normalizeProtocol(QString protocol)
{
  static QHash<QString, QString> map;
  if (map.isEmpty()) {
    map["db2"]     = "QDB2";
    map["ibase"]   = "QIBASE";
    map["mysql"]   = "QMYSQL";
    map["odbc"]    = "QODBC";
    map["oracle"]  = "QOCI";
    map["pgsql"]   = "QPSQL";
    map["psql"]    = "QPSQL";
    map["sqlite"]  = "QSQLITE";
    map["sqlite2"] = "QSQLITE2";
    map["sybase"]  = "QTDS";
  }

  return map.contains(protocol) ? map.value(protocol)
                                : protocol.toUpper();
}

QSqlDatabase databaseFromURL( const QString& databaseURL )
{
  QString protocol;
  QString hostName;
  QString dbName;
  QString port;

  parseDatabaseURL( databaseURL, protocol, hostName, dbName, port );
  QSqlDatabase db = QSqlDatabase::addDatabase(normalizeProtocol(protocol));
  if ( db.isValid() )
  {
    db.setDatabaseName(dbName);
    db.setHostName(hostName);
    // Check if port was provided. If not, let Qt decide, it knows default values.
    bool ok;
    int iport = port.toInt( &ok );
    if ( ok )
      db.setPort( iport );
  }
  return db;
}
