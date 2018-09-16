include( ../../global.pri )

QT       += core gui uitools widgets

LIBS += -lz -lfontconfig

TARGET = qzint
TEMPLATE = lib
DEFINES += MAKELIB
DESTDIR = ../../lib

SOURCES += barcodeitem.cpp \
    qzint.cpp \
    common.c \
    dllversion.c \
    large.c \
    library.c \
    pdf417.c \
    png.c \
    ps.c \
    render.c \
    svg.c \
    qr.c \
    reedsol.c \
    gs1.c

HEADERS  += barcodeitem.h \
    qzint.h \
    common.h \
    font.h \
    large.h \
    pdf417.h \
    zint.h \
    qr.h \
    reedsol.h \
    gs1.h
