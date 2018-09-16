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

#ifndef SELECTMQL_H
#define SELECTMQL_H

#include <QWidget>
#include <QSqlTableModel>

#include "ui_selectmql.h"

class SelectMQL : public QWidget, public Ui::SelectMQL
{
    Q_OBJECT

  public:
    SelectMQL(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~SelectMQL();

    Q_INVOKABLE virtual int     grade()  const;
    Q_INVOKABLE virtual QString group()  const;
    Q_INVOKABLE virtual int     id()     const;
    Q_INVOKABLE virtual QString name()   const;
    Q_INVOKABLE virtual QString notes()  const;
    Q_INVOKABLE virtual QString query()  const;
    Q_INVOKABLE virtual QString schema();

  public slots:
    virtual void setId(int id);

  signals:
    void selectedQuery(int id);

  protected slots:
    virtual void languageChange();
    virtual void sAccepted();
    virtual void sPopulate();
    virtual void sRejected();

  private:
    QSqlTableModel *_model;
};

#endif // SELECTMQL_H
