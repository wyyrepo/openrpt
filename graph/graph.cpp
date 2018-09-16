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

#include <QPainter>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMap>
#include <QPair>

#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QVariant>
#include <QPaintEvent>

#include "graph.h"


Graph::Graph(QWidget * parent, const char * name) : QWidget(parent)
{
    this->setObjectName(name);
    // constructor
    _hPadding = 5;
    _vPadding = 5;

    _dataLabel = QString::null;
    _valueLabel = QString::null;
    _title = QString::null;

    _minValue = 0.0;
    _maxValue = 100.0;

    _drawBars = true;
    _drawLines = false;
    _drawPoints = false;

    _titleFont = 0;
    _dataLabelFont = 0;
    _dataFont = 0;
    _valueLabelFont = 0;
    _valueFont = 0;

    _autoMinMax = true;
    _autoRepaint = true;
}

Graph::~Graph() {
    if(_titleFont) delete _titleFont;
    _titleFont = 0;
    if(_dataLabelFont) delete _dataLabelFont;
    _dataLabelFont = 0;
    if(_dataFont) delete _dataFont;
    _dataFont = 0;
    if(_valueLabelFont) delete _valueLabelFont;
    _valueLabelFont = 0;
    if(_valueFont) delete _valueFont;
    _valueFont = 0;
}


int  Graph::hPadding() { return _hPadding; }
int  Graph::vPadding() { return _vPadding; }

void Graph::setHPadding(int pad) {
    if(pad < 0) pad = 0;
    int max_pad = width() / 2;
    if(pad > max_pad) pad = max_pad;
    _hPadding = pad;

    if(autoRepaint()) repaint();
}

void Graph::setVPadding(int pad) {
    if(pad < 0) pad = 0;
    int max_pad = height() / 2;
    if(pad > max_pad) pad = max_pad;
    _vPadding = pad;

    if(autoRepaint()) repaint();
}


QString Graph::dataLabel() { return _dataLabel; }
QString Graph::valueLabel() { return _valueLabel; }
QString Graph::title() { return _title; }

void Graph::setDataLabel(const QString & label) {
    _dataLabel = label;

    if(autoRepaint()) repaint();
}
void Graph::setValueLabel(const QString & label) {
    _valueLabel = label;

    if(autoRepaint()) repaint();
}
void Graph::setTitle(const QString & str) {
    _title = str;

    if(autoRepaint()) repaint();
}

QFont Graph::titleFont()      { return (_titleFont      ? *_titleFont      : font()); }
QFont Graph::dataLabelFont()  { return (_dataLabelFont  ? *_dataLabelFont  : font()); }
QFont Graph::dataFont()       { return (_dataFont       ? *_dataFont       : font()); }
QFont Graph::valueLabelFont() { return (_valueLabelFont ? *_valueLabelFont : font()); }
QFont Graph::valueFont()      { return (_valueFont      ? *_valueFont      : font()); }

void Graph::setTitleFont(const QFont & fnt) { setTitleFont(&fnt); }
void Graph::setTitleFont(const QFont * fnt) {
    if(fnt) {
        if(!_titleFont) _titleFont = new QFont();
        *_titleFont = *fnt;
    } else {
        if(_titleFont) delete _titleFont;
        _titleFont = 0;
    }
}

void Graph::setDataLabelFont(const QFont & fnt) { setDataLabelFont(&fnt); }
void Graph::setDataLabelFont(const QFont * fnt) {
    if(fnt) {
        if(!_dataLabelFont) _dataLabelFont = new QFont();
        *_dataLabelFont = *fnt;
    } else {
        if(_dataLabelFont) delete _dataLabelFont;
        _dataLabelFont = 0;
    }
}

void Graph::setDataFont(const QFont & fnt) { setDataFont(&fnt); }
void Graph::setDataFont(const QFont * fnt) {
    if(fnt) {
        if(!_dataFont) _dataFont = new QFont();
        *_dataFont = *fnt;
    } else {
        if(_dataFont) delete _dataFont;
        _dataFont = 0;
    }
}

void Graph::setValueLabelFont(const QFont & fnt) { setValueLabelFont(&fnt); }
void Graph::setValueLabelFont(const QFont * fnt) {
    if(fnt) {
        if(!_valueLabelFont) _valueLabelFont = new QFont();
        *_valueLabelFont = *fnt;
    } else {
        if(_valueLabelFont) delete _valueLabelFont;
        _valueLabelFont = 0;
    }
}

void Graph::setValueFont(const QFont & fnt) { setValueFont(&fnt); }
void Graph::setValueFont(const QFont * fnt) {
    if(fnt) {
        if(!_valueFont) _valueFont = new QFont();
        *_valueFont = *fnt;
    } else {
        if(_valueFont) delete _valueFont;
        _valueFont = 0;
    }
}

int   Graph::titleAlignment() { return Qt::AlignCenter; }
int   Graph::dataLabelAlignment() { return Qt::AlignCenter; }
int   Graph::valueLabelAlignment() { return Qt::AlignCenter; }

double Graph::minValue() { return _minValue; }
double Graph::maxValue() { return _maxValue; }

void   Graph::setMinValue(double d) {
    _minValue = d;
    if(autoRepaint()) repaint();
}
void   Graph::setMinValue(int i) { setMinValue((double)i); }
void   Graph::setMaxValue(double d) {
    _maxValue = d;
    if(autoRepaint()) repaint();
}
void   Graph::setMaxValue(int i) { setMaxValue((double)i); }

bool Graph::drawBars() { return _drawBars; }
bool Graph::drawLines() { return _drawLines; }
bool Graph::drawPoints() { return _drawPoints; }

void Graph::setDrawBars(bool b) {
    _drawBars = b;
    if(autoRepaint()) repaint();
}
void Graph::setDrawLines(bool b) {
    _drawLines = b;
    if(autoRepaint()) repaint();
}
void Graph::setDrawPoints(bool b) {
    _drawPoints = b;
    if(autoRepaint()) repaint();
}


void Graph::setReferenceLabel(int rnum, const QString & label) {
    GReference ref = _data[rnum];
    ref.first = label;
    _data[rnum] = ref;

    if(autoRepaint()) repaint();
}

void Graph::setSetValue(int rnum, int snum, double val) {
    GReference ref = _data[rnum];
    ref.second[snum] = val;
    _data[rnum] = ref;

    if(autoMinMax()) {
        _minValue = qMin(val, _minValue);
        _minValue = ((int)_minValue == _minValue ? _minValue : (int)_minValue - 1 );
        _maxValue = qMax(val, _maxValue);
        _maxValue = ((int)_maxValue == _maxValue ? _maxValue : (int)_maxValue + 1 );
    }

    if(autoRepaint()) repaint();
}

void Graph::setSetColor(int set, const QColor & color) { setSetColor(set,&color); }
void Graph::setSetColor(int set, const QColor * color) {
    if(color) {
        _setColors[set] = *color;
    } else {
        _setColors.remove(set);
    }
}

QColor Graph::getSetColor(int snum) {
    QColor color;
    if(_setColors.contains(snum)) {
        color = _setColors[snum];
    } else {
        switch(snum) {
            case 0:
                color = QColor(255,0,0);
                break;
            case 1:
                color = QColor(0,255,0);
                break;
            case 2:
                color = QColor(0,0,255);
                break;
            case 3:
                color = QColor(255,255,0);
                break;
            case 4:
                color = QColor(255,0,255);
                break;
            case 5:
                color = QColor(0,255,255);
                break;
            default:
                color = QColor(150,150,150);
        }
    }
    return color;
}

void Graph::clear() {
    _data.clear();
    if(autoRepaint()) repaint();
}

void Graph::setAutoMinMax(bool yes) {
    _autoMinMax = yes;
}
bool Graph::autoMinMax() { return _autoMinMax; }

void Graph::setAutoRepaint(bool yes) {
    _autoRepaint = yes;
}
bool Graph::autoRepaint() { return _autoRepaint; }

void Graph::populateFromResult(QSqlQuery & qry) {
    bool valid = false;
    bool old_repaint = autoRepaint();
    setAutoRepaint(false);
    QVariant var;
    double val;
    int cols = qry.record().count();
    while(qry.next()) {
        if(!qry.isNull(0)) {
            setReferenceLabel(qry.at(), qry.value(0).toString());
        }
        for(int c = 1; c < cols; c++) {
            if(!qry.isNull(c)) {
                val = qry.value(c).toDouble(&valid);
                if(valid) {
                    setSetValue(qry.at(), c - 1, val);
                }
            }
        }
    }
    setAutoRepaint(old_repaint);
    if(autoRepaint()) repaint();
}

const double sin45deg = 0.70710678119;
const double cos45deg = 0.70710678119;

void Graph::paintEvent(QPaintEvent * ) {
    QPainter paint(this);
    paint.save();

    int gx1 = hPadding();
    int gy1 = vPadding();
    int gx2 = width() - hPadding();
    int gy2 = height() - vPadding();

    //paint.drawRect(gx1, gy1, gx2 - gx1, gy2 - gy1);

    QFontMetrics fm(font());
    //QRect brect;

    // get the dimensions of the title label and then draw it
    if(title().length() > 0) {
        fm = QFontMetrics(titleFont());
        //brect = fm.boundingRect(title());
        paint.setFont(titleFont());
        paint.drawText(gx1, gy1, gx2 - gx1, fm.height(), titleAlignment(), title());
        gy1 += fm.height();
    }

    // we need to do some drawing that depends on some other elements having
    // already been placed... since those require that these have already been
    // placed we will just save the old value of gy2 and then calculate the
    // value that we should have after the other code runs without actually
    // drawing anything right now
    int gy2_old = gy2;
    if(dataLabel().length() > 0) {
        fm = QFontMetrics(dataLabelFont());
        gy2 -= fm.height();
    }
    fm = QFontMetrics(dataFont());
    double tlh = 0.0;
    QMapIterator<int, GReference> dlit(_data);
    while(dlit.hasNext())
    {
      dlit.next();
      tlh = qMax(sin45deg * fm.width(dlit.value().first), tlh);
    }
    // don't change this variable as we use it later
    int th = (tlh == 0.0 ? 0 : (int)(tlh + (fm.height() * sin45deg)) + 2);
    fm = QFontMetrics(valueFont());
    if(fm.height() > th) th = fm.height();
    gy2 -= th;


    // get the dimensions of the value label then draw it
    if(valueLabel().length() > 0) {
        fm = QFontMetrics(valueLabelFont());
        //brect = fm.boundingRect(valueLabel());
        paint.setFont(valueLabelFont());
        paint.save();
        paint.rotate(-90);
        paint.drawText(-gy2, gx1, gy2 - gy1, fm.height(), valueLabelAlignment(), valueLabel());
        paint.restore();
        gx1 += fm.height();
    }

    fm = QFontMetrics(valueFont());

    QString min_str = QString().sprintf("%-.0f",minValue());
    QString org_str = ( minValue() == 0.0 ? QString::null : QString("0") );
    QString max_str = QString().sprintf("%-.0f",maxValue());

    int width = qMax(fm.width(min_str), fm.width(max_str));
    if(org_str.length() > 0) width = qMax(width, fm.width(org_str));

    gx1 += (width + 5);

    int gy_max = gy1;
    int gy_min = gy2 - 1;
    int gy_org = gy_min;

    paint.setFont(valueFont());
    int tfa = Qt::AlignTop | Qt::AlignRight;
    paint.drawText(gx1 - 5 - fm.width(min_str), gy_min, fm.width(min_str), fm.height(), tfa, min_str);
    paint.drawLine(gx1 - 3, gy_min, gx1 + 2, gy_min);
    paint.drawText(gx1 - 5 - fm.width(max_str), gy_max, fm.width(max_str), fm.height(), tfa, max_str);
    paint.drawLine(gx1 - 3, gy_max, gx1 + 2, gy_max);
    int gheight = gy2 - gy1;
    double grng = maxValue() - minValue();
    if(org_str.length() > 0) {
        double perc = (0 - minValue()) / grng;
        gy_org = gy2 - (int)(perc * (double)gheight);
        paint.drawText(gx1 - 5 - fm.width(org_str), gy_org, fm.width(org_str), fm.height(), tfa, org_str);
        paint.drawLine(gx1 - 3, gy_org, gx1 + 2, gy_org);
    }

    gx1 += 3;

    // put the old value back so all the code to come draw correctly!
    gy2 = gy2_old;

    // get the dimensions of the data label then draw it
    if(dataLabel().length() > 0) {
        fm = QFontMetrics(dataLabelFont());
        //brect = fm.boundingRect(dataLabel());
        paint.setFont(dataLabelFont());
        gy2 -= fm.height();
        paint.drawText(gx1, gy2, gx2 - gx1, fm.height(), dataLabelAlignment(), dataLabel());
    }

    gy2 -= th;

    int ref_cnt = _data.count();
    int gwidth = gx2 - gx1;
    gheight = gy2 - gy1;

    if(ref_cnt > 0) {
        paint.save();
        fm = QFontMetrics(dataFont());
        paint.setFont(dataFont());
        int refwidth = qMax(1, gwidth / ref_cnt);
        int buf = (int)(refwidth / 5);
        int buf2 = buf * 2;
        int pos = gx1 + (int)((gwidth - (refwidth * ref_cnt)) / 2);
        int bar_height;
        int fmheight = fm.height();
        int fmheight_div_2 = fmheight / 2;
        int refwidth_div_2 = refwidth / 2;
        int label_offset = (int)(fmheight_div_2 * cos45deg);
        int last_label_at = -1000;
        QMap<int, double> last_map;
        QMap<int, double> this_map;
        QMapIterator<int, GReference> rit(_data);
        while(rit.hasNext())
        {
            rit.next();
            GReference ref = rit.value();
            QString label = ref.first;
            if(label.length() > 0 && ((pos + refwidth_div_2) - last_label_at) > ((label_offset * 2) + 1)) {
                last_label_at = pos + refwidth_div_2;
                int lx = (int)(((pos + refwidth_div_2) * cos45deg) - ((gy2 + label_offset) * sin45deg));
                int ly = (int)(((pos + refwidth_div_2) * sin45deg) + ((gy2 + label_offset) * cos45deg));
                int fmwidth = fm.width(label) + 5;
                paint.save();
                paint.rotate(-45);
                paint.drawText(lx - fmwidth, ly - fmheight_div_2, fmwidth, fmheight, Qt::AlignCenter | Qt::AlignTop, label);
                paint.restore();
            }

            QMapIterator<int, double> sit(ref.second);
            paint.save();
            if(drawBars() == true) {
                TSetValue tval;
                QMap<double, TSetValue> sort_map;
                sit = ref.second;
                while(sit.hasNext())
                {
                    sit.next();
                    if(sit.value() != 0.0) {
                        tval.first = sit.key();
                        tval.second = sit.value();
                        sort_map[(tval.second < 0.0 ? minValue() : maxValue()) - (tval.second < 0.0 ? -tval.second : tval.second)] = tval;
                    }
                }
                QMapIterator<double, TSetValue> it(sort_map);
                while(it.hasNext())
                {
                    it.next();
                    tval = it.value();
                    if(tval.second != 0.0) {
                        if(tval.second < 0) {
                            bar_height = (int)((tval.second / minValue()) * (gy_org - gy_min));
                        } else {
                            bar_height = (int)((tval.second / maxValue()) * (gy_org - gy_max));
                        } 
                        paint.fillRect(pos + buf, gy_org - bar_height, refwidth - buf2, bar_height, getSetColor(tval.first));
                    }
                }
            }
            if(drawLines() == true) {
                this_map.clear();
                sit = ref.second;
                while(sit.hasNext())
                {
                    sit.next();
                    this_map[sit.key()] = sit.value();
                    if(last_map.contains(sit.key())) {
                        paint.setPen(getSetColor(sit.key()));
                        double old_val = last_map[sit.key()];
                        double new_val = sit.value();
                        int ly1;
                        if(old_val < 0.0) ly1 = (int)((old_val / minValue()) * (gy_org - gy_min));
                        else              ly1 = (int)((old_val / maxValue()) * (gy_org - gy_max));
                        ly1 = gy_org - ly1;
                        int lx1 = pos - refwidth_div_2;
                        int ly2;
                        if(new_val < 0.0) ly2 = (int)((new_val / minValue()) * (gy_org - gy_min));
                        else              ly2 = (int)((new_val / maxValue()) * (gy_org - gy_max));
                        ly2 = gy_org - ly2;
                        int lx2 = pos + refwidth_div_2;
                        paint.drawLine(lx1, ly1, lx2, ly2);
                    }
                }
                last_map = this_map;
            }
            if(drawPoints() == true) {
                sit = ref.second;
                while(sit.hasNext())
                {
                    sit.next();
                    paint.setBrush(getSetColor(sit.key()));
                    paint.setPen(QColor(0,0,0));
                    int ly1;
                    if(sit.value() < 0.0) ly1 = (int)((sit.value() / minValue()) * (gy_org - gy_min));
                    else                  ly1 = (int)((sit.value() / maxValue()) * (gy_org - gy_max));
                    ly1 = gy_org - ly1;
                    int lx1 = pos + refwidth_div_2;
                    paint.drawEllipse(lx1 - 2, ly1 - 2, 5, 5);
                }
            }
            paint.restore();
            pos += refwidth;
        }
        paint.restore();
    }

    paint.drawLine(gx1, gy_org, gx2 - 1, gy_org);
    paint.drawRect(gx1, gy1, gx2 - gx1, gy2 - gy1);


    // Now that we are done return the paint device back to the state
    // it was when we started to mess with it
    paint.restore();
}
