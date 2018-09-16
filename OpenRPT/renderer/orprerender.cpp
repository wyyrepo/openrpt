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

#include <QtGui>

#include "orprerender.h"
#include "renderobjects.h"
#include "orutils.h"
#include "graph.h"
#include "crosstab.h"
#include "reportprinter.h"
#include "textelementsplitter.h"

#include <QPrinter>
#include <QFontMetrics>
#include <QPainter>

#include <xsqlquery.h>
#include <parameter.h>
#include <parsexmlutils.h>
#include <builtinformatfunctions.h>
#include <builtinSqlFunctions.h>
#include <pagesizeinfo.h>
#include <labelsizeinfo.h>
#include <quuencode.h>

//
// ORPreRenderPrivate
// This class is the private class that houses all the internal
// variables so we can provide a cleaner interface to the user
// without presenting to them things that they don't need to see
// and may change over time.
//
class ORPreRenderPrivate {
  public:
    ORPreRenderPrivate();
    virtual ~ORPreRenderPrivate();

    bool _valid;
    QDomDocument _docReport;
    ParameterList _lstParameters;

    QSqlDatabase _database;
    ORODocument* _document;
    OROPage*     _page;
    ORReportData* _reportData;

    qreal _yOffset;      // how far down the current page are we in inches. That way we can use dpi.
    qreal _topMargin;    // value stored in the correct units
    qreal _bottomMargin; // -- same as above --
    qreal _leftMargin;   // -- same as above --
    qreal _rightMargin;  // -- same as above --
    qreal _maxHeight;    // -- same as above --
    qreal _maxWidth;     // -- same as above --
    int _pageCounter;    // what page are we currently on?

    QList<orQuery*> _lstQueries;
    QMap<QString, QColor> _colorMap;
    QList<OROTextBox*> _postProcText;

    // data for the watermark feature
    bool    _wmStatic;   // is this watermark static text or is the data
                         // pulled from a query source
    QString _wmText;     // the text that is to be rendered as a watermark
    ORDataData _wmData;  // the dynamic source for the watermark text
    QFont   _wmFont;     // the font to use when rendering the _wmText value
                         // on the background of the page. For purposes of
                         // fit, pointSize() of font is ignored and the largest
                         // point size is used that will still allow the entire
                         // text to fit on the page.
    unsigned char _wmOpacity; // a value from 0..255
                              // with 0   being not visible (white)
                              // and  255 being fully opaque (black)

    bool    _bgStatic;   // is this image static or pulled from a datasource
    QImage  _bgImage;    // the image that is to be rendered as a background
    ORDataData _bgData;  // the dynamic source for the background image
    QRectF  _bgRect;     // area of the page that we print background to
    unsigned char _bgOpacity; // a value from 0..255 : see _wmOpacity
    int     _bgAlign;    // Alignment of background within _bgRect
    bool    _bgScale;    // scale image to fit in _bgRect?
    Qt::AspectRatioMode _bgScaleMode; // how do we scale the image?

    void renderBackground(OROPage *);
    void renderWatermark(OROPage *);

    QMap<ORDataData,double> _subtotPageCheckPoints;
    QMap<ORDataData,double> * _subtotContextMap;
    ORDetailSectionData * _subtotContextDetail;
    bool _subtotContextPageFooter;

    bool populateData(const ORDataData &, orData &);
    bool populateData(const ORDataData &, orData &, const QString&);
    bool populateColorData(const ORDataData&, orData&);

    orQuery* getQuerySource(const QString &);

    void createNewPage();
    qreal finishCurPage(bool = false);
    qreal finishCurPageSize(bool = false);
    qreal maxDetailSectionY();

    void renderDetailSection(ORDetailSectionData &);
    qreal renderSection(const ORSectionData &);
    qreal renderTextElements(QList<ORObject*> elemList, qreal sectionHeight);
    void addTextPrimitive(ORObject *element, QPointF pos, QSizeF size, int align, QString text, QFont font = QFont(), QString color = QString());
    QString evaluateField(ORFieldData* f, QString* outColorStr);
    qreal renderSectionSize(const ORSectionData &, bool = false);

    // Calculate the remaining space on the page after printing the footers and applying the margins
    qreal calculateRemainingPageSize(bool lastPage = false);

    double getNearestSubTotalCheckPoint(const ORDataData &);

    XSqlQuery *_detailQuery;
    ReportPrinter::type             _printerType;
    QList<QPair<QString,QString> >  _printerParams;
};

ORPreRenderPrivate::ORPreRenderPrivate()
{
  _valid = false;
  _reportData = 0;
  _document = 0;
  _page = 0;
  _yOffset = 0.0;
  _topMargin = _bottomMargin = 0.0;
  _leftMargin = _rightMargin = 0.0;
  _pageCounter = 0;
  _maxHeight = _maxWidth = 0.0;

  _wmStatic = true;
  _wmText = QString::null;
  _wmData.query = QString::null;
  _wmData.column = QString::null;
  _wmFont = QFont("Helvetic");
  _wmOpacity = 25;

  _bgStatic = true;
  _bgImage = QImage();
  _bgData.query = QString::null;
  _bgData.column = QString::null;
  _bgOpacity = 25;
  _bgAlign = Qt::AlignLeft | Qt::AlignTop;
  _bgScale = false;
  _bgScaleMode = Qt::IgnoreAspectRatio;

  _subtotContextMap = 0;
  _subtotContextDetail = 0;
  _subtotContextPageFooter = false;

  _detailQuery = 0;
  _printerType = ReportPrinter::Standard;
}

ORPreRenderPrivate::~ORPreRenderPrivate()
{
  if(_reportData != 0)
  {
    delete _reportData;
    _reportData = 0;
  }

  while(!_lstQueries.isEmpty())
    delete _lstQueries.takeFirst();
  _postProcText.clear();
}

bool ORPreRenderPrivate::populateData(const ORDataData & dataSource, orData &dataTarget)
{
  return populateData(dataSource, dataTarget, dataSource.column);
}

bool ORPreRenderPrivate::populateData(const ORDataData & dataSource, orData &dataTarget, const QString& column)
{
  for(int queryCursor = 0; queryCursor < _lstQueries.count(); queryCursor++)
  {
    if(_lstQueries.at(queryCursor)->getName() == dataSource.query)
    {
      dataTarget.setQuery(_lstQueries.at(queryCursor));
      dataTarget.setField(column);
      return true;
    }
  }

  return false;
}

// Allow for query-based color information
bool ORPreRenderPrivate::populateColorData(const ORDataData& dataSource, orData& dataTarget)
{
  QString designator = "qtforegroundrole";
  QString column = dataSource.column + "_" + designator;

  return populateData(dataSource, dataTarget, column);
}

orQuery* ORPreRenderPrivate::getQuerySource(const QString & qstrQueryName)
{
  static orQuery emptyQuery;

  for(int queryCursor = 0; queryCursor < _lstQueries.count(); queryCursor++)
  {
    if (_lstQueries.at(queryCursor)->getName() == qstrQueryName)
      return _lstQueries.at(queryCursor);
  }

  QString docName = _reportData ? _reportData->name : "";
  qWarning() << "No Query Source with name" << qstrQueryName << "in" << docName;
  return &emptyQuery;
}

void ORPreRenderPrivate::renderBackground(OROPage * p)
{
  bool staticBg = (_bgStatic && !_bgImage.isNull());
  bool dynamicBg = (!_bgStatic && !_bgData.query.isEmpty() && !_bgData.column.isEmpty());
  bool doBg = (_bgOpacity != 0 && _bgRect.isValid() && (staticBg || dynamicBg));

  if(_reportData != 0 && doBg) {
    QImage img = _bgImage;
    if(dynamicBg) {
      orData dataThis;
      populateData(_bgData, dataThis);
      QString uudata = dataThis.getValue();

      QByteArray imgdata = QUUDecode(uudata);
      img = QImage::fromData(imgdata);
    }
    if(img.isNull())
      return;

    p->setBackgroundImage(img);
    p->setBackgroundScale(_bgScale);
    p->setBackgroundScaleMode(_bgScaleMode);
    p->setBackgroundAlign(_bgAlign);
    p->setBackgroundOpacity(_bgOpacity);
    p->setBackgroundPosition(_bgRect.topLeft());
    p->setBackgroundSize(_bgRect.size());
  }
}

void ORPreRenderPrivate::renderWatermark(OROPage * p)
{
  if(!p || _wmOpacity == 0)
    return;

  QString wmText = _wmText;
  if(!_wmStatic && !_wmData.query.isEmpty() && !_wmData.column.isEmpty())
  {
    if(_wmData.query == "Context Query" && _wmData.column == "page_number")
      wmText = QString("%1").arg(_pageCounter);
    else if(_wmData.query == "Context Query" && _wmData.column == "report_name")
      wmText = _reportData->name;
    else if(_wmData.query == "Context Query" && _wmData.column == "report_title")
      wmText = _reportData->title;
    else if(_wmData.query == "Context Query" && _wmData.column == "report_description")
      wmText = _reportData->description;
    else
    {
      orData dataThis;
      populateData(_wmData, dataThis);
      wmText = dataThis.getValue();
    }
  }

  if(wmText.isEmpty())
    return;

  p->setWatermarkText(wmText);
  p->setWatermarkFont(_wmFont);
  p->setWatermarkOpacity(_wmOpacity);
}

void ORPreRenderPrivate::createNewPage()
{
  if(_pageCounter > 0)
    finishCurPage();

  QMapIterator<ORDataData,double> it(_subtotPageCheckPoints);
  while(it.hasNext())
  {
    it.next();
    double d = 0.0;
    ORDataData data = it.key();
    XSqlQuery * xqry = getQuerySource(data.query)->getQuery();
    if(xqry)
      d = xqry->getFieldTotal(data.column);
    _subtotPageCheckPoints.insert(it.key(), d);
  }

  _pageCounter++;

  _page = new OROPage(0);
  _document->addPage(_page);
  renderBackground(_page);
  renderWatermark(_page);

  bool lastPage = false;

  _yOffset = _topMargin;

  if(_pageCounter == 1 && _reportData->pghead_first != 0)
    renderSection(*(_reportData->pghead_first));
  else if(lastPage == true && _reportData->pghead_last != 0)
    renderSection(*(_reportData->pghead_last));
  else if((_pageCounter % 2) == 1 && _reportData->pghead_odd != 0)
    renderSection(*(_reportData->pghead_odd));
  else if((_pageCounter % 2) == 0 && _reportData->pghead_even != 0)
    renderSection(*(_reportData->pghead_even));
  else if(_reportData->pghead_any != 0)
    renderSection(*(_reportData->pghead_any));
}

qreal ORPreRenderPrivate::finishCurPageSize(bool lastPage)
{
  qreal retval = 0.0;

  _subtotContextPageFooter = true;
  if(lastPage && _reportData->pgfoot_last != 0)
      retval = renderSectionSize(*(_reportData->pgfoot_last));
  else if(_pageCounter == 1 && _reportData->pgfoot_first)
      retval = renderSectionSize(*(_reportData->pgfoot_first));
  else if((_pageCounter % 2) == 1 && _reportData->pgfoot_odd)
      retval = renderSectionSize(*(_reportData->pgfoot_odd));
  else if((_pageCounter % 2) == 0 && _reportData->pgfoot_even)
      retval = renderSectionSize(*(_reportData->pgfoot_even));
  else if(_reportData->pgfoot_any != 0)
      retval = renderSectionSize(*(_reportData->pgfoot_any));
  _subtotContextPageFooter = false;

  return retval;
}

qreal ORPreRenderPrivate::maxDetailSectionY()
{
    int l = (_detailQuery ? _detailQuery->at() : 0);
    int qs = (_detailQuery ? _detailQuery->size() : 1);
    qreal sizeLimit = _maxHeight - _bottomMargin;
    if(!_subtotContextPageFooter)
        sizeLimit = _maxHeight - _bottomMargin - finishCurPageSize((l+1 == qs));

    return sizeLimit;
}

//
// calculateRemainingPageSize
//   Calculate the remaining space on the page after printing the fo oters and
//   applying the margins
//
// ToDo: How to handle last page when this function is used to calculate a
//       page break
// Note: Preassumption is that this function is called by detailed sections.
//       Thus footer sections are also calculated.
// Note: There is an order in here. So only one footer per page.
//
qreal ORPreRenderPrivate::calculateRemainingPageSize(bool lastPage)
{
  qreal retval = _yOffset;

  // Current position
  qreal total(_yOffset);

  // Add footers
  _subtotContextPageFooter = true;
  if(lastPage && _reportData->pgfoot_last != 0)
  {
    total += renderSectionSize(*(_reportData->pgfoot_last));
  }
  else if(_pageCounter == 1 && _reportData->pgfoot_first)
  {
    total += renderSectionSize(*(_reportData->pgfoot_first));
  }
  else if((_pageCounter % 2) == 1 && _reportData->pgfoot_odd)
  {
    total += renderSectionSize(*(_reportData->pgfoot_odd));
  }
  else if((_pageCounter % 2) == 0 && _reportData->pgfoot_even)
  {
    total += renderSectionSize(*(_reportData->pgfoot_even));
  }
  else if(_reportData->pgfoot_any != 0)
  {
    total += renderSectionSize(*(_reportData->pgfoot_any));
  }
  _subtotContextPageFooter = false;

  // Add bottom margin
  total += _bottomMargin;

  // Calculate space left
  retval = _maxHeight - total;

  // Prevent returning negative size as that does not make sense
  return ((retval<0.0) ? 0.0 : retval);
}

qreal ORPreRenderPrivate::finishCurPage(bool lastPage)
{
  qreal offset = _maxHeight - _bottomMargin;
  qreal retval = 0.0;

  _subtotContextPageFooter = true;
  if(lastPage && _reportData->pgfoot_last != 0)
  {
    _yOffset = offset - renderSectionSize(*(_reportData->pgfoot_last));
    retval = renderSection(*(_reportData->pgfoot_last));
  }
  else if(_pageCounter == 1 && _reportData->pgfoot_first)
  {
    _yOffset = offset - renderSectionSize(*(_reportData->pgfoot_first));
    retval = renderSection(*(_reportData->pgfoot_first));
  }
  else if((_pageCounter % 2) == 1 && _reportData->pgfoot_odd)
  {
    _yOffset = offset - renderSectionSize(*(_reportData->pgfoot_odd));
    retval = renderSection(*(_reportData->pgfoot_odd));
  }
  else if((_pageCounter % 2) == 0 && _reportData->pgfoot_even)
  {
    _yOffset = offset - renderSectionSize(*(_reportData->pgfoot_even));
    retval = renderSection(*(_reportData->pgfoot_even));
  }
  else if(_reportData->pgfoot_any != 0)
  {
    _yOffset = offset - renderSectionSize(*(_reportData->pgfoot_any));
    retval = renderSection(*(_reportData->pgfoot_any));
  }
  _subtotContextPageFooter = false;

  return retval;
}

//
// RenderDetailSection
//   Render a detailed section. A detailed section can contain groups. These
//   have to be rendered too.
// 
//   In this code a query is connected to a detail section.
//   TODO: add data limit to query
//   The data of a section can be one image. The section should get grow and
//   shrink parameters.
//
//   This can be used as an example for wrapping of sections over pages.
//
void ORPreRenderPrivate::renderDetailSection(ORDetailSectionData & detailData)
{
  if(detailData.detail != 0)
  {
    orQuery *orqThis = getQuerySource(detailData.key.query);
    XSqlQuery *query;

    _subtotContextDetail = &detailData;

    if (orqThis->getQuery() && ((query = orqThis->getQuery())->size()))
    {
      bool hasRecords = query->first();
	  if(!hasRecords) {
	    return; // No records => don't print the section
	  }

      _detailQuery = query;
      QStringList keys;
      QStringList keyValues;
      bool    status;
      int i = 0, pos = 0, cnt = 0;
      ORDetailGroupSectionData * grp = 0;

      // Go through the groups
      for(i = 0; i < (int)detailData.groupList.count(); i++)
      {
        cnt++;
        grp = detailData.groupList[i];
        QMapIterator<ORDataData,double> it(grp->_subtotCheckPoints);
        while(it.hasNext())
        {
          it.next();
          grp->_subtotCheckPoints.insert(it.key(), 0.0);
        }
        keys.append(grp->column);
        if(!keys[i].isEmpty()) keyValues.append(query->value(keys[i]).toString());
        else keyValues.append(QString());
        _subtotContextMap = &(grp->_subtotCheckPoints);
        if(grp->head)
          renderSection(*(grp->head));
        _subtotContextMap = 0;
      }

      int rowCnt = 0;
      do
      {
        rowCnt++;
        // Do we need to go to the next page for this section
        int l = query->at();
        if ( renderSectionSize(*(detailData.detail), true) + finishCurPageSize((l+1 == query->size())) + _bottomMargin + _yOffset >= _maxHeight)
        {
          if(l > 0)
            query->prev();
          createNewPage();
          if(l > 0)
            query->next();
        }

        // Render this section
        renderSection(*(detailData.detail));


        status = query->next();
        if (status == true && keys.count() > 0)
        {
          // check to see where it is we need to start
          pos = -1; // if it's still -1 by the time we are done then no keyValues changed
          for(i = 0; i < keys.count(); i++)
          {
            if(keyValues[i] != query->value(keys[i]).toString())
            {
              pos = i;
              break;
            }
          }

          // don't bother if nothing has changed
          if(pos != -1)
          {
            // roll back the query and go ahead if all is good
            status = query->prev();
            if(status == true)
            {
              // print the footers as needed
              // any changes made in this for loop need to be duplicated
              // below where the footers are finished.
              bool do_break = false;
              for(i = cnt - 1; i >= pos; i--)
              {
                if(do_break)
                  createNewPage();
                do_break = false;
                grp = detailData.groupList[i];
                _subtotContextMap = &(grp->_subtotCheckPoints);
                if(grp->foot)
                {
                  if ( renderSectionSize(*(grp->foot)) + finishCurPageSize() + _bottomMargin + _yOffset >= _maxHeight)
                    createNewPage();
                  renderSection(*(grp->foot));
                }
                _subtotContextMap = 0;
                // reset the sub-total values for this group
                QMapIterator<ORDataData,double> it(grp->_subtotCheckPoints);
                while(it.hasNext())
                {
                  it.next();
                  double d = 0.0;
                  ORDataData data = it.key();
                  XSqlQuery * xqry = getQuerySource(data.query)->getQuery();
                  if(xqry)
                    d = xqry->getFieldTotal(data.column);
                  grp->_subtotCheckPoints.insert(it.key(), d);
                }
                if(ORDetailGroupSectionData::BreakAfterGroupFoot == grp->pagebreak)
                  do_break = true;
              }
              // step ahead to where we should be and print the needed headers
              // if all is good
              status = query->next();
              if(do_break)
                createNewPage();
              if(status == true)
              {
                for(i = pos; i < cnt; i++)
                {
                  grp = detailData.groupList[i];
                  _subtotContextMap = &(grp->_subtotCheckPoints);
                  if(grp->head)
                  {
                    if ( renderSectionSize(*(grp->head)) + finishCurPageSize() + _bottomMargin + _yOffset >= _maxHeight)
                    {
                      query->prev();
                      createNewPage();
                      query->next();
                    }
                    renderSection(*(grp->head));
                  }
                  _subtotContextMap = 0;
                  if(!keys[i].isEmpty())
                    keyValues[i] = query->value(keys[i]).toString();
                }
              }
            }
          }
        }
      } while (status == true);

	  if(query->at()==QSql::AfterLastRow)
	  {
		query->prev(); // move back to a valid record -- this keeps the records accessible and the (sub)totals still valid as well
              if(!query->isValid())
              {
                  query->first();
                  for(int i = 1; i < rowCnt; i++)
                      query->next();
              }
	  }

      if(keys.size() > 0 && query->isValid ())
      {
        // finish footers
        // duplicated changes from above here
        for(i = cnt - 1; i >= 0; i--)
        {
          grp = detailData.groupList[i];
          _subtotContextMap = &(grp->_subtotCheckPoints);
          if(grp->foot)
          {
            if ( renderSectionSize(*(grp->foot)) + finishCurPageSize() + _bottomMargin + _yOffset >= _maxHeight)
              createNewPage();
            renderSection(*(grp->foot));
          }
          _subtotContextMap = 0;
          // reset the sub-total values for this group
          QMapIterator<ORDataData,double> it(grp->_subtotCheckPoints);
          while(it.hasNext())
          {
            it.next();
            double d = 0.0;
            ORDataData data = it.key();
            XSqlQuery * xqry = getQuerySource(data.query)->getQuery();
            if(xqry)
              d = xqry->getFieldTotal(data.column);
            grp->_subtotCheckPoints.insert(it.key(), d);
          }
        }
      }
    }
    _detailQuery = 0;
    _subtotContextDetail = 0;
    if(ORDetailSectionData::BreakAtEnd == detailData.pagebreak)
      createNewPage();
  }
}

qreal ORPreRenderPrivate::renderSectionSize(const ORSectionData & sectionData, bool ignoreTextArea)
{
  qreal sectionHeight = sectionData.height / 100.0;

  if(sectionData.objects.count() == 0 || ignoreTextArea)
    return sectionHeight;

  for(int it = 0; it < sectionData.objects.size(); ++it)
  {
    ORObject * element = sectionData.objects.at(it);
    if (element->isText())
    {
      orData       dataThis;
      ORTextData * t = element->toText();

      populateData(t->data, dataThis);

      TextElementSplitter textSplitter(element, dataThis.getValue(), _leftMargin, _yOffset);

      while (!textSplitter.endOfText())
      {
        textSplitter.nextLine();
      }

      if (textSplitter.textBottomRelativePos() > sectionHeight)
      {
        sectionHeight = textSplitter.textBottomRelativePos();
      }
    }
  }

  return sectionHeight;
}

qreal ORPreRenderPrivate::renderSection(const ORSectionData & sectionData)
{
  qreal intHeight = sectionData.height / 100.0;

  if(sectionData.objects.count() == 0)
    return 0;

  ORObject * elemThis;
  QList<ORObject*> textelem;
  bool newPageRequested = false;

  bool recallMask = !ReportPrinter::getRecallMask(_printerParams).isEmpty();
  bool storeMask = !ReportPrinter::getStoreMask(_printerParams).isEmpty();

  for(int it = 0; it < sectionData.objects.size(); ++it)
  {
    elemThis = sectionData.objects.at(it);

    if((storeMask && !elemThis->isStatic()) || (recallMask && elemThis->isStatic()))
    {
      continue; // ignore element
    }

    if (elemThis->isLabel())
    {
      ORLabelData * l = elemThis->toLabel();
      QPointF pos = l->rect.topLeft();
      QSizeF size = l->rect.size();
      pos /= 100.0;
      pos += QPointF(_leftMargin, _yOffset);
      size /= 100.0;

      addTextPrimitive(elemThis, pos, size, l->align, qApp->translate(_reportData->name.toUtf8().data(), l->string.toUtf8().data(), 0, QCoreApplication::UnicodeUTF8), l->font);
    }
    else if (elemThis->isField())
    {
        ORFieldData *f = elemThis->toField();

        int nbOfLines = f->lines;
        int nbOfCol = f->columns;
        qreal xSpacing = f->xSpacing;
        qreal ySpacing = f->ySpacing;
        bool triggerPageBreak = f->triggerPageBreak;
        bool leftToRight = f->leftToRight;

        QPointF pos = f->rect.topLeft();
        QSizeF size = f->rect.size();
        pos /= 100.0;
        pos += QPointF(_leftMargin, _yOffset);
        size /= 100.0;

        bool hasNext = false;
        qreal startX = pos.x();
        qreal startY = pos.y();

        QList < QPair<int,int> > cellLst;

        if(leftToRight) {
          for (int col = 0; col < nbOfCol; col++) {
            for (int line = 0; line < nbOfLines; line++) {
              cellLst.append(qMakePair(col,line));
            }
          }
        }
        else {
          for (int line = 0; line < nbOfLines; line++) {
            for (int col = 0; col < nbOfCol; col++) {
              cellLst.append(qMakePair(col,line));
            }
          }
        }

        QPair<int,int> cell;

        foreach(cell, cellLst) {

          qreal x = startX +  cell.first*(size.width() + xSpacing);
          qreal y = startY + cell.second*(size.height() + ySpacing);
		  XSqlQuery * xqry = getQuerySource(f->data.query)->getQuery();
          if (!xqry)
            break;

          if (xqry->isValid() || (nbOfCol == 1 && nbOfLines == 1))
          {
            QString colorStr = QString::null;
            QString text = evaluateField(elemThis->toField(), &colorStr);
            addTextPrimitive(elemThis, QPointF(x, y), size, f->align, text, f->font, colorStr);
          }
          else 
            break;

          if(nbOfCol > 1 || nbOfLines > 1 || triggerPageBreak) {
            hasNext = xqry->next();
          }

          if(!hasNext) {
            break;
          }
        }

        if(!newPageRequested) {
          newPageRequested = hasNext && triggerPageBreak;
        }

    }
    else if (elemThis->isText())
    {
      textelem.append(elemThis);
    }
    else if (elemThis->isLine())
    {
      ORLineData * l = elemThis->toLine();
      OROLine * ln = new OROLine(elemThis);
      ln->setStartPoint(QPointF((l->xStart / 100.0) + _leftMargin, (l->yStart / 100.0) + _yOffset));
      ln->setEndPoint(QPointF((l->xEnd / 100.0) + _leftMargin, (l->yEnd / 100.0) + _yOffset));
      ln->setRotation(l->rotation());
      _page->addPrimitive(ln);
    }
    else if (elemThis->isRect())
    {
      ORRectData * r = elemThis->toRect();
      ORORect * rn = new ORORect(elemThis);
      rn->setRect(QRectF((r->x / 100.0) + _leftMargin, (r->y / 100.0) + _yOffset, r->width / 100.0, r->height / 100.0));
      rn->setRotation(r->rotation());
      _page->addPrimitive(rn);
    }
    else if (elemThis->isBarcode())
    {
      ORBarcodeData * bc = elemThis->toBarcode();
      OROBarcode* bcPrimitive = new OROBarcode(elemThis);

      QPointF pos = bc->rect.topLeft();
      QSizeF size = bc->rect.size();
      pos /= 100.0;
      pos += QPointF(_leftMargin, _yOffset);
      size /= 100.0;

      orData       dataThis;
      populateData(bc->data, dataThis);

      bcPrimitive->setPosition(pos);
      bcPrimitive->setSize(size);
      bcPrimitive->setData(dataThis.getValue());
      bcPrimitive->setFormat(bc->format);
      bcPrimitive->setNarrowBarWidth(bc->narrowBarWidth);
      bcPrimitive->setAlign(bc->align);
      bcPrimitive->setRotation(bc->rotation());
      _page->addPrimitive(bcPrimitive);
    }
    else if (elemThis->isImage())
    {
      ORImageData * im = elemThis->toImage();
      QString uudata = im->inline_data;
      QByteArray imgdata;

      if(uudata == QString::null)
      {
        orData dataThis;
        populateData(im->data, dataThis);

        // need to determine the type of the column specified by this image
        // if it is coming from the file table, it will be bytea/QByteArray
        // if it is coming from the image table, it will be text/QString encoded with UUEncode
        if (dataThis.getType() == QMetaType::QByteArray)
        {
          QByteArray bytes = dataThis.getByteValue();
          // its a byte array, first check that someone didn't stick uuencoded bytes in there
          if (bytes.startsWith("begin"))
          {
            uudata = QString::fromLatin1(bytes.constData());
            imgdata = QUUDecode(uudata);
          }
          else
          {
            // since it is already bytes we can just set use them directly
            imgdata = bytes;
          }
        }
        else
        {
          // uuencoded data, get the encoded string and uudecode it into bytes
          uudata = dataThis.getValue();
          imgdata = QUUDecode(uudata);
        }
      }
      else
      {
        // decode the provided inline data
        imgdata = QUUDecode(uudata);
      }

      QImage img;

      if(imgdata.isEmpty()) {
          // not uuencoded data, so it should be a file name
          bool ok = img.load(uudata.trimmed());
          if(!ok) {
              qDebug("Invalid image data");
          }
      }
      else {
          img = QImage::fromData(imgdata);
      }

      OROImage * id = new OROImage(elemThis);
      id->setImage(img);
      if(im->mode == "stretch")
      {
        id->setScaled(true);
        id->setAspectRatioMode(Qt::KeepAspectRatio);
        id->setTransformationMode(Qt::SmoothTransformation);
      }
      QPointF pos = im->rect.topLeft();
      QSizeF size = im->rect.size();
      pos /= 100.0;
      pos += QPointF(_leftMargin, _yOffset);
      size /= 100.0;
      id->setPosition(pos);
      id->setSize(size);
	  id->setRotation(im->rotation());
      _page->addPrimitive(id);
    }
    else if (elemThis->isGraph())
    {
      ORGraphData * gData = elemThis->toGraph();

      QPointF pos = gData->rect.topLeft();
      QSizeF size = gData->rect.size();
      pos /= 100.0;
      pos += QPointF(_leftMargin, _yOffset);
      size /= 100.0;

      QRect rect = QRect(QPoint(0, 0), gData->rect.size());

// What we are doing here is we are creating an image assuming the original
// 100dpi to render the graph to. All the original code is usable this way
// as we just need to setup a painter for the image and pass that along to
// the graph drawing code.
      QImage gImage(rect.size(), QImage::Format_RGB32);
      gImage.setDotsPerMeterX(3937); // should be 100dpi
      gImage.setDotsPerMeterY(3937); // should be 100dpi
      QPainter gPainter;
      if(gPainter.begin(&gImage))
      {
        gPainter.fillRect(rect, QColor(Qt::white));
        gPainter.setPen(Qt::black);
        renderGraph(gPainter, rect, *gData, getQuerySource(gData->data.query)->getQuery(), _colorMap);
        gPainter.end();

        OROImage * id = new OROImage(elemThis);
        id->setImage(gImage);
        id->setPosition(pos);
        id->setSize(size);
        id->setScaled(true);
        id->setAspectRatioMode(Qt::KeepAspectRatio);
        id->setTransformationMode(Qt::SmoothTransformation);
		id->setRotation(gData->rotation());
        _page->addPrimitive(id);

      }
    }
    else if (elemThis->isCrossTab())
    {
      ORCrossTabData * ctData = elemThis->toCrossTab();

      CrossTab crossTab(ctData->font);

      // Initialise
      crossTab.Initialize(*ctData, _colorMap);
      // Populate storage from query
      orQuery* ctq = getQuerySource(ctData->data.query);
      if(ctq)
      {
        // We calculate our own height and correct the parameters
        intHeight = 0;

        crossTab.PopulateFromQuery(ctq->getQuery());
        // Calculate widths and heights
        crossTab.CalculateCrossTabMeasurements();

        do
        {
          // Inform of iteration
          crossTab.Iterate();

          // Calculate position and size on page
          // CrossTab table will always start on the top left of the band
          QPointF pos(0.0,0.0);
          pos /= 100.0;
          pos += QPointF(_leftMargin, _yOffset);

          // What we are doing here is we are creating an image assuming the original
          // 100dpi to render the graph to. All the original code is usable this way
          // as we just need to setup a painter for the image and pass that along to
          // the graph drawing code.

          // TODO: parameter lastPage
          //       lastpage: 1. last detailed section in report list
          //                 2. last element in section list
          //                 3. last query in element if multiquery
          qreal emptyHeight = calculateRemainingPageSize(true);
          qreal emptyWidth  = _maxWidth - _leftMargin - _rightMargin;

          // Convert to pixels
          emptyHeight *= 100;
          emptyWidth  *= 100;

          // If the emptyHeight is smaller than the table it will not be the last
          // page. Recalculate.
  
          // ToDo: Calculate remaining table size
          QRect tableSize;
          crossTab.CalculateTableSize (tableSize);
          if (emptyHeight < tableSize.height())
          {
            emptyHeight = calculateRemainingPageSize(false);
            emptyHeight *= 100;
          }

          // This rectangle gives the maximum amount of space on this page
          QRect rect = QRect(QPoint(0, 0), QSize(emptyWidth,emptyHeight));
          crossTab.CalculateSize(rect);
          crossTab.SetRect(rect);

          QImage gImage(rect.size(), QImage::Format_RGB32);
          gImage.setDotsPerMeterX(3937); // should be 100dpi
          gImage.setDotsPerMeterY(3937); // should be 100dpi
          QPainter gPainter;
          if(gPainter.begin(&gImage))
          {
            gPainter.fillRect(rect, QColor(Qt::white));
            gPainter.setPen(Qt::black);
            crossTab.Draw(gPainter);
            gPainter.end();

            OROImage * id = new OROImage(elemThis);
            id->setImage(gImage);
            id->setPosition(pos);
            id->setSize(gImage.size()/100);
            id->setScaled(true);
            id->setAspectRatioMode(Qt::KeepAspectRatio);
            id->setTransformationMode(Qt::SmoothTransformation);
            _page->addPrimitive(id);
            _yOffset += (rect.height ()/100.0); //_yOffset in inches
          }

          // Situation of wrapping of table
          if (!crossTab.AllDataRendered())
          {
            // Calculate remaining space on page
            qreal emptyHeight = calculateRemainingPageSize(true);
            emptyHeight *= 100;
        
            // Calculate minimal size needed for the table
            int nextRowSize(0);
            crossTab.CalculateNextRowSize (nextRowSize);

            // Decision for new page
            if ((0 < nextRowSize) && (nextRowSize < emptyHeight))
            {
              // Add to current page
            }
            else
            {
              // No space left, new page
              createNewPage();
            }
          }
        }
        while(!crossTab.AllDataRendered());
      }
    }
    else
    {
      qDebug("Encountered an unknown element while rendering a section.");
    }
  }

  intHeight = renderTextElements(textelem, intHeight);

  _yOffset += intHeight;

  if(newPageRequested) {
    createNewPage();
  }

  return intHeight;
}

qreal ORPreRenderPrivate::renderTextElements(QList<ORObject*> elemList, qreal sectionHeight)
{
    QList<TextElementSplitter> splitters;

    foreach (ORObject *elem, elemList)
    {
        orData       dataThis;
        ORTextData * t = elem->toText();

        populateData(t->data, dataThis);

        splitters.append(TextElementSplitter(elem, dataThis.getValue(),
                                                 _leftMargin, _yOffset, maxDetailSectionY()));
    }

    while (!splitters.isEmpty())
    {
        bool newPageRequested = false;

        for (int i = 0; i < splitters.size(); i++)
        {
            TextElementSplitter *splitter = &(splitters[i]);

            while (!splitter->endOfText() && !splitter->endOfPage())
            {
                splitter->nextLine();

                addTextPrimitive(splitter->element(),
                                 splitter->currentLineRect().topLeft(),
                                 splitter->currentLineRect().size(),
                                 splitter->element()->align,
                                 splitter->currentLine(),
                                 splitter->element()->font);

            }

            if (splitter->textBottomRelativePos() > sectionHeight)
                sectionHeight = splitter->textBottomRelativePos();

            if(splitter->endOfText())
            {
                splitters.removeAt(i);
                i--;
            }
            else if (splitter->endOfPage())
            {
                newPageRequested = true;
            }
        }

        if(newPageRequested && !_subtotContextPageFooter)
        {
            int l = _detailQuery ? _detailQuery->at() : 0;
            if(l > 0)
              _detailQuery->prev();
            createNewPage();
            if(l > 0)
              _detailQuery->next();

            for (int i = 0; i < splitters.size(); i++)
            {
                splitters[i].newPage(_yOffset);
            }

            sectionHeight = 0;
        }
    }

    return sectionHeight;
}

void ORPreRenderPrivate::addTextPrimitive(ORObject *element, QPointF pos, QSizeF size, int align, QString text, QFont font, QString colorStr)
{
  OROTextBox * tb = new OROTextBox(element);
  tb->setPosition(pos);
  tb->setSize(size);
  tb->setFont(font);
  tb->setText(text);
  tb->setFlags(align);
  tb->setRotation(element->rotation());

  if (colorStr.length() > 0)
  {
    tb->setPen(QPen(QColor(colorStr)));
  }

  _page->addPrimitive(tb);

  if(text == "page_count") {
    _postProcText.append(tb);
  }
}


QString ORPreRenderPrivate::evaluateField(ORFieldData* f, QString* outColorStr)
{
    orData       dataThis;

    QString str = QString::null;
    double d_val = 0;
    bool isFloat = false;

    if(f->trackTotal) {
        XSqlQuery * xqry = getQuerySource(f->data.query)->getQuery();
        if(xqry)
        {
            isFloat = true;
            d_val = xqry->getFieldTotal(f->data.column);
            if(f->sub_total)
                d_val -= getNearestSubTotalCheckPoint(f->data);
        }
        str = QString("%1").arg(d_val);
    }
    else
    {
        if(f->data.query == "Context Query" && f->data.column == "page_number")
            str = QString("%1").arg(_pageCounter);
        else if(f->data.query == "Context Query" && f->data.column == "page_count")
            str = f->data.column;
        else if(f->data.query == "Context Query" && f->data.column == "report_name")
            str = _reportData->name;
        else if(f->data.query == "Context Query" && f->data.column == "report_title")
            str = _reportData->title;
        else if(f->data.query == "Context Query" && f->data.column == "report_description")
            str = _reportData->description;
        else
        {
            populateData(f->data, dataThis);
            str = dataThis.getValue();
            QVariant v = dataThis.getVariant();
            d_val = v.toDouble(&isFloat);
        }
    }

    // Field coloring as a result of data.
    if (populateColorData(f->data, dataThis))
    {
      *outColorStr = dataThis.getValue();
    }

    // formatting
    if(f->format.length()>0)
    {
        if(!f->builtinFormat)
        {
            str = isFloat ? QString().sprintf(f->format.toLatin1().data(), d_val)
                          : QString().sprintf(f->format.toLatin1().data(), str.toLatin1().data());
        }
        else
        {
            if ((_database.driverName() != "QOCI8") && (_database.driverName() != "QOCI"))
                str = QString().sprintf(getSqlFromTag("fmt02",_database.driverName()).toLatin1().data(),getFunctionFromTag(f->format).toLatin1().data(), d_val);
            else
                str = QString().sprintf(getSqlFromTag("fmt02",_database.driverName()).toLatin1().data(), d_val);
        }

        if(f->builtinFormat)
        {
            XSqlQuery q(str, _database);
            if(q.first())
                str = q.value(0).toString();
            else
                str = QString::null;
        }
    }

    return str;
}


double ORPreRenderPrivate::getNearestSubTotalCheckPoint(const ORDataData & d)
{
  // use the various contexts setup to determine what we should be
  // doing and try and locate the nearest subtotal check point value
  // and return that value... if we are unable to locate one then we
  // will just return 0.0 which will case the final value to be a
  // running total

  if(_subtotContextPageFooter)
  {
    // first check to see if it's a page footer context
    // as that can happen from anywhere at any time.

    // TODO: acutally make this work
    if(_subtotPageCheckPoints.contains(d))
      return _subtotPageCheckPoints[d];

  }
  else if(_subtotContextMap != 0)
  {
    // next if an explicit map is set then we are probably
    // rendering a group head/foot now so we will use the
    // available their.. if it's not then we made a mistake

    if(_subtotContextMap->contains(d))
      return (*_subtotContextMap)[d];

  } else if(_subtotContextDetail != 0) {
    // finally if we are in a detail section then we will simply
    // traverse that details sections groups from the most inner
    // to the most outer and use the first check point we find

    // in actuallity we search from the outer most group to the
    // inner most group and just take the last value found which
    // would be the inner most group

    double dbl = 0.0;
    ORDetailGroupSectionData * grp = 0;
    for(int i = 0; i < (int)_subtotContextDetail->groupList.count(); i++)
    {
      grp = _subtotContextDetail->groupList[i];
      if(grp->_subtotCheckPoints.contains(d))
        dbl = grp->_subtotCheckPoints[d];
    }
    return dbl;

  }

  return 0.0;
}


//
// ORPreRender
//
ORPreRender::ORPreRender(QSqlDatabase pDb)
{
  _internal = new ORPreRenderPrivate();
  setDatabase(pDb);
}

ORPreRender::ORPreRender(const QDomDocument & pDocument, QSqlDatabase pDb)
{
  _internal = new ORPreRenderPrivate();
  setDatabase(pDb);
  setDom(pDocument);
}

ORPreRender::ORPreRender(const QDomDocument & pDocument, const ParameterList & pParams, QSqlDatabase pDb)
{
  _internal = new ORPreRenderPrivate();
  setDatabase(pDb);
  setParamList(pParams);
  setDom(pDocument);
}

ORPreRender::~ORPreRender()
{
  if(_internal)
  {
    delete _internal;
    _internal = 0;
  }
}

ORODocument* ORPreRender::generate()
{

  if (_internal == 0 || !_internal->_valid || _internal->_reportData == 0)
    return 0;

  // Do this check now so we don't have to undo alot of work later if it fails
  LabelSizeInfo label;
  if(_internal->_reportData->page.getPageSize() == "Labels") {
    label = LabelSizeInfo::getByName(_internal->_reportData->page.getLabelType());
    if(label.isNull())
      return 0;
  }

  _internal->_document = new ORODocument(_internal->_reportData->title, _internal->_printerType);
  _internal->_document->setPrinterParams(_internal->_printerParams );

  _internal->_pageCounter  = 0;
  _internal->_yOffset      = 0.0;

  if(!label.isNull())
  {
    if(_internal->_reportData->page.isPortrait()) {
      _internal->_topMargin = (label.startY() / 100.0);
      _internal->_bottomMargin = 0;
      _internal->_rightMargin = 0;
      _internal->_leftMargin = (label.startX() / 100.0);
    } else {
      _internal->_topMargin = (label.startX() / 100.0);
      _internal->_bottomMargin = 0;
      _internal->_rightMargin = 0;
      _internal->_leftMargin = (label.startY() / 100.0);
    }
  }
  else
  {
    _internal->_topMargin    = _internal->_reportData->page.getMarginTop();
    _internal->_bottomMargin = _internal->_reportData->page.getMarginBottom();
    _internal->_rightMargin  = _internal->_reportData->page.getMarginRight();
    _internal->_leftMargin   = _internal->_reportData->page.getMarginLeft();
  }

  _internal->_colorMap = _internal->_reportData->color_map;

  ReportPageOptions rpo(_internal->_reportData->page);
  // This should reflect the information of the report page size
  if(_internal->_reportData->page.getPageSize() == "Custom")
  {
    _internal->_maxWidth = _internal->_reportData->page.getCustomWidth();
    _internal->_maxHeight = _internal->_reportData->page.getCustomHeight();
  }
  else
  {
    PageSizeInfo pi = PageSizeInfo::getByName(_internal->_reportData->page.getPageSize());
    if(!label.isNull())
    {
      pi = PageSizeInfo::getByName(label.paper());
      rpo.setPageSize(label.paper());
    }
    if(pi.isNull())
    {
      _internal->_maxWidth = 8.5;
      _internal->_maxHeight = 11.0;
    }
    else
    {
      _internal->_maxWidth = (pi.width()/100.0);
      _internal->_maxHeight = (pi.height() / 100.0);
    }
  }

  if(!_internal->_reportData->page.isPortrait()) {
    qreal tmp = _internal->_maxWidth;
    _internal->_maxWidth = _internal->_maxHeight;
    _internal->_maxHeight = tmp;
  }

  _internal->_document->setPageOptions(rpo);

  while(!_internal->_lstQueries.isEmpty())
    delete _internal->_lstQueries.takeFirst();

  _internal->_lstQueries.append( new orQuery( "Context Query",			// MANU
        getSqlFromTag("fmt03", _internal->_database.driverName()),
      ParameterList(), true, _internal->_database ));

  QString tQuery = getSqlFromTag("fmt01",_internal->_database.driverName() );	// MANU
  if(_internal->_database.driverName()== "QOCI")
	  tQuery.replace("from dual","");

  QString val = QString::null;
  QRegExp re("'");
  for(int t = 0; t < _internal->_lstParameters.count(); t++)
  {
    Parameter p = _internal->_lstParameters[t];
    val = p.value().toString();
    val = val.replace(re, "''");
    if (_internal->_database.driverName() == "QMYSQL")
      tQuery += QString().sprintf(", \"%s\" AS \"%d\"", val.toLatin1().data(), t + 1);
	else if (_internal->_database.driverName() == "QOCI")
		tQuery += QString().sprintf(", '%s' AS \"%d\"", val.toLatin1().data(), t + 1);
    else
      tQuery += QString().sprintf(", text('%s') AS \"%d\"", val.toLatin1().data(), t + 1);

    if(!p.name().isEmpty())
    {
      if (_internal->_database.driverName() == "QMYSQL" )
        tQuery += QString().sprintf(", \"%s\" AS \"%s\"", val.toLatin1().data(), p.name().toLatin1().data());
	  else if ( _internal->_database.driverName() == "QOCI")
		  tQuery += QString().sprintf(", '%s' AS \"%s\"", val.toLatin1().data(), p.name().toLatin1().data());
      else
        tQuery += QString().sprintf(", text('%s') AS \"%s\"", val.toLatin1().data(), p.name().toLatin1().data());
    }
  }
  if(_internal->_database.driverName() == "QOCI")
	tQuery.push_back(" from dual");

  _internal->_lstQueries.append(new orQuery("Parameter Query", tQuery, ParameterList(), true, _internal->_database));
  
  QuerySource * qs = 0;
  for(unsigned int i = 0; i < _internal->_reportData->queries.size(); i++) {
      qs = _internal->_reportData->queries.get(i);
      _internal->_lstQueries.append(new orQuery(qs->name(), qs->query(_internal->_database), _internal->_lstParameters, true, _internal->_database));
  }

  _internal->_subtotPageCheckPoints.clear();
  for(int i = 0; i < _internal->_reportData->trackTotal.count(); i++)
  {
    _internal->_subtotPageCheckPoints.insert(_internal->_reportData->trackTotal[i], 0);

    XSqlQuery * xqry = _internal->getQuerySource(_internal->_reportData->trackTotal[i].query)->getQuery();
    if(xqry)
      xqry->trackFieldTotal(_internal->_reportData->trackTotal[i].column);
  }

  _internal->createNewPage();
  if(!label.isNull())
  {
// Label Print Run
    int row = 0;
    int col = 0;

    // remember the initial margin setting as we will be modifying
    // the value and restoring it as we move around
    qreal margin = _internal->_leftMargin;

    _internal->_yOffset = _internal->_topMargin;

    qreal w = (label.width() / 100.0);
    qreal wg = (label.xGap() / 100.0);
    qreal h = (label.height() / 100.0);
    qreal hg = (label.yGap() / 100.0);
    int numCols = label.columns();
    int numRows = label.rows();
    qreal tmp;

    // flip the value around if we are printing landscape
    if(!_internal->_reportData->page.isPortrait())
    {
      w = (label.height() / 100.0);
      wg = (label.yGap() / 100.0);
      h = (label.width() / 100.0);
      hg = (label.xGap() / 100.0);
      numCols = label.rows();
      numRows = label.columns();
    }

    for(int i = 0; i < _internal->_reportData->sections.count(); i++)
    {
      if(_internal->_reportData->sections.at(i))
      {
        ORDetailSectionData * detailData = _internal->_reportData->sections.at(i);
        if(detailData->detail != 0)
        {
          orQuery *orqThis = _internal->getQuerySource(detailData->key.query);
          XSqlQuery *query;

          if((orqThis->getQuery() != 0) && ((query = orqThis->getQuery())->size()))
          {
            query->first();
            do
            {
              tmp = _internal->_yOffset; // store the value as renderSection changes it
              _internal->renderSection(*(detailData->detail));
              _internal->_yOffset = tmp; // restore the value that renderSection modified

              col ++;
              _internal->_leftMargin += w + wg;
              if(col >= numCols)
              {
                _internal->_leftMargin = margin; // reset back to original value
                col = 0;
                row ++;
                _internal->_yOffset += h + hg;
                if(row >= numRows)
                {
                  _internal->_yOffset = _internal->_topMargin;
                  row = 0;
                  _internal->createNewPage();
                }
              }
            } while(query->next());
          }
        }
      }
    }
  }
  else
  {
// Normal Print Run
    if(_internal->_reportData->rpthead != 0)
      _internal->renderSection(*(_internal->_reportData->rpthead));

    for(int i = 0; i < _internal->_reportData->sections.count(); i++)
      if(_internal->_reportData->sections.at(i) != 0)
        _internal->renderDetailSection(*(_internal->_reportData->sections.at(i)));

    qreal rptfootSize = (_internal->_reportData->rptfoot != 0) ? _internal->renderSectionSize(*(_internal->_reportData->rptfoot), true) : 0;
    if(rptfootSize + _internal->finishCurPageSize(true) + _internal->_bottomMargin + _internal->_yOffset >= _internal->_maxHeight)
      _internal->createNewPage();
    if(_internal->_reportData->rptfoot != 0)
      _internal->renderSection(*(_internal->_reportData->rptfoot));
  }
  _internal->finishCurPage(true);

  // _postProcText contains those text boxes that need to be updated
  // with information that wasn't available at the time it was added to the document
  for(int i = 0; i < _internal->_postProcText.size(); i++)
  {
    OROTextBox * tb = _internal->_postProcText.at(i);
    if(tb->text() == "page_count")
      tb->setText(QString::number(_internal->_document->pages()));
  }

  while(!_internal->_lstQueries.isEmpty())
    delete _internal->_lstQueries.takeFirst();
  _internal->_postProcText.clear();

  ORODocument * pDoc = _internal->_document;
  _internal->_document = 0;
  return pDoc;
}

void ORPreRender::setDatabase(QSqlDatabase pDb)
{
  if(_internal != 0)
  {
    _internal->_database = pDb;
    if(!_internal->_database.isValid())
      _internal->_database = QSqlDatabase::database();
  }
}

QSqlDatabase ORPreRender::database() const
{
  if(_internal != 0)
    return _internal->_database;
  return QSqlDatabase();
}

bool ORPreRender::setDom(const QDomDocument & docReport)
{
  if(_internal != 0)
  {
    if(_internal->_reportData != 0)
      delete _internal->_reportData;
    _internal->_valid = false;

    _internal->_docReport = docReport;
    _internal->_reportData = new ORReportData();
    if(parseReport(_internal->_docReport.documentElement(),*(_internal->_reportData), database()))
    {
      _internal->_valid = true;

      QList<ORParameter> vSATO = _internal->_reportData->definedParams.values("$SATO");
      QList<ORParameter> vZEBRA = _internal->_reportData->definedParams.values("$ZEBRA");
      if(!vSATO.isEmpty()) {
        _internal->_printerType = ReportPrinter::Sato;
        _internal->_printerParams = vSATO.first().values;
      }
      else if(!vZEBRA.isEmpty()) {
        _internal->_printerType = ReportPrinter::Zebra;
        _internal->_printerParams = vZEBRA.first().values;
      }

      // make sure all the watermark values are at their defaults
      _internal->_wmStatic = true;
      _internal->_wmText = QString::null;
      _internal->_wmData.query = QString::null;
      _internal->_wmData.column = QString::null;
      _internal->_wmFont = QFont("Arial");
      _internal->_wmOpacity = 25;

      if(_internal->_reportData->wmData.valid)
      {
        if(_internal->_reportData->wmData.staticText)
          _internal->_wmText = _internal->_reportData->wmData.text;
        else
        {
          _internal->_wmStatic = false;
          _internal->_wmData.query = _internal->_reportData->wmData.data.query;
          _internal->_wmData.column = _internal->_reportData->wmData.data.column;
        }
        if(!_internal->_reportData->wmData.useDefaultFont)
          _internal->_wmFont = _internal->_reportData->wmData.font;
        _internal->_wmOpacity = _internal->_reportData->wmData.opacity;
      }

      // mark sure all the background values are at their defaults
      _internal->_bgStatic = true;
      _internal->_bgImage = QImage();
      _internal->_bgData.query = QString::null;
      _internal->_bgData.column = QString::null;
      _internal->_bgOpacity = 25;
      _internal->_bgAlign = Qt::AlignLeft | Qt::AlignTop;
      _internal->_bgScale = false;
      _internal->_bgScaleMode = Qt::IgnoreAspectRatio;
      _internal->_bgRect.setX(0.0);
      _internal->_bgRect.setY(0.0);
      _internal->_bgRect.setWidth(1.0);
      _internal->_bgRect.setHeight(1.0);

      if(_internal->_reportData->bgData.enabled)
      {
        if(_internal->_reportData->bgData.staticImage)
        {
          if(!_internal->_reportData->bgData.image.isEmpty())
          {
            QByteArray imgdata = QUUDecode(_internal->_reportData->bgData.image);
            QImage img = QImage::fromData(imgdata);
            _internal->_bgImage = img;
          }
        }
        else
        {
          _internal->_bgStatic = false;
          _internal->_bgData.query = _internal->_reportData->bgData.data.query;
          _internal->_bgData.column = _internal->_reportData->bgData.data.column;
        }
        _internal->_bgOpacity = _internal->_reportData->bgData.opacity;
        _internal->_bgAlign = _internal->_reportData->bgData.align;
        _internal->_bgScale = (_internal->_reportData->bgData.mode != "clip");
        _internal->_bgScaleMode = Qt::KeepAspectRatio;
        _internal->_bgRect.setX(_internal->_reportData->bgData.rect.x() / 100.0);
        _internal->_bgRect.setY(_internal->_reportData->bgData.rect.y() / 100.0);
        _internal->_bgRect.setWidth(_internal->_reportData->bgData.rect.width() / 100.0);
        _internal->_bgRect.setHeight(_internal->_reportData->bgData.rect.height() / 100.0);
      }
    }
  }
  return isValid(); 
}

void ORPreRender::setParamList(const ParameterList & pParams)
{
  if(_internal != 0) {
    _internal->_lstParameters = pParams;
  }
}

ParameterList ORPreRender::paramList() const
{
  ParameterList plist;
  if(_internal != 0)
    plist = _internal->_lstParameters;
  return plist;
}

bool ORPreRender::isValid() const
{
  if(_internal != 0 && _internal->_valid && doParamsSatisfy())
    return true;
  return false;
}

bool ORPreRender::doParamsSatisfy() const
{
  if(_internal == 0 || !_internal->_valid)
    return false;

  QuerySource * qs = 0;
  for(unsigned int i = 0; i < _internal->_reportData->queries.size(); i++)
  {
    qs = _internal->_reportData->queries.get(i);
    orQuery qry(qs->name(), qs->query(_internal->_database), _internal->_lstParameters, false, _internal->_database);
    if(qry.missingParamList.count() > 0)
      return false;
  }
  
  return true;
}

QString ORPreRender::watermarkText() const
{
  return ( _internal != 0 ? _internal->_wmText : QString::null );
}

void ORPreRender::setWatermarkText(const QString & txt)
{
  if(_internal != 0)
    _internal->_wmText = txt;
}

QFont ORPreRender::watermarkFont() const
{
  return (_internal != 0 ? _internal->_wmFont : QFont() );
}

void ORPreRender::setWatermarkFont(const QFont & fnt)
{
  if(_internal != 0)
    _internal->_wmFont = fnt;
}

unsigned char ORPreRender::watermarkOpacity() const
{
  return ( _internal != 0 ? _internal->_wmOpacity : 0 );
}

void ORPreRender::setWatermarkOpacity(unsigned char o)
{
  if(_internal != 0)
    _internal->_wmOpacity = o;
}

QImage ORPreRender::backgroundImage() const
{
  return ( _internal != 0 ? _internal->_bgImage : QImage() );
}

void ORPreRender::setBackgroundImage(const QImage & img)
{
  if(_internal != 0)
    _internal->_bgImage = img;
}

QRectF ORPreRender::backgroundRect() const
{
  if(_internal != 0) {
    if(_internal->_bgRect.isValid()) {
      return _internal->_bgRect;
    }
  }
  return QRectF();
}

void ORPreRender::setBackgroundRect(const QRectF & r)
{
  if(_internal != 0)
    _internal->_bgRect = r;
}

void ORPreRender::setBackgroundRect(double x, double y, double w, double h)
{
  if(_internal != 0)
    _internal->_bgRect.setRect(x, y, w, h);
}

unsigned char ORPreRender::backgroundOpacity() const
{
  return ( _internal != 0 ? _internal->_bgOpacity : 0 );
}

void ORPreRender::setBackgroundOpacity(unsigned char o)
{
  if(_internal != 0)
    _internal->_bgOpacity = o;
}

int ORPreRender::backgroundAlignment() const
{
  return ( _internal != 0 ? _internal->_bgAlign : 0 );
}

void ORPreRender::setBackgroundAlignment(int a) {
  if(_internal != 0)
    _internal->_bgAlign = a;
}

bool ORPreRender::backgroundScale() const
{
  return ( _internal != 0 ? _internal->_bgScale : false );
}

void ORPreRender::setBackgroundScale(bool scale)
{
  if(_internal != 0)
    _internal->_bgScale = scale;
}

Qt::AspectRatioMode ORPreRender::backgroundScaleMode() const
{
  return ( _internal != 0 ? _internal->_bgScaleMode : Qt::IgnoreAspectRatio );
}

void ORPreRender::setBackgroundScaleMode(Qt::AspectRatioMode mode)
{
  if(_internal != 0)
    _internal->_bgScaleMode = mode;
}

