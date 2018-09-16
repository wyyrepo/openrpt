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

TEMPLATE = lib
CONFIG  += qt warn_on
DEFINES += MAKELIB

DESTDIR = ../../lib
MOC_DIR = tmp
OBJECTS_DIR = tmp
UI_DIR = tmp

INCLUDEPATH += ../common ../../common ../qzint

QMAKE_LIBDIR = ../../lib $$QMAKE_LIBDIR
LIBS += -ldmtx -lopenrptcommon -lMetaSQL -lqzint

HEADERS = openreports.h \
          barcodes.h \
          crosstab.h \
          graph.h \
          orcrosstab.h \
          orutils.h \
          orprerender.h \
          orprintrender.h \
          renderobjects.h \
          previewdialog.h \
          labelpaintengine.h \
          labelprintengine.h \
          satopaintengine.h \
          satoprintengine.h \
          zebrapaintengine.h \
          zebraprintengine.h \
          reportprinter.h \
          textelementsplitter.h \
          ../common/builtinformatfunctions.h \
          ../common/builtinSqlFunctions.h \
          ../common/labelsizeinfo.h \
          ../common/pagesizeinfo.h

SOURCES = openreports.cpp \
          3of9.cpp \
          ext3of9.cpp \
          i2of5.cpp \
          code128.cpp \
          codeean.cpp \
          crosstab.cpp \
          graph.cpp \
          orutils.cpp \
          orprerender.cpp \
          orprintrender.cpp \
          renderobjects.cpp \
          previewdialog.cpp \ 
          labelpaintengine.cpp \
          labelprintengine.cpp \
          satopaintengine.cpp \
          satoprintengine.cpp \
          zebrapaintengine.cpp \
          zebraprintengine.cpp \
          reportprinter.cpp \
          textelementsplitter.cpp \
          ../common/builtinformatfunctions.cpp \
          ../common/builtinSqlFunctions.cpp \
          ../common/labelsizeinfo.cpp \
          ../common/pagesizeinfo.cpp \ 
          datamatrix.cpp \
          pdf417bc.cpp \
          qrbc.cpp

QT += xml sql gui network printsupport core

TRANSLATIONS    = renderer_fr.ts renderer_it.ts renderer_ru.ts renderer_es.ts renderer_ar.ts

bundled_dmtx {
  INCLUDEPATH += ../Dmtx_Library
}
