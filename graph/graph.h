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

#ifndef __GRAPH_WIDGET_H__
#define __GRAPH_WIDGET_H__

#include <qwidget.h>
#include <qstring.h>
#include <qmap.h>
#include <qpair.h>
#include <qsqlquery.h>
#include <qcolor.h>
#include <qfont.h>
//Added by qt3to4:
#include <QPaintEvent>

typedef QPair<int, double> TSetValue;
typedef QMap<int, double> GSetValue;
typedef QPair<QString, GSetValue> GReference;
typedef QMap<int, GReference> GReferences;


class Graph : public QWidget {
    Q_OBJECT
    public:
        Graph(QWidget* = 0, const char* = 0);
        ~Graph();

        int hPadding();
        int vPadding();

        QString dataLabel();
        QString valueLabel();
        QString title();

        QFont titleFont();
        QFont dataLabelFont();
        QFont dataFont();
        QFont valueLabelFont();
        QFont valueFont();

        int   titleAlignment();
        int   dataLabelAlignment();
        int   valueLabelAlignment();

        QColor getSetColor(int);

        double minValue();
        double maxValue();

        bool drawBars();
        bool drawLines();
        bool drawPoints();

        bool autoMinMax();
        bool autoRepaint();

        void populateFromResult(QSqlQuery&);

    public slots:
        void setHPadding(int);
        void setVPadding(int);

        void setDataLabel(const QString&);
        void setValueLabel(const QString&);
        void setTitle(const QString&);

        void setTitleFont(const QFont *);
        void setTitleFont(const QFont &);
        void setDataLabelFont(const QFont *);
        void setDataLabelFont(const QFont &);
        void setDataFont(const QFont *);
        void setDataFont(const QFont &);
        void setValueLabelFont(const QFont *);
        void setValueLabelFont(const QFont &);
        void setValueFont(const QFont *);
        void setValueFont(const QFont &);

        void setMinValue(double);
        void setMinValue(int);
        void setMaxValue(double);
        void setMaxValue(int);

        void setReferenceLabel(int, const QString&);
        void setSetValue(int, int, double);
        void setSetColor(int, const QColor*);
        void setSetColor(int, const QColor&);

        void setDrawBars(bool);
        void setDrawLines(bool);
        void setDrawPoints(bool);

        void setAutoMinMax(bool);
        void setAutoRepaint(bool);

        void clear();

    protected:
        virtual void paintEvent(QPaintEvent*); 

        GReferences _data;

        QString _dataLabel;
        QString _valueLabel;
        QString _title;

        int _hPadding;
        int _vPadding;

        double _minValue;
        double _maxValue;

        bool _drawBars;
        bool _drawLines;
        bool _drawPoints;

        QMap<int, QColor> _setColors;

        QFont * _titleFont;
        QFont * _dataLabelFont;
        QFont * _dataFont;
        QFont * _valueLabelFont;
        QFont * _valueFont;

        bool _autoMinMax;
        bool _autoRepaint;
};

#endif

