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

#include <qstring.h>
#include <qstringlist.h>

#include <qfile.h>
#include <qtextstream.h>

#include "metasql.h"

int main(int /*argc*/, const char * /*argv*/[]) {
    QTextOStream qout(stdout);

    QFile file("test.mql");
    if(file.open(IO_ReadOnly)) {
        QTextStream stream(&file);
        QString query = stream.read();
        file.close();

        MetaSQLQuery mql(query);

        if(mql.isValid()) {
            ParameterList params;
            params.append("classcode_code", QVariant("^MA"));
            qout << "The parsed query generated a valid object." << endl;
            XSqlQuery qry = mql.toQuery(params);
            while(qry.next()) {
                qDebug("row");
            }
        } else {
            qout << "The parsed query did not generate a valid object." << endl;
        }
    } else {
        qout << "Unable to open '" << file.name() << "' for reading." << endl;
    }

    return 0;
}
