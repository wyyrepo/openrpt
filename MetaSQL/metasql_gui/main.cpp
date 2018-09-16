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

#include <QApplication>
#include <QTranslator>
#include <QSqlDatabase>
#include <QMessageBox>

#include <xsqlquery.h>
#include <dbtools.h>
#include <login.h>

#include <parameter.h>

#include "data.h"

#include "mqledit.h"

int main(int argc, char *argv[])
{
  QString username  = "";
  bool    haveUsername    = false;
  bool    haveDatabaseURL = false;
  OpenRPT::loggedIn       = false;

  OpenRPT::databaseURL = "";

#if defined Q_WS_WIN
  OpenRPT::name = QObject::tr("MetaSQL Editor for Windows");
#elif defined Q_WS_X11
  OpenRPT::name = QObject::tr("MetaSQL Editor for Linux");
#elif defined Q_WS_MAC
  OpenRPT::name = QObject::tr("MetaSQL Editor for OS X");
#else
  OpenRPT::name = QObject::tr("MetaSQL Editor");
#endif

  QApplication app(argc, argv);
  app.addLibraryPath(".");

  if (argc > 1)
  {
    haveUsername        = false;
    bool    havePasswd  = false;
    QString passwd      = "";

    for (int intCounter = 1; intCounter < argc; intCounter++)
    {
      QString argument(argv[intCounter]);

      if (argument.contains("-databaseURL=")) {
        haveDatabaseURL = true;
        OpenRPT::databaseURL    = argument.right(argument.length() - 13);
      }
      else if (argument.contains("-username="))
      {
        haveUsername = true;
        username     = argument.right(argument.length() - 10);
      }
      else if (argument.contains("-passwd="))
      {
        havePasswd = true;
        passwd     = argument.right(argument.length() - 8);
      }
      else if (argument.contains("-noAuth"))
      {
        haveUsername = true;
        havePasswd   = true;
      } 

    }

    if ( (haveDatabaseURL) && (haveUsername) && (havePasswd) )
    {

	  QSqlDatabase db = databaseFromURL(OpenRPT::databaseURL);
      if (!db.isValid())
      {
        QMessageBox::critical(0, QObject::tr("Can not load database driver"),
          QObject::tr("Unable to load the database driver. Please contact your systems administrator."));
        QApplication::exit(-1);
      }

      db.setUserName(username);
      db.setPassword(passwd);

      if (!db.open())
      {
        QMessageBox::critical(0, QObject::tr("Unable to connect to database"),
          QObject::tr("Unable to connect to the database with the given information."));
        QApplication::exit(-1);
      }
      else
        OpenRPT::loggedIn = true;
    }

  }

  OpenRPT::languages.addTranslationToDefault(":/common.qm");
  OpenRPT::languages.addTranslationToDefault(":/metasql.qm");
  OpenRPT::languages.installSelected();

  MQLEdit mainwin;
  mainwin.show();

  QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
  return app.exec();
}
