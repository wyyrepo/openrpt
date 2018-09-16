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

/********************************

bldtbl.cpp

Chris Newey 15 June 2007
Email newey499 at hotmail.com

Utility to create report table for use by OpenRpt.

*************************************/
#include <QtGui>

#include <stdlib.h>

#include <QCoreApplication>
#include <QString>
#include <QRegExp>
#include <QSqlDatabase>
#include <QVariant>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

#include <dbtools.h>

#include "builtinSqlFunctions.h"

// return codes
#define EXIT_SUCCESS                  0
#define EXIT_ERROR_DB_DRIVER          1
#define EXIT_ERROR_DB_ENGINE          2
#define EXIT_ERROR_DB_LOGIN           3
#define EXIT_ERROR_DB_TABLE_BUILD     4
#define EXIT_ERROR_MISSING_URL        5
#define EXIT_ERROR_MISSING_USERNAME   6
#define EXIT_ERROR_MISSING_PASSWORD   7
#define EXIT_ERROR_MISSING_DB_ENGINE  8
#define EXIT_ERROR_BAD_ARGS           9


// prototypes
bool buildTable(QSqlDatabase &db, QTextStream &out, QString &dbServer);
void displayNotSupportedMesg(QSqlDatabase &db, QTextStream &out);
bool odbcSanityCheck(QSqlDatabase &db, QString &dbServer);
bool execTableBuild(QString &qryStr, QTextStream &out);
bool isValidProtocol(const QString &protocol, const bool allowOdbc);

int main(int argc, char *argv[])
{
  QCoreApplication application(argc, argv);
  application.addLibraryPath(".");

  QTextStream out(stdout, QIODevice::WriteOnly);

  if (argc > 1)
  {
    QString databaseURL;
    QString username;
    QString passwd;
    QString arguments;
    QString dbServer = QString::null;


    for (int counter = 1; counter < argc; counter++)
    {
      QString arguments(argv[counter]);

      if (arguments.startsWith("-databaseURL=", Qt::CaseInsensitive))
        databaseURL = arguments.right(arguments.length() - 13);
      else if (arguments.startsWith("-username=", Qt::CaseInsensitive))
        username = arguments.right(arguments.length() - 10);
      else if (arguments.startsWith("-passwd=", Qt::CaseInsensitive))
        passwd = arguments.right(arguments.length() - 8);
      else if (arguments.startsWith("-dbengine=", Qt::CaseInsensitive))
        dbServer = arguments.right(arguments.length() - 10);
    }


    if (  (databaseURL != "") &&
          (username != "")    &&
          (passwd != "")          ) 
    {

      QSqlDatabase db;
      QString protocol = QString::null;
      QString server   = QString::null;
      QString database = QString::null;
      QString port     = QString::null;

      // Note: parseDatabaseURL returns a default port of 5432 (Default Postgresql port)
      //       is this a bug or a feature ?
      parseDatabaseURL(databaseURL, protocol, server, database, port);

      // treat odbc connections as a special case
      if ( protocol == "odbc")
      {
        if ( dbServer == QString::null )
        {
          out << " database URL = " << databaseURL << endl;
          out << "Protocol=" << protocol << ", Host=" << server << ", Database=" << database << ", port=" << port << endl;
          out << "\"--dbengine=\" parameter required when url protocol is odbc" << endl;
          exit(EXIT_ERROR_MISSING_DB_ENGINE);
        }
        if (! isValidProtocol(dbServer, false) )
        {
          out << "Unrecognised database server: [--dbengine=" << dbServer << "]" << endl;
          out << "Protocol=" << protocol << ", Host=" << server << ", Database=" << database << ", port=" << port << endl;
          exit(EXIT_ERROR_DB_ENGINE);
        }
      }

      // Open the Database Driver
      db = databaseFromURL( databaseURL );
      if (!db.isValid())
      {
        out << " database URL = " << databaseURL << endl;
        out << "Protocol=" << protocol << ", Host=" << server << ", Database=" << database << ", port=" << port << endl;
        out << "Could not load the specified database driver." << endl;
        exit(EXIT_ERROR_DB_DRIVER);
      }

      //  Try to connect to the Database
      db.setUserName(username);
      db.setPassword(passwd);
      if (!db.open())
      {
        out << "Protocol=" << protocol << ", Host=" << db.hostName() << 
              ", Database=" << db.databaseName() << ", port=" << db.port() << endl;
        out << "Could not log into database.  System Error: "
            << db.lastError().text() << endl;
        exit(EXIT_ERROR_DB_LOGIN);
      }

      if ( ! buildTable(db, out, dbServer) ) 
        exit(EXIT_ERROR_DB_TABLE_BUILD);
    }
    else if (databaseURL == "")
    {
      out << "You must specify a Database URL by using the -databaseURL= parameter." << endl;
      exit(EXIT_ERROR_MISSING_URL);
    }
    else if (username == "")
    {
      out << "You must specify a Database Username by using the -username= parameter." << endl;
      exit(EXIT_ERROR_MISSING_USERNAME);
    }
    else if (passwd == "")
    {
      out << "You must specify a Database Password by using the -passwd= parameter." << endl;
      exit(EXIT_ERROR_MISSING_PASSWORD);
    }
  }
  else
  {
    out << "Usage: bldtbls -databaseURL='$' -username='$' -passwd='$' [-dbengine='$']" << endl;
    exit(EXIT_ERROR_BAD_ARGS);
  }

  return EXIT_SUCCESS;
}


// Builds the report table if its a database engine that is supported
bool buildTable(QSqlDatabase &db, QTextStream &out, QString &dbServer)
{
  bool result = false;  // be pessimistic
  QString stCommand;
  QString stDriver = db.driverName();

  if ( ! getSqlDriver(stDriver)) 
  {
     displayNotSupportedMesg(db, out);
	 return result;
  }

  if ( stDriver == "QODBC" ) 
  {
    if ( ! odbcSanityCheck(db, dbServer) ) 
    {
      out << "If -databaseURL uses the ODBC protocol then -dbengine " << endl;
      out << "must indicate the type of database being connected to via " << endl;
      out << "ODBC. Eg. mysql, db2, psql" << endl;
      out << "-dbengine=" << dbServer << " Do not know how to create report table for this engine" << endl;
      displayNotSupportedMesg(db, out);
	  return result;
    }
	stDriver = dbServer;
  }

  if (stDriver == "QOCI")
  {
	stCommand = getSqlFromTag("crt01", stDriver);
    execTableBuild(stCommand, out);
  	stCommand = getSqlFromTag("crt02", stDriver);
    execTableBuild(stCommand, out);
	stCommand = getSqlFromTag("crt03", stDriver);
    execTableBuild(stCommand, out);
  }
  else
  {
  	stCommand = getSqlFromTag("crt01", stDriver);
    execTableBuild(stCommand, out);

    // Create the labeldef table
    stCommand = getSqlFromTag("crt04", stDriver);
    if (execTableBuild(stCommand, out))
    {
      stCommand = getSqlFromTag("crt07", stDriver);
      execTableBuild(stCommand, out);
    }
  }

  return true;
}


void displayNotSupportedMesg(QSqlDatabase &db, QTextStream &out)
{
  out << "Selected Driver [" << db.driverName() << "]" << endl;
  out << "Cannot build the required table - Do not know how to "              << endl;
  out << "specify an auto incrementing primary key for this database engine." << endl;
  out << "CREATE TABLE report"                                                << endl;
  out << "("                                                                  << endl;
  out << "  report_id integer NOT NULL PRIMARY KEY,"                          << endl;
  out << "  report_name TEXT,"                                                << endl;
  out << "  report_descrip TEXT NOT NULL,"                                    << endl;
  out << "  report_grade INTEGER NOT NULL DEFAULT 0,"                         << endl;
  out << "  report_source TEXT"                                               << endl;
  out << ")"                                                                  << endl;
  out << endl;
}

/**********
 CDN 14/6/7
 If the database URL passed on the command line indicates an ODBC connection
 then there is a problem for which this is a kludge.

 We don't know what database engine is on the other side of the ODBC connection.

 Some years ago I played around with the ODBC API on Windows and vaguely recall its
 possible to get the driver name back for a connection. However, I don't reckon its
 worth a dependancy on the ODBC API for a little utility like this.

 The Kludge is that when the command line database url indicates an odbc connection then
 the command line argument "-dbengine='$'" is required and is expected to contain 
 the name of the database server behind the ODBC connection:-
    Eg.  "pgsql" "psql" "mysql" "db2" etc. etc. 

 A side effect of calling this function is that the above protocol passed in the dbServer
 argument to this function is mapped to the QT Driver name for the protocol.

    Eg. parameter dbServer value on entry "mysql" is mapped to "QMYSQL"

******************/
bool odbcSanityCheck(QSqlDatabase &db, QString &dbServer)
{
  bool result = true;

// qDebug("Sanity check Server %s\n", dbServer.ascii());

  if ( db.driverName() == "QODBC" )
  {
    if( "odbc" == dbServer )    // An ODBC connection to an ODBC database
    {                           // danger of disappearing up own orifice
      dbServer = "QODBC";       
      result = false;
    }
    else if ( "pgsql" == dbServer || "psql" == dbServer )
      dbServer = "QPSQL";
    else if ( "db2" == dbServer )
      dbServer = "QDB2";
    else if ( "ibase" == dbServer )
      dbServer = "QIBASE";
    else if ( "mysql" == dbServer )
      dbServer = "QMYSQL";
    else if ( "oracle" == dbServer )
      dbServer = "QOCI";
    else if ( "sqlite" == dbServer )
      dbServer = "QSQLITE";
    else if ( "sqlite2" == dbServer )
      dbServer = "QSQLITE2";
    else if ( "sybase" == dbServer )
      dbServer = "QTDS";
    else
      result = false;  // whatever type of server it is we don't know how to deal with it
  }

  return result;
}

bool execTableBuild(QString &qryStr, QTextStream &out)
{
  bool result;
  QSqlQuery qry;

  if (! (result = qry.exec(qryStr)) )
  {
    out << "Failed to build report table" << endl;
    out << "Error Number: " << qry.lastError().number() << endl;
    out << "Driver Error: " << qry.lastError().driverText() << endl;
    out << "Database Error: " << qry.lastError().databaseText() << endl;
  }
  else
    out << "Table built OK" << endl;

  return result;
}


bool isValidProtocol(const QString &protocol, const bool allowOdbc = true)
{
  if ( protocol == "odbc" )
  {
    if ( allowOdbc )
      return true;
    else
      return false;
  }

  if (
      ( "pgsql"   == protocol || "psql" == protocol ) ||
      ( "db2"     == protocol )                       ||
      ( "ibase"   == protocol )                       ||
      ( "mysql"   == protocol )                       ||
      ( "oracle"  == protocol )                       ||
      ( "sqlite"  == protocol )                       ||
      ( "sqlite2" == protocol )                       ||
      ( "sybase"  == protocol )
     )
    return true;

  return false;
}
