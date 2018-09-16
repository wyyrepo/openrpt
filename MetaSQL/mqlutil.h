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

#ifndef __MQLUTIL_H__
#define __MQLUTIL_H__

#include <QString>

#include "metasql.h"

class MQLUtil {
  public:
    static bool extractMetadata(const QString &query, QString &group, QString &name, QString &notes);
    static bool rewriteMetadata(QString &query, const QString &group, const QString &name, const QString &notes);

    static QString mqlLoad(const QString &name, QString &errmsg, bool *valid);
    static QString mqlLoad(const QString &group, const QString &name, QString &errmsg, bool *valid);
};

// backwards compatibility functions for xtuple sources
QString mqlLastError();
void    mqlClearLastError();
MetaSQLQuery mqlLoad(const QString &name, bool *valid = 0);
MetaSQLQuery mqlLoad(const QString &group, const QString &name, bool *valid = 0);

#endif
