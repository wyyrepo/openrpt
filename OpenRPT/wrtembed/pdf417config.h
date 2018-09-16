/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2016 by OpenMFG, LLC
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

#ifndef PDF417CONFIG_H
#define PDF417CONFIG_H

#include <QWidget>
#include <QComboBox>
#include <QString>

namespace Ui {
    class PDF417Config;
}

class PDF417Config : public QWidget
{
    Q_OBJECT

public:
    explicit PDF417Config(QWidget *parent = 0);
    ~PDF417Config();
    QString format(const QString &align);
    enum BCType { Standard, Truncated, HIBC };

public slots:
    void setCodewords(const int &p);
    void setColumns(const int &p);
    void setErrorCorrecton(const int &p);
    void setType(const BCType &type);

private slots:

signals:

private:
    Ui::PDF417Config *ui;

};

#endif // PDF417CONFIG_H
