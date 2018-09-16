#
# OpenRPT report writer and rendering engine
# Copyright (C) 2001-2016 by OpenMFG, LLC
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# Please contact info@openmfg.com with any questions on this license.
#

include( ../../global.pri )

QT      -= gui

TARGET   = dmtx
TEMPLATE = lib
CONFIG  += qt warn_on

DEFINES += DMTX_LIBRARY_LIBRARY
DESTDIR  = ../../lib

SOURCES += \
    dmtxvector2.c \
    dmtxtime.c \
    dmtxsymbol.c \
    dmtxscangrid.c \
    dmtxregion.c \
    dmtxreedsol.c \
    dmtxplacemod.c \
    dmtxmessage.c \
    dmtxmatrix3.c \
    dmtximage.c \
    dmtxencodestream.c \
    dmtxencodescheme.c \
    dmtxencodeoptimize.c \
    dmtxencodeedifact.c \
    dmtxencodec40textx12.c \
    dmtxencodebase256.c \
    dmtxencodeascii.c \
    dmtxencode.c \
    dmtxdecodescheme.c \
    dmtxdecode.c \
    dmtxbytelist.c \
    dmtx.c
#   simple_test.c \

HEADERS +=\
    dmtxstatic.h \
    dmtx.h \
    config.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE6F5730D
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = dmtx.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

