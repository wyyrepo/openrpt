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

#include "loginOptions.h"

#include <QVariant>
#include <QSettings>
#include "dbtools.h"

class DriverInfo {
  public:
    DriverInfo(const QString & pDriver, const QString & pName,
               const QString & pAlias1 = QString::null,
               const QString & pAlias2 = QString::null,
               const QString & pAlias3 = QString::null)
    {
      driver = pDriver;
      name = pName;
      if(!pAlias1.isEmpty())
        aliases << pAlias1;
      if(!pAlias2.isEmpty())
        aliases << pAlias2;
      if(!pAlias3.isEmpty())
        aliases << pAlias3;
    }
    virtual ~DriverInfo() {}
    QString driver;
    QString name;
    QStringList aliases;

    bool isNull() const { return driver.isEmpty(); }
};

const DriverInfo _driverInfoList[] = {
  DriverInfo("QODBC",  "ODBC", "odbc"),
  DriverInfo("QODBC3", "ODBC"), // same as QODBC
  DriverInfo("QPSQL",  "PostgreSQL", "psql", "pgsql"),
  DriverInfo("QPSQL7", "PostgreSQL"), // same as QPSQL

  DriverInfo(QString::null, QString::null) // NULL record
};

/*
 *  Constructs a loginOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
loginOptions::loginOptions(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, fl)
{
  setModal(modal);
  setObjectName(name);

  setupUi(this);

  // fill in the sql driver list
  QStringList drivers = QSqlDatabase::drivers();
  QStringList modDrivers;
  QString sInsert;
  for(int i = 0; i < drivers.size(); i++)
  {
    sInsert = drivers.at(i);
    for(int l = 0; !_driverInfoList[l].isNull(); l++)
    {
      if(_driverInfoList[l].driver == drivers.at(i))
      {
        sInsert = _driverInfoList[l].name;
        break;
      }
    }
    if(!modDrivers.contains(sInsert))
      modDrivers << sInsert;
  }
  _driver->clear();
  _driver->addItems(modDrivers);
  // signals and slots connections
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));

  _database->setFocus();
}

/*
 *  Destroys the object and frees any allocated resources
 */
loginOptions::~loginOptions()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void loginOptions::languageChange()
{
  retranslateUi(this);
}

void loginOptions::set(const ParameterList &pParams)
{
  QVariant param;
  bool     valid;

  param = pParams.value("databaseURL", &valid);
  if (valid)
  {
    _databaseURL = param.toString();

    QString protocol;
    QString server;
    QString database;
    QString port;
    parseDatabaseURL(_databaseURL, protocol, server, database, port);
    for(int l = 0; !_driverInfoList[l].isNull(); l++)
    {
      if(_driverInfoList[l].driver == protocol ||
         _driverInfoList[l].aliases.contains(protocol))
      {
        protocol = _driverInfoList[l].name;
        break;
      }
    }
    int index = _driver->findText(protocol,Qt::MatchExactly);
    if(index == -1)
    {
        qDebug("Error, the protocol %s was not found!", protocol.toLatin1().data());
	// just set something
	_driver->setCurrentIndex(0);
    } else {
	_driver->setCurrentIndex(index);
    }
    _server->setText(server);
    _database->setText(database);
    _port->setText(port);
  }
}

void loginOptions::sSave()
{
  QString protocol = _driver->currentText();
  for(int l = 0; !_driverInfoList[l].isNull(); l++)
  {
    if(_driverInfoList[l].name == protocol)
    {
      protocol = _driverInfoList[l].driver;
      break;
    }
  }
  buildDatabaseURL(_databaseURL, protocol, _server->text(), _database->text(), _port->text());
  QSettings setting(QSettings::UserScope, "OpenMFG.com", "OpenRPT");
  setting.setValue("/OpenRPT/_databaseURL", _databaseURL);

  accept();
}
