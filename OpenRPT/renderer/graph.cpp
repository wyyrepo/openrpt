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

/*
 *     This is a hack of the Graph Widget to render in reports.
 * In the future the two need to be merged in some fasion.
 */

#include <qstring.h>
#include <qmap.h>
#include <qpair.h>
#include <qsqlquery.h>
#include <qcolor.h>
#include <qfont.h>

#include <qpainter.h>
#include <qfont.h>
#include <qfontmetrics.h>

#include <qsqldriver.h>
#include <qsqlrecord.h>
#include <qsqlquery.h>
#include <qvariant.h>

#include <xsqlquery.h>
#include <parsexmlutils.h>

typedef QPair<int, double> TSetValue;
typedef QMap<int, double> GSetValue;
typedef QPair<QString, GSetValue> GReference;
typedef QMap<int, GReference> GReferences;


class tGraph {
    public:
        tGraph(const QRect &);
        virtual ~tGraph();

        int hPadding();
        int vPadding();

        QString dataLabel();
        QString valueLabel();
        QString title();

        QFont font();
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

        //void populateFromResult(QSqlQuery&);

        void setHPadding(int);
        void setVPadding(int);

        void setDataLabel(const QString&);
        void setValueLabel(const QString&);
        void setTitle(const QString&);

        void setFont(const QFont &);
        void setTitleFont(const QFont *);
        void setDataLabelFont(const QFont *);
        void setDataFont(const QFont *);
        void setValueLabelFont(const QFont *);
        void setValueFont(const QFont *);

        void setMinValue(double);
        void setMinValue(int);
        void setMaxValue(double);
        void setMaxValue(int);

        void setReferenceLabel(int, const QString&);
        void setSetValue(int, int, double);
        void setSetColor(int, const QColor&);
        void setSetStyle(int, const ORStyleData &);

        void setDrawBars(bool);
        void setDrawLines(bool);
        void setDrawPoints(bool);

        void setAutoMinMax(bool);
        void setAutoRepaint(bool);

        void clear();

        int width();
        int height();

        void repaint();

        virtual void draw(QPainter &); 

    protected:
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
        QMap<int, ORStyleData> _setStyle;

        QFont _font;
        QFont * _titleFont;
        QFont * _dataLabelFont;
        QFont * _dataFont;
        QFont * _valueLabelFont;
        QFont * _valueFont;

        bool _autoMinMax;
        bool _autoRepaint;

        QRect _rect;
};


tGraph::tGraph(const QRect & r) {
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

    _font = QFont();
    _titleFont = 0;
    _dataLabelFont = 0;
    _dataFont = 0;
    _valueLabelFont = 0;
    _valueFont = 0;

    _autoMinMax = true;
    _autoRepaint = true;

    _rect = r;
}

tGraph::~tGraph() {
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

int  tGraph::width() { return _rect.width(); }
int  tGraph::height() { return _rect.height(); }

int  tGraph::hPadding() { return _hPadding; }
int  tGraph::vPadding() { return _vPadding; }

void tGraph::setHPadding(int pad) {
    if(pad < 0) pad = 0;
    int max_pad = width() / 2;
    if(pad > max_pad) pad = max_pad;
    _hPadding = pad;

    if(autoRepaint()) repaint();
}

void tGraph::setVPadding(int pad) {
    if(pad < 0) pad = 0;
    int max_pad = height() / 2;
    if(pad > max_pad) pad = max_pad;
    _vPadding = pad;

    if(autoRepaint()) repaint();
}


QString tGraph::dataLabel() { return _dataLabel; }
QString tGraph::valueLabel() { return _valueLabel; }
QString tGraph::title() { return _title; }

void tGraph::setDataLabel(const QString & label) {
    _dataLabel = label;

    if(autoRepaint()) repaint();
}
void tGraph::setValueLabel(const QString & label) {
    _valueLabel = label;

    if(autoRepaint()) repaint();
}
void tGraph::setTitle(const QString & str) {
    _title = str;

    if(autoRepaint()) repaint();
}

QFont tGraph::font()           { return _font; }
QFont tGraph::titleFont()      { return (_titleFont      ? *_titleFont      : font()); }
QFont tGraph::dataLabelFont()  { return (_dataLabelFont  ? *_dataLabelFont  : font()); }
QFont tGraph::dataFont()       { return (_dataFont       ? *_dataFont       : font()); }
QFont tGraph::valueLabelFont() { return (_valueLabelFont ? *_valueLabelFont : font()); }
QFont tGraph::valueFont()      { return (_valueFont      ? *_valueFont      : font()); }

void tGraph::setFont(const QFont & f) {
    _font = f;
}

void tGraph::setTitleFont(const QFont * fnt) {
    if(fnt) {
        if(!_titleFont) _titleFont = new QFont();
        *_titleFont = *fnt;
    } else {
        if(_titleFont) delete _titleFont;
        _titleFont = 0;
    }
}

void tGraph::setDataLabelFont(const QFont * fnt) {
    if(fnt) {
        if(!_dataLabelFont) _dataLabelFont = new QFont();
        *_dataLabelFont = *fnt;
    } else {
        if(_dataLabelFont) delete _dataLabelFont;
        _dataLabelFont = 0;
    }
}

void tGraph::setDataFont(const QFont * fnt) {
    if(fnt) {
        if(!_dataFont) _dataFont = new QFont();
        *_dataFont = *fnt;
    } else {
        if(_dataFont) delete _dataFont;
        _dataFont = 0;
    }
}

void tGraph::setValueLabelFont(const QFont * fnt) {
    if(fnt) {
        if(!_valueLabelFont) _valueLabelFont = new QFont();
        *_valueLabelFont = *fnt;
    } else {
        if(_valueLabelFont) delete _valueLabelFont;
        _valueLabelFont = 0;
    }
}

void tGraph::setValueFont(const QFont * fnt) {
    if(fnt) {
        if(!_valueFont) _valueFont = new QFont();
        *_valueFont = *fnt;
    } else {
        if(_valueFont) delete _valueFont;
        _valueFont = 0;
    }
}

int   tGraph::titleAlignment() { return Qt::AlignCenter; }
int   tGraph::dataLabelAlignment() { return Qt::AlignCenter; }
int   tGraph::valueLabelAlignment() { return Qt::AlignCenter; }

double tGraph::minValue() { return _minValue; }
double tGraph::maxValue() { return _maxValue; }

void   tGraph::setMinValue(double d) {
    _minValue = d;
    if(autoRepaint()) repaint();
}
void   tGraph::setMinValue(int i) { setMinValue((double)i); }
void   tGraph::setMaxValue(double d) {
    _maxValue = d;
    if(autoRepaint()) repaint();
}
void   tGraph::setMaxValue(int i) { setMaxValue((double)i); }

bool tGraph::drawBars() {
    //return _drawBars;

    QMapIterator<int, ORStyleData> mit(_setStyle);
    while(mit.hasNext())
    {
        mit.next();
        if(mit.value().bar) return true;
    }
    return false;
}

bool tGraph::drawLines() {
    //return _drawLines;

    QMapIterator<int, ORStyleData> mit(_setStyle);
    while(mit.hasNext())
    {
        mit.next();
        if(mit.value().line) return true;
    }
    return false;
}
bool tGraph::drawPoints() {
    //return _drawPoints;

    QMapIterator<int, ORStyleData> mit(_setStyle);
    while(mit.hasNext())
    {
        mit.next();
        if(mit.value().point) return true;
    }
    return false;
}

void tGraph::setDrawBars(bool b) {
    _drawBars = b;
    if(autoRepaint()) repaint();
}
void tGraph::setDrawLines(bool b) {
    _drawLines = b;
    if(autoRepaint()) repaint();
}
void tGraph::setDrawPoints(bool b) {
    _drawPoints = b;
    if(autoRepaint()) repaint();
}


void tGraph::setReferenceLabel(int rnum, const QString & label) {
    GReference ref = _data[rnum];
    ref.first = label;
    _data[rnum] = ref;

    if(autoRepaint()) repaint();
}

void tGraph::setSetValue(int rnum, int snum, double val) {
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

void tGraph::setSetStyle(int set, const ORStyleData & s) {
    _setStyle[set] = s;
}

void tGraph::setSetColor(int set, const QColor & color) {
    // The remove portion was not being used and the implmentation was causing warning/errors
    //if(color) {
        _setColors[set] = color;
    //} else {
    //    _setColors.remove(set);
    //}
}

QColor tGraph::getSetColor(int snum) {
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

void tGraph::clear() {
    _data.clear();
    if(autoRepaint()) repaint();
}

void tGraph::setAutoMinMax(bool yes) {
    _autoMinMax = yes;
}
bool tGraph::autoMinMax() { return _autoMinMax; }

void tGraph::setAutoRepaint(bool yes) {
    _autoRepaint = yes;
}
bool tGraph::autoRepaint() { return _autoRepaint; }

/* ***********************************************
void tGraph::populateFromResult(QSqlQuery & qry) {
    bool valid = false;
    bool old_repaint = autoRepaint();
    setAutoRepaint(false);
    QVariant var;
    double val;
    int cols = qry.driver()->record(qry).count();
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
 */

const double sin45deg = 0.70710678119;
const double cos45deg = 0.70710678119;

void tGraph::draw(QPainter & paint) {
    //QPainter paint(this);
    paint.save();

    int gx1 = hPadding() + _rect.x();
    int gy1 = vPadding() + _rect.y();
    int gx2 = _rect.x() + width() - hPadding();
    int gy2 = _rect.y() + height() - vPadding();

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

    gx1 += width;

    int gy_max = gy1;
    int gy_min = gy2 - 1;
    int gy_org = gy_min;

    paint.setFont(valueFont());
    int tfa = Qt::AlignTop | Qt::AlignRight;
    paint.drawText(gx1 - (fm.width(min_str)+1), gy_min, fm.width(min_str)+1, fm.height(), tfa, min_str);
    paint.drawLine(gx1 - 3, gy_min, gx1 + 2, gy_min);
    paint.drawText(gx1 - (fm.width(max_str)+1), gy_max, fm.width(max_str)+1, fm.height(), tfa, max_str);
    paint.drawLine(gx1 - 3, gy_max, gx1 + 2, gy_max);
    int gheight = gy2 - gy1;
    double grng = maxValue() - minValue();
    if(org_str.length() > 0) {
        double perc = (0 - minValue()) / grng;
        gy_org = gy2 - (int)(perc * (double)gheight);
        paint.drawText(gx1 - fm.width(org_str), gy_org, fm.width(org_str), fm.height(), tfa, org_str);
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
                int fmwidth = fm.width(label);
                paint.save();
                paint.rotate(-45);
                paint.drawText(lx - fmwidth, ly - fmheight_div_2, fmwidth, fmheight, Qt::AlignRight | Qt::AlignTop, label);
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
                    if(sit.value() != 0.0 && _setStyle[sit.key()].bar == true) {
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
                    if(_setStyle[sit.key()].line == true) {
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
                }
                last_map = this_map;
            }
            if(drawPoints() == true) {
                sit = ref.second;
                while(sit.hasNext())
                {
                    sit.next();
                    if(_setStyle[sit.key()].point == true) {
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

void tGraph::repaint() {}




void renderGraph(QPainter & paint, const QRect & rect, ORGraphData & gData, XSqlQuery * query, const QMap<QString, QColor> & _colorMap) {
    int dpi = paint.device()->logicalDpiX();

    QFont fnt;

    tGraph graph(rect);
    fnt = gData.font;
    fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
    graph.setFont(fnt);
    if(gData.title.string.length() > 0) {
        graph.setTitle(gData.title.string);
        if(gData.title.font_defined) {
            fnt = gData.title.font;
            fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
            graph.setTitleFont(&fnt);
        }
    }

    QString lbl_clmn = gData.dataaxis.column;
    if(lbl_clmn.length() > 0 && gData.dataaxis.font_defined) {
        fnt = gData.dataaxis.font;
        fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
        graph.setDataFont(&fnt);
    }
    if(gData.dataaxis.title.string.length() > 0) {
        graph.setDataLabel(gData.dataaxis.title.string);
        if(gData.dataaxis.title.font_defined) {
            fnt = gData.dataaxis.title.font;
            fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
            graph.setDataLabelFont(&fnt);
        }
    }

    graph.setAutoMinMax(gData.valueaxis.autominmax);
    graph.setMinValue(gData.valueaxis.min);
    graph.setMaxValue(gData.valueaxis.max);
    if(gData.valueaxis.font_defined) {
        fnt = gData.valueaxis.font;
        fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
        graph.setValueFont(&fnt);
    }
    if(gData.valueaxis.title.string.length() > 0) {
        graph.setValueLabel(gData.valueaxis.title.string);
        if(gData.valueaxis.title.font_defined) {
            fnt = gData.valueaxis.title.font;
            fnt.setPointSizeF((100.0/dpi)*fnt.pointSize());
            graph.setValueLabelFont(&fnt);
        }
    }

    graph.setHPadding(gData.padding.horizontal);
    graph.setVPadding(gData.padding.vertical);

    // setup the sets/series
    int snum = 0;
    ORSeriesData * sd = 0;
    for(snum = 0; snum < gData.series.count(); snum++) {
        sd = gData.series.at(snum);
        if(sd) {
            graph.setSetColor(snum, _colorMap[sd->color]);
            graph.setSetStyle(snum, sd->style);
        }
    }

    // populate the data
    if(query->first()) {
        do {
            if(lbl_clmn.length() > 0) {
                graph.setReferenceLabel(query->at(), query->value(lbl_clmn).toString());
            }
            for(snum = 0; snum < gData.series.count(); snum++) {
                sd = gData.series.at(snum);
                if(sd) {
                    graph.setSetValue(query->at(), snum, query->value(sd->column).toDouble());
                }
            }
        } while(query->next());
    }

    // draw the graph
    graph.draw(paint);
}
