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

#include "quuencode.h"

#include <QString>
#include <QIODevice>
#include <QTextStream>
#include <QBuffer>
#include <QChar>

#define ENC(c) ((c) ? ((c) & 077) + ' ': '`')
#define DEC(c) (((c) - ' ') & 077)

QString QUUEncode(QIODevice & iod, const QString & _remote, int mode) {
    QString value;
    QString remote = _remote;
    if(!iod.isOpen()) {
        if(!iod.open(QIODevice::ReadOnly)) {
            qDebug("Could not open the QIODevice for reading.");
            return QString::null;
        }
    }

    if(remote == QString::null) remote = "internal";
    if(mode == 0) mode = 0644;

    value = QString().sprintf("begin %o %s\n", mode, remote.toLatin1().data());

    qint64 nr = 0;
    char data[45];
    char c;
    char * b;
    while( (nr = iod.read(&data[0], 45)) > 0 ) {
        c = ENC(nr);
        value += c;
        for(b = data; nr > 0; nr -= 3, b += 3) {
            c = b[0] >> 2;
            c = ENC(c);
            value += c;
            c = ((b[0] << 4) & 060) | ((b[1] >> 4) & 017);
            c = ENC(c);
            value += c;
            c = ((b[1] << 2) & 074) | ((b[2] >> 6) & 03);
            c = ENC(c);
            value += c;
            c = b[2] & 077;
            c = ENC(c);
            value += c;
        }
        value += '\n';
    }
    c = ENC('\0');
    value += c;
    value += "\nend\n";

    return value;
}

QByteArray QUUDecode(const QString & _source, QString * name, int * mode) {
    QString source = _source;
    QByteArray b;
    if(source.isEmpty()) return b;

    QTextStream in(&source, QIODevice::ReadOnly);

    QString l;
    // find the start of the UUEncoded data
    while( ((l = in.readLine()) != QString::null) && (!l.startsWith("begin ")) );
    if(!l.startsWith("begin ")) {
        return b; // not UUEncoded data, returns empty array    
    }

    QTextStream tin(&l, QIODevice::ReadOnly);
    QString _begin, _name, _mode;
    tin >> _begin;
    tin >> _mode;
    tin >> _name;

    if(_begin != "begin") {
        qDebug("We may have a problem. Header not parsed correctly.");
    }

    if(name != 0) {
        *name = _name;
    }
    QChar qc;
    char c;
    if(mode != 0) {
        *mode = 0;
        for(int i = 0; i < _mode.length(); i++) {
            qc = _mode.at(i);
            c = qc.toLatin1();
            c -= '0'; // convert the ascii value to the decimal value
            *mode = (*mode << 3) | (c & 07);
        }
    }

    QBuffer buf(&b);
    buf.open(QIODevice::WriteOnly);

    int num = 0;
    char c0, c1, c2, c3;
    while( ((l = in.readLine()) != QString::null) && (!l.startsWith("end")) ) {
        qc = l[0];
        c = qc.toLatin1();
        num = DEC(c);
        for(int p = 1; num > 0; p += 4, num -= 3) {
            if(num >= 3) {
                qc = l[p + 0];
                c0 = qc.toLatin1();
                qc = l[p + 1];
                c1 = qc.toLatin1();
                qc = l[p + 2];
                c2 = qc.toLatin1();
                qc = l[p + 3];
                c3 = qc.toLatin1();

                c = DEC(c0) << 2 | DEC(c1) >> 4;
                buf.putChar(c);
                c = DEC(c1) << 4 | DEC(c2) >> 2;
                buf.putChar(c);
                c = DEC(c2) << 6 | DEC(c3);
                buf.putChar(c);
            } else {
                if(num >= 1) {
                    qc = l[p + 0];
                    c0 = qc.toLatin1();
                    qc = l[p + 1];
                    c1 = qc.toLatin1();

                    c = DEC(c0) << 2 | DEC(c1) >> 4;
                    buf.putChar(c);
                }
                if(num >= 2) {
                    qc = l[p + 1];
                    c1 = qc.toLatin1();
                    qc = l[p + 2];
                    c2 = qc.toLatin1();

                    c = DEC(c1) << 4 | DEC(c2) >> 2;
                    buf.putChar(c);
                }
            }
        }
    }

    if(!l.startsWith("end")) {
        qDebug("We seem to have run out of file before we were finished.");
    }

    return b;
}
