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

#ifndef METASQLSAVEPARAMETERS_H
#define METASQLSAVEPARAMETERS_H

#include <QDialog>
#include <QString>

#include "ui_metasqlsaveparameters.h"

class MetaSQLSaveParameters : public QDialog, public Ui::MetaSQLSaveParameters
{
  Q_OBJECT

  public:
    MetaSQLSaveParameters(QWidget *parent = 0, Qt::WindowFlags fl = 0);
    ~MetaSQLSaveParameters();

    int     grade()     const;
    QString group()     const;
    QString name()      const;
    QString notes()     const;
    QString schema()    const;

  public slots:
    void    setGrade(const  int     p);
    void    setGroup(const  QString p);
    void    setName(const   QString p);
    void    setNotes(const  QString p);
    void    setSchema(const QString p);

  protected slots:
    virtual void languageChange();
    virtual void sGetGroups();
};

#endif
