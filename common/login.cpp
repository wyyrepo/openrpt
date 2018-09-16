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

#include "login.h"

#include <QVariant>
#include <QMessageBox>
#include <QCursor>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSettings>
#include <QApplication>
#include <QSplashScreen>
#include "dbtools.h"
#include "xsqlquery.h"
#include "loginOptions.h"
#include "login.h"

/*
 *  Constructs a login as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
login::login(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog(parent, fl)
{
  setModal(modal);
  setObjectName(name);

  Q_INIT_RESOURCE(OpenRPTCommon);
  setupUi(this);


  // signals and slots connections
  connect(_login, SIGNAL(clicked()), this, SLOT(sLogin()));
  connect(_options, SIGNAL(clicked()), this, SLOT(sOptions()));

  _splash = 0;

  _captive = false;

  _password->setEchoMode(QLineEdit::Password);
}

/*
 *  Destroys the object and frees any allocated resources
 */
login::~login()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void login::languageChange()
{
  retranslateUi(this);
}

int login::set(const ParameterList &pParams)
{
  return set(pParams, 0);
}

int login::set(const ParameterList &pParams, QSplashScreen *pSplash)
{
  _splash = pSplash;
  
  QVariant param;
  bool     valid;

  param = pParams.value("username", &valid);
  if (valid)
  {
    _username->setText(param.toString());
    _password->setFocus();
    _captive = true;
  }
  else
  {
    _username->setFocus();
    _captive = false;
  }

  param = pParams.value("copyright", &valid);
  if (valid)
    _copyrightLit->setText(param.toString());

  param = pParams.value("version", &valid);
  if (valid)
    _versionLit->setText(tr("Version ") + param.toString());

  param = pParams.value("build", &valid);
  if (valid)
    _build->setText(param.toString());

  param = pParams.value("name", &valid);
  if (valid)
  {
    _nameLit->setText(param.toString());
  }

  param = pParams.value("databaseURL", &valid);
  if (valid)
    _databaseURL = param.toString();
  else
  {
    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenRPT");
    _databaseURL = settings.value("/OpenRPT/_databaseURL", QVariant("pgsql://127.0.0.1/mfg:5432")).toString();
  }

  populateDatabaseInfo();

  return 0;
}

void login::sLogin()
{
  QSqlDatabase db;

// Open the Database Driver
  if (_splash)
  {
    _splash->show();
    _splash->showMessage(tr("Initializing the Database Connector"));
    qApp->processEvents();
  }

  QString databaseURL;
  databaseURL = _databaseURL;
  QString protocol;
  QString hostName;
  QString dbName;
  QString port;
  db = databaseFromURL( databaseURL );
  if (!db.isValid())
  {
    QMessageBox::warning( this, tr("No Database Driver"),
                          tr( "A connection could not be established with the specified\n"
                              "Database as the Proper Database Drivers have not been installed.\n"
                                 "Contact your Systems Administrator.\n"  ));
    
    if (_splash)
      _splash->hide();
    
    return;
  }

//  Try to connect to the Database
  _cUsername = _username->text().trimmed();
  _cPassword = _password->text().trimmed();

  db.setUserName(_cUsername);
  db.setPassword(_cPassword);
  setCursor(QCursor(Qt::WaitCursor));

  if (_splash)
  {
    _splash->showMessage(tr("Connecting to the Database"));
    qApp->processEvents();
  }
  
  bool result = db.open();

  if (!result)
  {
    if (_splash)
      _splash->hide();
    
    setCursor(QCursor(Qt::ArrowCursor));

    QMessageBox::critical( this, tr("Cannot Connect to Database Server"),
                           tr( "A connection to the specified Database Server cannot be made.  This may be due to an\n"
                               "incorrect Username and/or Password or that the Database Server in question cannot\n"
                               "support anymore connections.\n\n"
                               "Please verify your Username and Password and try again or wait until the specified\n"
                               "Database Server is less busy.\n\n"
                               "System Error '%1'\n%2" ).arg(db.lastError().text(), db.lastError().driverText()));
    if (!_captive)
    {
      _username->setText("");
      _username->setFocus();
    }
    else
      _password->setFocus();

    _password->setText("");
    return;
  }

  if (_splash)
  {
    _splash->showMessage(tr("Logging into the Database"));
    qApp->processEvents();
  }
  
  setCursor(QCursor(Qt::ArrowCursor));
  accept();
}

void login::sOptions()
{
  ParameterList params;
  params.append("databaseURL", _databaseURL);

  loginOptions newdlg(this, "", true);
  newdlg.set(params);
  if (newdlg.exec() != QDialog::Rejected)
  {
    _databaseURL = newdlg._databaseURL;
    populateDatabaseInfo();
  }
}

void login::populateDatabaseInfo()
{
  QString protocol;
  QString hostName;
  QString dbName;
  QString port;

  parseDatabaseURL(_databaseURL, protocol, hostName, dbName, port);
  _server->setText(hostName);
  _database->setText(dbName);
}

QString login::username()
{
  return _cUsername;
}

QString login::password()
{
  return _cPassword;
}

