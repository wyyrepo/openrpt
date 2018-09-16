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

#include "documentscene.h"
#include "reporthandler.h"
#include "reportpageoptions.h"
#include "pagesizeinfo.h"
#include "labelsizeinfo.h"
#include "graphicsitems.h"
#include "graphicssection.h"
#include "sectioneditor.h"
#include "colorlist.h"
#include "pagesetup.h"
#include "dbfiledialog.h"
#include "builtinSqlFunctions.h"
#include "reportparameterlist.h"
#include "reportproperties.h"
#include "reportgridoptions.h"
#include "fontutils.h"
#include "labeldefinitions.h"


#include <xsqlquery.h>

#include <QSqlError>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>

DocumentScene::DocumentScene(bool newDoc, ReportHandler *handler, QObject * parent)
  : QGraphicsScene(parent), _undoIndex(0), _handler(handler), _loadingInProgress(false)
{
  _loadingInProgress = true;
  _gridOptions = 0;
  _scaledTo = 1.0;
  lastSaveToDb = false;
  dbRecordGrade = -1;
  _pageOptions = 0;
  
  initData();

  if(newDoc) {
      pageOptionsChanged();
  }

  connect(this, SIGNAL(selectionChanged()), this, SLOT(onSelChanged()));

  _loadingInProgress = false;
}


DocumentScene::~DocumentScene()
{
  clear();
}


void DocumentScene::initData()
{
    _modified = false;
    _maxSnaphots = 50; // default size of undo stack

    qsList = new QuerySourceList();
    connect(qsList, SIGNAL(updated()), this, SLOT(setModified()));

    rptHead = rptFoot = 0;
    pageHeadFirst = pageHeadOdd = pageHeadEven = pageHeadLast = pageHeadAny = 0;
    pageFootFirst = pageFootOdd = pageFootEven = pageFootLast = pageFootAny = 0;

    // Set default Watermark Properties
    _wmOpacity = 25;
    _wmFont = QFont("Arial");
    _wmUseDefaultFont = true;
    _wmUseStaticText = true;
    _wmText = QString::null;
    _wmColumn = QString::null;
    _wmQuery = QString::null;

    // Set default Background Properties
    _bgEnabled = false;
    _bgStatic = true;
    _bgImage = QString::null;
    _bgQuery = QString::null;
    _bgColumn = QString::null;
    _bgResizeMode = "clip";
    _bgAlign = Qt::AlignLeft | Qt::AlignTop;
    _bgBoundsX = 0;
    _bgBoundsY = 0;
    _bgBoundsWidth = 0;
    _bgBoundsHeight = 0;
    _bgOpacity = 25;

    _pageShadow = addRect(5, 5, 0, 0, QPen(Qt::NoPen), QBrush(QColor("lightgrey")));
    _pageShadow->setZValue(-3);
    _page = addRect(0, 0, 0, 0, QPen(), QBrush(QColor("white")));
    _page->setZValue(-2);
    QPen p(Qt::DashLine);
    p.setColor(QColor("blue"));
    _pageMargin = addRect(0, 0, 0, 0, p);
    _pageMargin->setZValue(-1);

    if(!_pageOptions) {    
        _pageOptions = new ReportPageOptions;
        connect(_pageOptions, SIGNAL(pageOptionsChanged()), this, SLOT(pageOptionsChanged()));
    }
}


void DocumentScene::clear()
{
    QList<QGraphicsItem*> list = selectedItems();
    foreach(QGraphicsItem* item, list) {
        // have to unselect items before deleting, else risk of error in during section removal
        item->setSelected(false);
    }

    removeReportHead();
    removePageHeadAny();
    removePageHeadFirst();
    removePageHeadLast();
    removePageHeadOdd();
    removePageHeadEven();
    removeReportFoot();
    removePageFootAny();
    removePageFootFirst();
    removePageFootLast();
    removePageFootOdd();
    removePageFootEven();
    while(detailSectionCount() > 0)
        removeSection(0, true);
    if(qsList != 0)
    {
        delete qsList;
        qsList = 0;
    }
    if(_pageOptions)
    {
        delete _pageOptions;
        _pageOptions = 0;
    }

}

void DocumentScene::setGridOptions(ReportGridOptions * rgo)
{
  if(_gridOptions == rgo)
    return;
  if(_gridOptions && _gridOptions != rgo)
    disconnect(_gridOptions, SIGNAL(gridOptionsChanged()), this, SLOT(update()));
  _gridOptions = rgo;
  if(_gridOptions)
    connect(_gridOptions, SIGNAL(gridOptionsChanged()), this, SLOT(update()));
  update();
}

void DocumentScene::pageOptionsChanged()
{
  int dpiX = 100;
  int cw = 0;
  int ch = 0;
  int width = 0;
  int height = 0;

  if(_pageOptions->getPageSize() == "Labels")
  {
    // add code here to determine how big our canvas area
    // should be the type of label specified
    LabelSizeInfo lbl = LabelSizeInfo::getByName(_pageOptions->getLabelType());
    if(!lbl.isNull())
    {
      if(_pageOptions->getOrientation() == ReportPageOptions::Portrait)
      {
        width = lbl.width();
        height = lbl.height();
      }
      else /*if(_pageOptions->getOrientation() == ReportPageOptions::Landscape)*/
      {
        width = lbl.height();
        height = lbl.width();
      }
    }
  }
  else
  {
    PageSizeInfo pi = PageSizeInfo::getByName(_pageOptions->getPageSize());
    if(pi.isNull())
    {
      cw = (int)(_pageOptions->getCustomWidth() * dpiX);
      ch = (int)(_pageOptions->getCustomHeight() * dpiX);
    }
    else
    {
      cw = (int)((pi.width() / 100.0) * dpiX);
      ch = (int)((pi.height() / 100.0) * dpiX);
    }

    width = (_pageOptions->getOrientation() == ReportPageOptions::Portrait ? cw : ch);
    height = (_pageOptions->getOrientation() == ReportPageOptions::Portrait ? ch : cw);
  }


  if(width < 1)
  {
    qDebug("%s: ERROR: width = %d", "pageOptionsChanged", width);
    width = 1;
  }
  if(height < 1)
  {
    qDebug("%s: ERROR: height = %d", "pageOptionsChanged", height);
    height = 1;
  }

  bool isModified = false;

  QRectF r = _pageShadow->rect();
  r.setSize(QSize(width, height));
  if(_pageShadow->rect() != r) 
  {
    _pageShadow->setRect(r);
    isModified = true;
  }

  r = _page->rect();
  r.setSize(QSize(width, height));
  if(_page->rect() != r) 
  {
    _page->setRect(r);
    isModified = true;
  }

  r.setLeft(r.left() + (_pageOptions->getMarginLeft() * dpiX));
  r.setRight(r.right() - (_pageOptions->getMarginRight() * dpiX));
  r.setTop(r.top() + (_pageOptions->getMarginTop() * dpiX));
  r.setBottom(r.bottom() - (_pageOptions->getMarginBottom() * dpiX));
  if(_pageMargin->rect() != r)
  {
      _pageMargin->setRect(r);  
      isModified = true;
  }

  QRectF newSceneRect = _page->rect();
  newSceneRect.adjust(-50,-50,50,50);
  setSceneRect(newSceneRect);

  repositionSections();

  if(isModified)
  {
    setModified();
  }
}


ORGraphicsSectionItem * DocumentScene::getSection(QPointF scenePos) const
{
    ORGraphicsSectionItem * section = NULL;

    QList<QGraphicsItem*> list = items(scenePos);
    for(int i = 0; i < list.count(); i++)
    {
        if(list.at(i)->type() == ORGraphicsSectionItem::Type)
        {
            section = qgraphicsitem_cast<ORGraphicsSectionItem*>(list.at(i));
            break;
        }
    }

    return section;
}


ORGraphicsSectionItem * DocumentScene::getSection(QString title) const
{
    ORGraphicsSectionItem * section = NULL;

    QList<ORGraphicsSectionItem *> list = sectionsList();

    for(int i = 0; i < list.count(); i++)
    {
        if(list.at(i)->title() == title)
        {
            section = list.at(i);
            break;
        }
    }

    return section;
}


void DocumentScene:: highlightSectionTitles()
{
    QList<ORGraphicsSectionItem *> sections = sectionsList();
    QList<QGraphicsItem*> selection = selectedItems();

    for(int i = 0; i < sections.count(); i++)
    {
        ORGraphicsSectionItem * section = sections.at(i);
        bool sectionSelected = false;

        foreach(QGraphicsItem* item, selection)
        {
            if(item->parentItem() == section)
            {
                sectionSelected = true;
                break;
            }
        }

        section->highlightTitle(sectionSelected);
    }
}


QList<ORGraphicsSectionItem *> DocumentScene::sectionsList() const
{
    QList<ORGraphicsSectionItem *> res;

    QList<QGraphicsItem*> list = items();
    for(int i = 0; i < list.count(); i++)
    {
        if(list.at(i)->type() == ORGraphicsSectionItem::Type)
        {
            res.append(qgraphicsitem_cast<ORGraphicsSectionItem*>(list.at(i)));
        }
    }

    return res;
}


void DocumentScene::highlightSections(ORGraphicsSectionItem::Highlight color)
{
	QList<QGraphicsItem*> list = items();
	for(int i = 0; i < list.count(); i++)
	{
		if(list.at(i)->type() == ORGraphicsSectionItem::Type)
		{
			ORGraphicsSectionItem * section = qgraphicsitem_cast<ORGraphicsSectionItem*>(list.at(i));
			section->highlight(color);
		}
	}
}


void DocumentScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
  if((mouseEvent->button() == Qt::LeftButton) && (_handler->insertItemCode() != 0))
  {
    QGraphicsItem * gi = 0;
    QGraphicsItem * section = 0;
    QList<QGraphicsItem*> list = items(mouseEvent->scenePos());
    for(int i = 0; i < list.count(); i++)
    {
      if(list.at(i)->type() == ORGraphicsSectionItem::Type)
      {
        section = list.at(i);
        break;
      }
    }
    if(!section)
    {
      QMessageBox::warning( mouseEvent->widget(), tr("No Section"),
        tr("You must place an object inside a section on the report.") );
      QGraphicsScene::mousePressEvent(mouseEvent);
      return;
    } 
    switch(_handler->insertItemCode())
    {
      case 0: // No Item
        break;
      case 1: // label
        gi = new ORGraphicsLabelItem();
        break;
      case 2: // label
        gi = new ORGraphicsFieldItem();
        break;
      case 3: // label
        gi = new ORGraphicsTextItem();
        break;
      case 4: // line
        gi = new ORGraphicsLineItem();
        break;
      case 5: // barcode
        gi = new ORGraphicsBarcodeItem();
        break;
      case 6: // image
        gi = new ORGraphicsImageItem();
        break;
      case 7: // graph
        gi = new ORGraphicsGraphItem();
        break;
      case 8: // rect
        gi = new ORGraphicsRectItem();
        break;
      case 9: // crosstab
        gi = new ORGraphicsCrossTabItem();
      default:
        qDebug("%s: unknown insertItemCode %d", "mousePressEvent", _handler->insertItemCode());
    }
    if(gi)
    {
      addItem(gi);

      ORGraphicsSectionItem * csection = getSection(mouseEvent->scenePos());
      gi->setParentItem(csection);
      gi->setPos(gi->mapToParent(gi->mapFromScene(mouseEvent->scenePos())));

      if(!(mouseEvent->modifiers() & Qt::ControlModifier))
      {
        _handler->resetInsertItemCode();
      }
      setModified(true);
	  update();
      return;
    }
  }
  else if((mouseEvent->button() == Qt::RightButton))
  {
      // if we don't intercept the event the items will the deselected on right-click, as
      // default QGraphicsScene::mousePressEvent() deselect items if "RubberBandDrag" active
      mouseEvent->setAccepted(true);
      return;
  }
  else if((mouseEvent->button() == Qt::LeftButton) && (mouseEvent->modifiers() & Qt::AltModifier)) {

      // Selection of all the items in the section
      // First find an item under the mouse pointer...
      QGraphicsItem * item = 0;
      QList<QGraphicsItem*> list = items(mouseEvent->scenePos());
      for(int i = 0; i < list.count(); i++)
      {
          if(list.at(i)->type() != ORGraphicsSectionItem::Type)
          {
              item = list.at(i);
              break;
          }
      }

      // ...then select all the items of the item's section
      if(item)
      {
          ORGraphicsSectionItem * csection = qgraphicsitem_cast<ORGraphicsSectionItem*>(item->parentItem());
          if(csection) {
              qWarning("section=%s", csection->title().toLocal8Bit().constData());
              QList<QGraphicsItem*> list = csection->childItems();
              foreach(QGraphicsItem* item, list)
              {
                  item->setSelected(true);
              }
              mouseEvent->setAccepted(true);
              return;
          }
      }
  }

  QGraphicsScene::mousePressEvent(mouseEvent);
}

void DocumentScene::sectionEditor(QWidget * parent)
{
  SectionEditor * se = new SectionEditor(parent);
  se->init(this);
  se->exec();
  delete se;
}

void DocumentScene::colorList(QWidget * parent)
{
  QMap<QString, QColor> cm = _colorMap;
  ColorList cl(parent);
  cl.init(&_colorMap);
  cl.exec();
  if(cm.count() == _colorMap.count())
  {
    // the two lists have the same number of items so
    // we will have to check each item for equality
    // to see if a change was made
    QMapIterator<QString, QColor> cit(cm);
    while(cit.hasNext())
    {
      cit.next();
      if(!_colorMap.contains(cit.key()) || _colorMap[cit.key()] != cm[cit.key()])
      {
        setModified(true);
        break;
      }
    }
  }
  else
  {
    // they don't have the same number of items
    // so we can just assume the list was changed
    setModified(true);
  }
}

void DocumentScene::docInfoEditor(QWidget * parent)
{
  ReportProperties * diag = new ReportProperties(parent);
  if(diag)
  {
    // Info
    diag->setReportTitle(reportTitle());
    diag->setReportName(reportName());
    diag->setReportDescription(reportDescription());
    // Background
    diag->setBgEnabled(bgEnabled());
    diag->setBgStatic(bgStatic());
    diag->setBgImageData(bgImage());
    diag->setBgQuery(qsList, bgQuery());
    diag->setBgColumn(bgColumn());
    diag->setBgResizeMode(bgResizeMode());
    diag->setBgAlign(bgAlign());
    diag->setBgBoundsX(bgBoundsX());
    diag->setBgBoundsY(bgBoundsY());
    diag->setBgBoundsWidth(bgBoundsWidth());
    diag->setBgBoundsHeight(bgBoundsHeight());
    diag->setBgOpacity(bgOpacity());
    // Watermark
    diag->setWatermarkOpacity(watermarkOpacity());
    diag->setWmFont(watermarkFont());
    diag->SetUseBestWMFont(watermarkUseDefaultFont());
    diag->setWmTextStatic(watermarkUseStaticText());
    diag->setWmText(watermarkText());
    diag->setWmColumn(watermarkColumn());
    diag->setWmQuery(qsList, watermarkQuery());
    if(diag->exec() == QDialog::Accepted)
    {
      // Info
      setReportTitle(diag->getReportTitle());
      setReportName(diag->getReportName());
      setReportDescription(diag->getReportDescription());
      // Background
      setBgEnabled(diag->isBgEnabled());
      setBgStatic(diag->isBgStatic());
      setBgImage(diag->getBgImageData());
      setBgQuery(diag->getBgQuery());
      setBgColumn(diag->getBgColumn());
      setBgResizeMode(diag->getBgResizeMode());
      setBgAlign(diag->getBgAlign());
      setBgBoundsX(diag->getBgBoundsX());
      setBgBoundsY(diag->getBgBoundsY());
      setBgBoundsWidth(diag->getBgBoundsWidth());
      setBgBoundsHeight(diag->getBgBoundsHeight());
      setBgOpacity(diag->getBgOpacity());
      // Watermark
      setWatermarkOpacity(diag->getWatermarkOpacity());
      setWatermarkUseDefaultFont(diag->getUseBestWmFont());
      setWatermarkFont(diag->getWmFont());
      setWatermarkUseStaticText(diag->isWmTextStatic());
      setWatermarkText(diag->getWmText());
      setWatermarkColumn(diag->getWmColumn());
      setWatermarkQuery(diag->getWmQuery());
    }
    delete diag;
  }
}

void DocumentScene::docPageSetup(QWidget * parent)
{
  PageSetup ps(parent);
  ps.setData(*_pageOptions);
  if(ps.exec() == QDialog::Accepted)
  {
    // ok it was good so lets update everything
    ps.getData(*_pageOptions);
  }
}

void DocumentScene::querySourceList(QWidget * parent)
{
  QueryList * ql = new QueryList(parent);
  ql->init(qsList);
  ql->exec();
  delete ql;
}

// Edit Label Definitions
void DocumentScene::editLabelDefinitions(QWidget * parent)
{
  LabelDefinitions * ld = new LabelDefinitions(parent);
  ld->init();
  ld->exec();
  delete ld;
}

void DocumentScene::setModified(bool yes)
{
  if(_modified != yes)
  {
    _modified = yes;
    emit modified();
  }

  if(!_handler || _loadingInProgress) {
      return; // modifications during initialization
  }

  if(yes) {
      recordSnapshot();
  }
}

void DocumentScene::recordSnapshot()
{
    // cut off the snapshots list after current position, in case of undo not followed by redo
    while(_snapshots.size() > _undoIndex+1) {
        _snapshots.removeLast(); 
    }

    _snapshots.append(document());
    if(_snapshots.size()>_maxSnaphots) {
        _snapshots.removeFirst();
    }

    _undoIndex = _snapshots.size() - 1;
}

bool DocumentScene::undo()
{
    if(_undoIndex>0 && _undoIndex < _snapshots.size()) {
        loadDocument(_snapshots.at(_undoIndex-1).documentElement(), _parentWindow, false);
        _undoIndex --;
        if(_undoIndex==0) {
            setModified(false);
        }
        return true;
    }
    else {
        return false;
    }
}

bool DocumentScene::redo()
{
    if(_snapshots.size() > 1 && _undoIndex < _snapshots.size() - 1) {
        loadDocument(_snapshots.at(_undoIndex+1).documentElement(), _parentWindow, false);
        _undoIndex ++;
        return true;
    }
    else {
        return false;
    }
}

ORGraphicsSectionItem * DocumentScene::getReportHead() { return rptHead; }

void DocumentScene::removeReportHead()
{
  if(rptHead)
  {
    delete rptHead;
    rptHead = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertReportHead()
{
  if(!rptHead)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Report Header"));
    rptHead = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getReportFoot() { return rptFoot; }

void DocumentScene::removeReportFoot()
{
  if(rptFoot)
  {
    delete rptFoot;
    rptFoot = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertReportFoot()
{
  if(!rptFoot)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Report Footer"));
    rptFoot = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageHeadFirst() { return pageHeadFirst; }

void DocumentScene::removePageHeadFirst()
{
  if(pageHeadFirst)
  {
    delete pageHeadFirst;
    pageHeadFirst = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageHeadFirst()
{
  if(!pageHeadFirst)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Header (First)"));
    pageHeadFirst = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageHeadOdd() { return pageHeadOdd; }

void DocumentScene::removePageHeadOdd()
{
  if(pageHeadOdd != 0)
  {
    delete pageHeadOdd;
    pageHeadOdd = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageHeadOdd()
{
  if(pageHeadOdd == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Header (Odd)"));
    pageHeadOdd = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageHeadEven() { return pageHeadEven; }

void DocumentScene::removePageHeadEven()
{
  if(pageHeadEven != 0)
  {
    delete pageHeadEven;
    pageHeadEven = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageHeadEven()
{
  if(pageHeadEven == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Header (Even)"));
    pageHeadEven = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageHeadLast() { return pageHeadLast; }

void DocumentScene::removePageHeadLast()
{
  if(pageHeadLast != 0)
  {
    delete pageHeadLast;
    pageHeadLast = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageHeadLast()
{
  if(pageHeadLast == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Header (Last)"));
    pageHeadLast = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageHeadAny() { return pageHeadAny; }

void DocumentScene::removePageHeadAny()
{
  if(pageHeadAny != 0)
  {
    delete pageHeadAny;
    pageHeadAny = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageHeadAny()
{
  if(pageHeadAny == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Header (Any)"));
    pageHeadAny = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageFootFirst() { return pageFootFirst; }

void DocumentScene::removePageFootFirst()
{
  if(pageFootFirst != 0)
  {
    delete pageFootFirst;
    pageFootFirst = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageFootFirst()
{
  if(pageFootFirst == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Footer (First)"));
    pageFootFirst = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageFootOdd() { return pageFootOdd; }

void DocumentScene::removePageFootOdd()
{
  if(pageFootOdd != 0)
  {
    delete pageFootOdd;
    pageFootOdd = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageFootOdd()
{
  if(pageFootOdd == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Footer (Odd)"));
    pageFootOdd = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageFootEven() { return pageFootEven; }

void DocumentScene::removePageFootEven()
{
  if(pageFootEven != 0)
  {
    delete pageFootEven;
    pageFootEven = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageFootEven()
{
  if(pageFootEven == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Footer (Even)"));
    pageFootEven = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageFootLast() { return pageFootLast; }

void DocumentScene::removePageFootLast()
{
  if(pageFootLast != 0)
  {
    delete pageFootLast;
    pageFootLast = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageFootLast()
{
  if(pageFootLast == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Footer (Last)"));
    pageFootLast = rs;
    setModified(true);
    repositionSections();
  }
}

ORGraphicsSectionItem * DocumentScene::getPageFootAny() { return pageFootAny; }

void DocumentScene::removePageFootAny()
{
  if(pageFootAny != 0)
  {
    delete pageFootAny;
    pageFootAny = 0;
    setModified(true);
    repositionSections();
  }
}

void DocumentScene::insertPageFootAny()
{
  if(pageFootAny == 0)
  {
    ORGraphicsSectionItem * rs = new ORGraphicsSectionItem();
    rs->setTitle(tr("Page Footer (Any)"));
    pageFootAny = rs;
    setModified(true);
    repositionSections();
  }
}

int DocumentScene::detailSectionCount()
{
  return detailList.count();
}

ORGraphicsSectionDetail * DocumentScene::getSection(int i)
{
  return detailList.at(i);
}

void DocumentScene::insertSection(int idx, ORGraphicsSectionDetail * rsd)
{
  detailList.insert(idx, rsd);
  setModified(true);
  repositionSections();
}

int DocumentScene::findSection(const QString & name)
{
  // find the item by its name
  ORGraphicsSectionDetail * rsd = 0;
  for(int i = 0; i < detailList.count(); i++)
  {
    rsd = detailList.at(i);
    if(name == rsd->getTitle()) return i;
  }
  return -1;
}

void DocumentScene::removeSection(int idx, bool del)
{
  ORGraphicsSectionDetail * rsd = detailList.at(idx);
  detailList.removeAt(idx);
  setModified(true);
  if(del)
    delete rsd;
  repositionSections();
}

void DocumentScene::setReportTitle(const QString & str)
{
  if(_title != str)
  {
    _title = str;
    setModified(true);
    emit modified(); // if the document is already marked as modified this signal does not get emitted so do so now to make sure it does
  }
}

void DocumentScene::setReportDescription(const QString & str)
{
  if(_descr != str)
  {
    _descr = str;
    setModified(true);
  }
}

void DocumentScene::setReportName(const QString & str)
{
  if(_name != str)
  {
    _name = str;
    setModified(true);
  }
}

bool DocumentScene::save(QWidget * parent)
{
  bool res = false;
  // do we have a valid filename?
  if(lastSaveToDb == false)
  {
    if(!filename.isEmpty())
    {
      QFile * file = new QFile(filename);
      res = save(file, parent);
      delete file;
    }
    else
    {
      res = saveAs(parent);
    }
  }
  else
  {
    // do a save to the database.
    res = saveToDb(parent);
  }
  return res;
}

bool DocumentScene::save(QIODevice * iod, QWidget * parent)
{
  // ok lets do it
  if(iod->open(QIODevice::WriteOnly))
  {
    // now write it to the file
    QTextStream ts(iod);
    ts.setCodec("UTF-8");
#ifdef Q_OS_WIN
    // bug #15457 Saving UTF files on windows does not write BOM
    ts.setGenerateByteOrderMark(true);
#endif
    ts << document().toString();
    iod->close();
    lastSaveToDb = false;
    setModified( false );
    return true;
  }
  else
  {
    //qDebug("Failed to open IODevice for writing.");
    QMessageBox::warning(parent,tr("Report Writer"),
        tr("Unable to open/create file for writing!\n"
           "Save Failed! Check to make sure that you have\n"
           "permissions to the file you are trying to save to."),
        tr("Ok"), 0, 0, 0, 0);
    return false;
  }
}

bool DocumentScene::saveAs(QWidget * parent)
{
  // we need to get a file to write to
  // really need to work on this....
  // but for now...
  QString file = QFileDialog::getSaveFileName( parent, tr("Choose filename to save"), "", tr("XML (*.xml)") );

  if( file.isEmpty() )
    return false;

  QFileInfo fi(file);
  if(fi.suffix().isEmpty())
    file += ".xml";

  filename = file;

  QFile f(file);
  bool res = save(&f, parent);

  return res;
}

bool DocumentScene::saveToDb(QWidget * parent)
{
  // do what we need to do
  DBFileDialog rptDiag(parent);
  rptDiag.setWindowTitle(tr("Save Report to Database"));
  if(!dbRecordName.isEmpty())
  {
    rptDiag._name->setText(dbRecordName);
  }
  else
  {
    rptDiag._name->setText(reportName());
  }
  rptDiag._grade->setValue(dbRecordGrade);
  if(rptDiag.exec() == QDialog::Accepted)
  {
    QString name = rptDiag.getName();
    QString desc = reportDescription();
    QString src  = document().toString();
    int grade = rptDiag.getGrade();

    XSqlQuery q;
    XSqlQuery qry;

    q.prepare(getSqlFromTag("fmt09", QSqlDatabase::database().driverName()));
    q.bindValue(":report_name", name);
    q.bindValue(":report_grade", grade);
    q.exec();
    if(q.first())
    {
      // update old record
      qry.prepare(getSqlFromTag("fmt10", QSqlDatabase::database().driverName()));
      qry.bindValue(":report_desc", desc);
      qry.bindValue(":report_src", src);
      qry.bindValue(":report_id", q.value("report_id").toInt());
      qry.bindValue(":report_name", name);
    }
    else
    {
      // insert new record
      qry.prepare(getSqlFromTag("fmt11", QSqlDatabase::database().driverName()));
      qry.bindValue(":report_name", name);
      qry.bindValue(":report_desc", desc);
      qry.bindValue(":report_src", src);
      qry.bindValue(":report_grade", grade);
    }

    if(qry.exec())
    {
      lastSaveToDb = true;
      setModified(false);
      dbRecordName = name;
      dbRecordGrade = grade;
      if(_handler)
      {
        q.exec();
        if(q.first())
          _handler->sReportsChanged(q.value("report_id").toInt(), true);
        else
          _handler->sReportsChanged(-1, true);
      }
      return true;
    }
    else
    {
      // no good lets tell the user
            QSqlError sqlErr = qry.lastError();
      QMessageBox::critical(parent,tr("Error saving to database"),sqlErr.databaseText());
    }
  }
  return false;
}

QDomDocument DocumentScene::document()
{
  QDomDocument doc = QDomDocument("openRPTDef");
  QDomElement root = doc.createElement("report");
  doc.appendChild(root);

  //title
  QDomElement title = doc.createElement("title");
  title.appendChild(doc.createTextNode(reportTitle()));
  root.appendChild(title);

  QDomElement rname = doc.createElement("name");
  rname.appendChild(doc.createTextNode(reportName()));
  root.appendChild(rname);

  QDomElement rdesc = doc.createElement("description");
  rdesc.appendChild(doc.createTextNode(reportDescription()));
  root.appendChild(rdesc);

  for(QMap<QString,ORParameter>::iterator it = _definedParams.begin();
          it != _definedParams.end(); it++)
  {
    QDomElement param = doc.createElement("parameter");
    ORParameter paramData = it.value();
    param.setAttribute("name", paramData.name);
    param.setAttribute("type", paramData.type);
    param.setAttribute("default", paramData.defaultValue);
    param.setAttribute("active", (paramData.active ? "true" : "false") );
    if(paramData.listtype.isEmpty())
      param.appendChild(doc.createTextNode(paramData.description));
    else
    {
      param.setAttribute("listtype", paramData.listtype);

      QDomElement paramDesc = doc.createElement("description");
      paramDesc.appendChild(doc.createTextNode(paramData.description));
      param.appendChild(paramDesc);

      if(paramData.listtype == "dynamic")
      {
        QDomElement paramQuery = doc.createElement("query");
        paramQuery.appendChild(doc.createTextNode(paramData.query));
        param.appendChild(paramQuery);
      }

      if(paramData.listtype == "static")
      {
        for(QList<QPair<QString,QString> >::iterator sit = paramData.values.begin();
              sit != paramData.values.end(); sit++)
        {
          QDomElement paramItem = doc.createElement("item");
          paramItem.setAttribute("value", (*sit).first);
          paramItem.appendChild(doc.createTextNode((*sit).second));
          param.appendChild(paramItem);
        }
      }
    }
    root.appendChild(param);
  }

  if((watermarkUseStaticText() && !watermarkText().isEmpty()) ||
      (!watermarkUseStaticText() && !watermarkQuery().isEmpty() && !watermarkColumn().isEmpty()))
  {
    QDomElement wm = doc.createElement("watermark");
    if(watermarkUseStaticText())
    {
      QDomElement wmtext = doc.createElement("text");
      wmtext.appendChild(doc.createTextNode(watermarkText()));
      wm.appendChild(wmtext);
    }
    else
    {
      QDomElement wmdata = doc.createElement("data");
      QDomElement wmdq = doc.createElement("query");
      wmdq.appendChild(doc.createTextNode(watermarkQuery()));
      wmdata.appendChild(wmdq);
      QDomElement wmdc = doc.createElement("column");
      wmdc.appendChild(doc.createTextNode(watermarkColumn()));
      wmdata.appendChild(wmdc);
      wm.appendChild(wmdata);
    }
    if(!watermarkUseDefaultFont())
    {
      ORGraphicsRectItem::buildXMLFont(doc,wm,watermarkFont());
    }
    QDomElement wmopac = doc.createElement("opacity");
    wmopac.appendChild(doc.createTextNode(QString::number(watermarkOpacity())));
    wm.appendChild(wmopac);
    root.appendChild(wm);
  }

  if(bgEnabled())
  {
    QDomElement bg = doc.createElement("background");
    if(bgStatic())
    {
      QDomElement bgimg = doc.createElement("image");
      bgimg.appendChild(doc.createTextNode(bgImage()));
      bg.appendChild(bgimg);
    }
    else
    {
      QDomElement bgdata = doc.createElement("data");
      QDomElement bgdq = doc.createElement("query");
      bgdq.appendChild(doc.createTextNode(bgQuery()));
      bgdata.appendChild(bgdq);
      QDomElement bgdc = doc.createElement("column");
      bgdc.appendChild(doc.createTextNode(bgColumn()));
      bgdata.appendChild(bgdc);
      bg.appendChild(bgdata);
    }
    QDomElement bgmode = doc.createElement("mode");
    bgmode.appendChild(doc.createTextNode(bgResizeMode()));
    bg.appendChild(bgmode);
    QDomElement bgopac = doc.createElement("opacity");
    bgopac.appendChild(doc.createTextNode(QString::number(bgOpacity())));
    bg.appendChild(bgopac);

    QDomElement bgrect = doc.createElement("rect");
    QDomElement bgrectx = doc.createElement("x");
    bgrectx.appendChild(doc.createTextNode(QString::number(bgBoundsX())));
    bgrect.appendChild(bgrectx);
    QDomElement bgrecty = doc.createElement("y");
    bgrecty.appendChild(doc.createTextNode(QString::number(bgBoundsY())));
    bgrect.appendChild(bgrecty);
    QDomElement bgrectw = doc.createElement("width");
    bgrectw.appendChild(doc.createTextNode(QString::number(bgBoundsWidth())));
    bgrect.appendChild(bgrectw);
    QDomElement bgrecth = doc.createElement("height");
    bgrecth.appendChild(doc.createTextNode(QString::number(bgBoundsHeight())));
    bgrect.appendChild(bgrecth);
    bg.appendChild(bgrect);

    int align = bgAlign();
    // horizontal
    if((align & Qt::AlignRight) == Qt::AlignRight)
      bg.appendChild(doc.createElement("right"));
    else if((align & Qt::AlignHCenter) == Qt::AlignHCenter)
      bg.appendChild(doc.createElement("hcenter"));
    else // Qt::AlignLeft
      bg.appendChild(doc.createElement("left"));
    // vertical
    if((align & Qt::AlignBottom) == Qt::AlignBottom)
      bg.appendChild(doc.createElement("bottom"));
    else if((align & Qt::AlignVCenter) == Qt::AlignVCenter)
      bg.appendChild(doc.createElement("vcenter"));
    else // Qt::AlignTop
      bg.appendChild(doc.createElement("top"));

    root.appendChild(bg);
  }
  
  // Grid
  if(_gridOptions)
  {
      QDomElement grid = doc.createElement("grid");
      root.appendChild(grid);    

      if(_gridOptions->isSnap()) 
      {
          QDomElement gridSnap = doc.createElement("snap");
          grid.appendChild(gridSnap);
      }
      if(_gridOptions->isVisible()) 
      {
          QDomElement gridShow = doc.createElement("show");
          grid.appendChild(gridShow);
      }

      QDomElement gridSizeX = doc.createElement("x");
      gridSizeX.appendChild(doc.createTextNode(QString::number(_gridOptions->xInterval())));
      grid.appendChild(gridSizeX);

      QDomElement gridSizeY = doc.createElement("y");
      gridSizeY.appendChild(doc.createTextNode(QString::number(_gridOptions->yInterval())));
      grid.appendChild(gridSizeY);
  }

  // pageOptions
  // -- size
  QDomElement size = doc.createElement("size");
  if(_pageOptions->getPageSize() == "Custom")
  {
    QDomElement page_width = doc.createElement("width");
    page_width.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getCustomWidth()*100))));
    size.appendChild(page_width);
    QDomElement page_height = doc.createElement("height");
    page_height.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getCustomHeight()*100))));
    size.appendChild(page_height);
  }
  else if(_pageOptions->getPageSize() == "Labels")
  {
    size.appendChild(doc.createTextNode("Labels"));
    QDomElement labeltype = doc.createElement("labeltype");
    labeltype.appendChild(doc.createTextNode(_pageOptions->getLabelType()));
    root.appendChild(labeltype);
  }
  else
  {
    size.appendChild(doc.createTextNode(_pageOptions->getPageSize()));
  }
  root.appendChild(size);
  // -- orientation
  QString str_orientation;
  if(_pageOptions->isPortrait())
  {
    str_orientation = "portrait";
  }
  else
  {
    str_orientation = "landscape";
  }
  root.appendChild(doc.createElement(str_orientation));
  // -- margins
  QDomElement margin;
  margin = doc.createElement("topmargin");
  margin.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getMarginTop()*100))));
  root.appendChild(margin);
  margin = doc.createElement("bottommargin");
  margin.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getMarginBottom()*100))));
  root.appendChild(margin);
  margin = doc.createElement("rightmargin");
  margin.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getMarginRight()*100))));
  root.appendChild(margin);
  margin = doc.createElement("leftmargin");
  margin.appendChild(doc.createTextNode(QString::number((int)(_pageOptions->getMarginLeft()*100))));
  root.appendChild(margin);

  // write out are Query Sources
  QDomElement qsource;
  QDomElement qname;
  QDomElement qsql;
  QDomElement qmgroup;
  QDomElement qmname;
  QuerySource * qs = 0;
  for(unsigned int i = 0; i < qsList->size(); i++)
  {
    qs = qsList->get(i);
    qsource = doc.createElement("querysource");
    qname = doc.createElement("name");
    qname.appendChild(doc.createTextNode(qs->name()));
    qsource.appendChild(qname);
    if(qs->loadFromDb())
    {
      qsource.setAttribute("loadFromDb", "true");
      qmgroup = doc.createElement("mqlgroup");
      qmgroup.appendChild(doc.createTextNode(qs->metaSqlGroup()));
      qsource.appendChild(qmgroup);
      qmname = doc.createElement("mqlname");
      qmname.appendChild(doc.createTextNode(qs->metaSqlName()));
      qsource.appendChild(qmname);
    }
    else
    {
      qsql = doc.createElement("sql");
      qsql.appendChild(doc.createTextNode(qs->query()));
      qsource.appendChild(qsql);
    }
    root.appendChild(qsource);
  }
  qs = 0;

  QDomElement cdef;
  QDomElement cname;
  QDomElement ccomponent;
  QMapIterator<QString, QColor> cit(_colorMap);
  while(cit.hasNext())
  {
    cit.next();
    cdef = doc.createElement("colordef");
    cname = doc.createElement("name");
    cname.appendChild(doc.createTextNode(cit.key()));
    cdef.appendChild(cname);
    ccomponent = doc.createElement("red");
    ccomponent.appendChild(doc.createTextNode(QString::number(cit.value().red())));
    cdef.appendChild(ccomponent);
    ccomponent = doc.createElement("green");
    ccomponent.appendChild(doc.createTextNode(QString::number(cit.value().green())));
    cdef.appendChild(ccomponent);
    ccomponent = doc.createElement("blue");
    ccomponent.appendChild(doc.createTextNode(QString::number(cit.value().blue())));
    cdef.appendChild(ccomponent);
    root.appendChild(cdef);
  }

  QDomElement section;

  // report head
  if(rptHead)
  {
    section = doc.createElement("rpthead");
    rptHead->buildXML(doc, section);
    root.appendChild(section);
  }

  // page head first
  if(pageHeadFirst)
  {
    section = doc.createElement("pghead");
    section.appendChild(doc.createElement("firstpage"));
    pageHeadFirst->buildXML(doc, section);
    root.appendChild(section);
  }
  // page head odd
  if(pageHeadOdd)
  {
    section = doc.createElement("pghead");
    section.appendChild(doc.createElement("odd"));
    pageHeadOdd->buildXML(doc, section);
    root.appendChild(section);
  }
  // page head even
  if(pageHeadEven)
  {
    section = doc.createElement("pghead");
    section.appendChild(doc.createElement("even"));
    pageHeadEven->buildXML(doc, section);
    root.appendChild(section);
  }
  // page head last
  if(pageHeadLast)
  {
    section = doc.createElement("pghead");
    section.appendChild(doc.createElement("lastpage"));
    pageHeadLast->buildXML(doc, section);
    root.appendChild(section);
  }
  // page head any
  if(pageHeadAny)
  {
    section = doc.createElement("pghead");
    pageHeadAny->buildXML(doc, section);
    root.appendChild(section);
  }

  // detail sections
  for(int ii = 0; ii < detailSectionCount(); ii++)
  {
    section = doc.createElement("section");
    getSection(ii)->buildXML(doc,section);
    root.appendChild(section);
  }

  // page foot first
  if(pageFootFirst)
  {
    section = doc.createElement("pgfoot");
    section.appendChild(doc.createElement("firstpage"));
    pageFootFirst->buildXML(doc, section);
    root.appendChild(section);
  }
  // page foot odd
  if(pageFootOdd)
  {
    section = doc.createElement("pgfoot");
    section.appendChild(doc.createElement("odd"));
    pageFootOdd->buildXML(doc, section);
    root.appendChild(section);
  }
  // page foot even
  if(pageFootEven)
  {
    section = doc.createElement("pgfoot");
    section.appendChild(doc.createElement("even"));
    pageFootEven->buildXML(doc, section);
    root.appendChild(section);
  }
  // page foot last
  if(pageFootLast)
  {
    section = doc.createElement("pgfoot");
    section.appendChild(doc.createElement("lastpage"));
    pageFootLast->buildXML(doc, section);
    root.appendChild(section);
  }
  // page foot any
  if(pageFootAny)
  {
    section = doc.createElement("pgfoot");
    pageFootAny->buildXML(doc, section);
    root.appendChild(section);
  }

  // report foot
  if(rptFoot)
  {
    section = doc.createElement("rptfoot");
    rptFoot->buildXML(doc, section);
    root.appendChild(section);
  }

  if(_handler && !_handler->databaseElt().isNull())
  {
    root.appendChild(_handler->databaseElt());
  }

  return doc;
}

void DocumentScene::addColorDef(QString name, QColor col)
{
  // We do not make the document as having been changed when this method
  // is called as it is ued by the routine that loads the document in the
  // first place. This function should not be called to edit the existing
  // document colorMap
  if(name.length() > 0)
    _colorMap[name] = col;
}

void DocumentScene::setWatermarkOpacity(int o)
{
  if(_wmOpacity != o)
  {
    _wmOpacity = o;
    setModified(true);
  }
}

void DocumentScene::setWatermarkFont(QFont f)
{
  if(_wmFont != f)
  {
    _wmFont = f;
    if(!watermarkUseDefaultFont()) 
    {
        setModified(true);    
    }
  }
}

void DocumentScene::setWatermarkUseDefaultFont(bool b)
{
  if(_wmUseDefaultFont != b)
  {
    _wmUseDefaultFont = b;
    setModified(true);
  }
}

void DocumentScene::setWatermarkUseStaticText(bool b)
{
  if(_wmUseStaticText != b)
  {
    _wmUseStaticText = b;
    setModified(true);
  }
}

void DocumentScene::setWatermarkText(QString str)
{
  if(_wmText != str)
  {
    _wmText = str;
    if(watermarkUseStaticText()) 
    {
      setModified(true);    
    }
  }
}

void DocumentScene::setWatermarkColumn(QString str)
{
  if(_wmColumn != str)
  {
    _wmColumn = str;
    setModified(true);
  }
}

void DocumentScene::setWatermarkQuery(QString str)
{
  if(_wmQuery != str)
  {
    _wmQuery = str;
    if(!watermarkUseStaticText()) 
    {
      setModified(true);
    }
  }
}

void DocumentScene::setBgEnabled(bool b)
{
  if(_bgEnabled != b)
  {
    _bgEnabled = b;
    setModified(true);
  }
}

void DocumentScene::setBgStatic(bool b)
{
  if(_bgStatic != b)
  {
    _bgStatic = b;
    setModified(true);
  }
}

void DocumentScene::setBgImage(QString str)
{
  if(_bgImage != str)
  {
    _bgImage = str;
    setModified(true);
  }
}

void DocumentScene::setBgQuery(QString str)
{
  if(_bgQuery != str)
  {
    _bgQuery = str;
    if(!bgStatic()) 
    {
        setModified(true);
    }
  }
}

void DocumentScene::setBgColumn(QString str)
{
  if(_bgColumn != str)
  {
    _bgColumn = str;
    setModified(true);
  }
}

void DocumentScene::setBgResizeMode(QString str)
{
  if(_bgResizeMode != str)
  {
    _bgResizeMode = str;
    setModified(true);
  }
}

void DocumentScene::setBgAlign(int i)
{
  if(_bgAlign != i)
  {
    _bgAlign = i;
    setModified(true);
  }
}

void DocumentScene::setBgBoundsX(int i)
{
  if(_bgBoundsX != i)
  {
    _bgBoundsX = i;
    setModified(true);
  }
}

void DocumentScene::setBgBoundsY(int i)
{
  if(_bgBoundsY != i)
  {
    _bgBoundsY = i;
    setModified(true);
  }
}

void DocumentScene::setBgBoundsWidth(int i)
{
  if(_bgBoundsWidth != i)
  {
    _bgBoundsWidth = i;
    setModified(true);
  }
}

void DocumentScene::setBgBoundsHeight(int i)
{
  if(_bgBoundsHeight != i)
  {
    _bgBoundsHeight = i;
    setModified(true);
  }
}

void DocumentScene::setBgOpacity(int i)
{
  if(_bgOpacity != i)
  {
    _bgOpacity = i;
    setModified(true);
  }
}

void DocumentScene::addDefinedParameter(const QString & n, const ORParameter & d)
{
  _definedParams.insert(n, d);
}

void DocumentScene::editDefinedParameters(QWidget * parent)
{
  ReportParameterList rlist(parent);
  rlist.setList(&_definedParams);
  rlist.exec();
}

double _repositionSection(ORGraphicsSectionItem * gsi, QGraphicsScene* scene, double x, double y, double w)
{
  if(!gsi)
    return 0.0;

  QRectF r;

  if(gsi->scene() != scene)
    scene->addItem(gsi);
  gsi->setPos(x, y);
  r = gsi->rect();
  r.setWidth(w);
  gsi->setRect(r);

  return r.height();
}

void DocumentScene::repositionSections()
{
  static bool working = false;

  if(working)
    return;

  working = true;

  double x, y = 0;
  double w = 0;
  x = _pageMargin->rect().left();
  y = _pageMargin->rect().top();
  w = _pageMargin->rect().width();

  y += _repositionSection(pageHeadFirst, this, x, y, w);
  y += _repositionSection(pageHeadOdd, this, x, y, w);
  y += _repositionSection(pageHeadEven, this, x, y, w);
  y += _repositionSection(pageHeadLast, this, x, y, w);
  y += _repositionSection(pageHeadAny, this, x, y, w);
  y += _repositionSection(rptHead, this, x, y, w);

  ORGraphicsSectionDetailGroup * group;
  int dsc = detailSectionCount();
  for(int dc = 0; dc < dsc; dc++)
  {
    int gsc = getSection(dc)->groupSectionCount();
    int i = 0;
    for(i = 0; i < gsc; i++)
    {
      group = getSection(dc)->getSection(i);
      if(group->isGroupHeadShowing())
        y += _repositionSection(group->getGroupHead(), this, x, y, w);
    }
    y += _repositionSection(getSection(dc)->getDetail(), this, x, y, w);
    for(i = 0; i < gsc; i++)
    {
      group = getSection(dc)->getSection(i);
      if(group->isGroupFootShowing())
        y += _repositionSection(group->getGroupFoot(), this, x, y, w);
    }
  }

  y += _repositionSection(rptFoot, this, x, y, w);
  y += _repositionSection(pageFootFirst, this, x, y, w);
  y += _repositionSection(pageFootOdd, this, x, y, w);
  y += _repositionSection(pageFootEven, this, x, y, w);
  y += _repositionSection(pageFootLast, this, x, y, w);
  y += _repositionSection(pageFootAny, this, x, y, w);

  // TODO
  QRectF newSceneRect = sceneRect();
  if(newSceneRect.bottom() < y+50)
  {
    newSceneRect.setBottom(y+50);
    setSceneRect(newSceneRect);
  }

  working = false;
}

void DocumentScene::deleteSelected()
{
  QList<QGraphicsItem*> list = selectedItems();
  while(!list.isEmpty())
    delete list.takeFirst();

  setModified(true);
}

void DocumentScene::contextMenuEvent(QGraphicsSceneContextMenuEvent * contextMenuEvent)
{
  if(_handler)
  {
    QMenu menu;
    _handler->buildItemContextMenu(&menu);
    if(!menu.isEmpty())
    {
      QPointF o = _handler->getCopyPoint();
      _handler->setCopyPoint(contextMenuEvent->scenePos() - QPointF(-7.0, -7.0));
      menu.exec(contextMenuEvent->screenPos());
      _handler->setCopyPoint(o);
    }
  }
}

void DocumentScene::drawForeground(QPainter * painter, const QRectF & rect)
{
  if(_gridOptions && _gridOptions->isVisible())
  {
    double xi = _gridOptions->xInterval() * 100.0;
    double yi = _gridOptions->yInterval() * 100.0;

      if(yi*_scaledTo <5 && xi*_scaledTo<5)
        return; // don't draw grid if points are too close

    int xs = (int)(rect.left() / xi);
    int ys = (int)(rect.top() / yi);
    int xe = (int)(rect.right() / xi) + 1;
    int ye = (int)(rect.bottom() / yi) + 1;

    painter->save();
    QColor c("gray");
    QPen p(c, 0);
    painter->setPen(p);
    for(int x = xs; x < xe; x++)
    {
      for(int y = ys; y < ye; y++)
      {
        painter->drawPoint(x * xi, y * yi);
      }
    }
    painter->restore();
  }
  QGraphicsScene::drawForeground(painter, rect);
}


void DocumentScene::onSelChanged()
{
    refreshFontToolBox();
    highlightSectionTitles();
}


void DocumentScene::refreshFontToolBox()
{
    QString font, size;
    bool bold = false;
    bool italic = false;

    QList<QGraphicsItem*> selList = selectedItems();

    for(int i = 0; i < selList.count(); i++)
    {
        QString itemFont = getItemFontFamily(selList.at(i));
        if(font.isEmpty()) 
        {
            font = itemFont;
        }
        else if(itemFont != font && !itemFont.isEmpty()) 
        {
            font.clear();
            break;
        }
    }

    for(int i = 0; i < selList.count(); i++)
    {
        QString itemSize = getItemFontSize(selList.at(i));
        if(size.isEmpty()) 
        {
            size = itemSize;
        }
        else if(itemSize != size && !itemSize.isEmpty()) 
        {
            size.clear();
            break;
        }
    }

    for(int i = 0; i < selList.count(); i++)
    {
        if(getItemFontFamily(selList.at(i)).isEmpty())
            continue;

        if (getItemFontWeight(selList.at(i))) 
        {
            bold = true;
        }
        else 
        {
            bold = false;
            break;
        }
    }

    for(int i = 0; i < selList.count(); i++)
    {
        if(getItemFontFamily(selList.at(i)).isEmpty())
            continue;

       if (getItemFontStyle(selList.at(i))) 
        {
            italic = true;
        }
        else 
        {
            italic = false;
            break;
        }
    }

    if(_handler)
    {
        _handler->setFontFamily(font);
        _handler->setFontSize(size);
        _handler->setFontWeight(bold);
        _handler->setFontStyle(italic);
    }
}


void DocumentScene::loadDocument(const QDomElement &root, QWidget *parent, bool loadFromfile)
{
    _parentWindow = parent;
    _loadingInProgress = true;

    clear();
    initData();

    QDomNodeList nlist = root.childNodes();
    QDomNode it;

    for(int i = 0; i < nlist.count(); i++ ) {
        it = nlist.item(i);
        // at this level all the children we get should be Elements
        if(it.isElement()) {
            QString n = it.nodeName();
            if(n == "title") {
                setReportTitle(it.firstChild().nodeValue());
            } else if(n == "name") {
                setReportName(it.firstChild().nodeValue());
            } else if(n == "description") {
                setReportDescription(it.firstChild().nodeValue());
            } else if(n == "parameter") {
                ORParameter param;
                param.name = it.toElement().attribute("name");
                param.type = it.toElement().attribute("type");
                param.defaultValue = it.toElement().attribute("default");
                param.active = (it.toElement().attribute("active") == "true");
                param.listtype = it.toElement().attribute("listtype");
                if(param.listtype.isEmpty())
                    param.description = it.firstChild().nodeValue();
                else
                {
                    QDomNodeList section = it.childNodes();
                    for(int nodeCounter = 0; nodeCounter < section.count(); nodeCounter++)
                    {
                        QDomElement elemThis = section.item(nodeCounter).toElement();
                        if(elemThis.tagName() == "description")
                            param.description = elemThis.text();
                        else if(elemThis.tagName() == "query")
                            param.query = elemThis.text();
                        else if(elemThis.tagName() == "item")
                            param.values.append(qMakePair(elemThis.attribute("value"), elemThis.text()));
                        else
                            qDebug("While parsing parameter encountered an unknown element: %s",elemThis.tagName().toLatin1().constData());
                    }
                }
                addDefinedParameter(param.name, param);
            } else if(n == "watermark") {
                ORWatermarkData wmData;
                parseReportWatermark(it.toElement(), wmData);
                setWatermarkOpacity(wmData.opacity);
                setWatermarkFont(wmData.font);
                setWatermarkText(wmData.text);
                setWatermarkUseDefaultFont(wmData.useDefaultFont);
                setWatermarkUseStaticText(wmData.staticText);
                setWatermarkQuery(wmData.data.query);
                setWatermarkColumn(wmData.data.column);
            } else if(n == "background") {
                ORBackgroundData bgData;
                parseReportBackground(it.toElement(), bgData);
                setBgEnabled(bgData.enabled);
                setBgStatic(bgData.staticImage);
                setBgImage(bgData.image);
                setBgQuery(bgData.data.query);
                setBgColumn(bgData.data.column);
                setBgOpacity(bgData.opacity);
                setBgResizeMode(bgData.mode);
                setBgAlign(bgData.align);
                setBgBoundsX(bgData.rect.x());
                setBgBoundsY(bgData.rect.y());
                setBgBoundsWidth(bgData.rect.width());
                setBgBoundsHeight(bgData.rect.height());
            } else if(n == "grid") {
                QDomNodeList section = it.toElement().childNodes();
                for(int nodeCounter = 0; nodeCounter < section.count(); nodeCounter++) {
                    QDomElement elt = section.item(nodeCounter).toElement();
                    if(elt.tagName() == "snap") {   
                        _gridOptions->setSnap(true);
                    };
                    if(elt.tagName() == "show") {   
                        _gridOptions->setVisible();
                    };
                    if(elt.tagName() == "x") {   
                        _gridOptions->setXInterval(elt.firstChild().nodeValue().toDouble());
                    };
                    if(elt.tagName() == "y") {   
                        _gridOptions->setYInterval(elt.firstChild().nodeValue().toDouble());
                    };
                }
            } else if(n == "size") {
                if(it.firstChild().isText()) {
                    pageOptions()->setPageSize(it.firstChild().nodeValue());
                } else {
                    //bad code! bad code!
                    // this code doesn't check the elemts and assums they are what
                    // they should be.
                    QDomNode n1 = it.firstChild();
                    QDomNode n2 = n1.nextSibling();
                    if(n1.nodeName() == "width") {
                        pageOptions()->setCustomWidth(n1.firstChild().nodeValue().toDouble() / 100.0);
                        pageOptions()->setCustomHeight(n2.firstChild().nodeValue().toDouble() / 100.0);
                    } else {
                        pageOptions()->setCustomWidth(n2.firstChild().nodeValue().toDouble() / 100.0);
                        pageOptions()->setCustomHeight(n1.firstChild().nodeValue().toDouble() / 100.0);
                    }
                    pageOptions()->setPageSize("Custom");
                }
            } else if(n == "labeltype") {
                pageOptions()->setLabelType(it.firstChild().nodeValue());
            } else if(n == "portrait") {
                pageOptions()->setPortrait(true);
            } else if(n == "landscape") {
                pageOptions()->setPortrait(false);
            } else if(n == "topmargin") {
                pageOptions()->setMarginTop(it.firstChild().nodeValue().toDouble() / 100.0);
            } else if(n == "bottommargin") {
                pageOptions()->setMarginBottom(it.firstChild().nodeValue().toDouble() / 100.0);
            } else if(n == "leftmargin") {
                pageOptions()->setMarginLeft(it.firstChild().nodeValue().toDouble() / 100.0);
            } else if(n == "rightmargin") {
                pageOptions()->setMarginRight(it.firstChild().nodeValue().toDouble() / 100.0);
            } else if(n == "querysource") {
                QDomNodeList qnl = it.childNodes();
                QDomElement qde = it.toElement();
                QString qname, qsql, qmgroup, qmname;
                bool lfdb = (qde.attribute("loadFromDb") == "true");
                QDomNode qit;
                for(int qi = 0; qi < qnl.count(); qi++) {
                    qit = qnl.item(qi);
                    if(qit.nodeName() == "name")
                        qname = qit.firstChild().nodeValue();
                    else if(qit.nodeName() == "sql")
                        qsql = qit.firstChild().nodeValue();
                    else if(qit.nodeName() == "mqlgroup")
                        qmgroup = qit.firstChild().nodeValue();
                    else if(qit.nodeName() == "mqlname")
                        qmname = qit.firstChild().nodeValue();
                    else
                        qDebug("While parsing quersource elements encountered unknown node.");
                }
                qsList->add(new QuerySource(qname,qsql, lfdb, qmgroup, qmname));
            } else if(n == "colordef") {
                QDomNodeList qnl = it.childNodes();
                QString cname = QString::null;
                int red = 0, green = 0, blue = 0;
                QDomNode qit;
                for(int qi = 0; qi < qnl.count(); qi++) {
                    qit = qnl.item(qi);
                    if(qit.nodeName() == "name")
                        cname = qit.firstChild().nodeValue();
                    else if(qit.nodeName() == "red")
                        red = qit.firstChild().nodeValue().toInt();
                    else if(qit.nodeName() == "green")
                        green = qit.firstChild().nodeValue().toInt();
                    else if(qit.nodeName() == "blue")
                        blue = qit.firstChild().nodeValue().toInt();
                    if(cname.length() > 0) {
                        addColorDef(cname, QColor(red, green, blue));
                    }
                }
            } else if(n == "rpthead") {
                if(getReportHead() == 0) {
                    insertReportHead();
                    getReportHead()->initFromXML(it);
                } else {
                    qDebug("While loading xml tried to add more than one rpthead");
                }
            } else if(n == "rptfoot") {
                if(getReportFoot() == 0) {
                    insertReportFoot();
                    getReportFoot()->initFromXML(it);
                } else {
                    qDebug("While loading xml tried to add more than one rpthead");
                }
            } else if(n == "pghead") {
                // we need to determine which page this is for
                // firstpage | odd | even | lastpage
                // or any if none was specified
                ORGraphicsSectionItem * rs = 0;
                if(!it.namedItem("firstpage").isNull()) {
                    if(getPageHeadFirst() == 0) {
                        insertPageHeadFirst();
                        rs = getPageHeadFirst();
                    } else {
                        qDebug("tried to load more than one page head first");
                    }
                } else if(!it.namedItem("odd").isNull()) {
                    if(getPageHeadOdd() == 0) {
                        insertPageHeadOdd();
                        rs = getPageHeadOdd();
                    } else {
                        qDebug("tried to load more than one page head odd");
                    }
                } else if(!it.namedItem("even").isNull()) {
                    if(getPageHeadEven() == 0) {
                        insertPageHeadEven();
                        rs = getPageHeadEven();
                    } else {
                        qDebug("tried to load more than one page head even");
                    }
                } else if(!it.namedItem("lastpage").isNull()) {
                    if(getPageHeadLast() == 0) {
                        insertPageHeadLast();
                        rs = getPageHeadLast();
                    } else {
                        qDebug("tried to load more than one page head last");
                    }
                } else {
                    // we have an any pghead
                    if(getPageHeadAny() == 0) {
                        insertPageHeadAny();
                        rs = getPageHeadAny();
                    } else {
                        qDebug("tried to load more than one page head any");
                    }
                }
                if(rs) rs->initFromXML(it);
            } else if(n == "pgfoot") {
                // we need to determine which page this is for
                ORGraphicsSectionItem * rs = 0;
                if(!it.namedItem("firstpage").isNull()) {
                    if(getPageFootFirst() == 0) {
                        insertPageFootFirst();
                        rs = getPageFootFirst();
                    } else {
                        qDebug("tried to load more than one page foot first");
                    }
                } else if(!it.namedItem("odd").isNull()) {
                    if(getPageFootOdd() == 0) {
                        insertPageFootOdd();
                        rs = getPageFootOdd();
                    } else {
                        qDebug("tried to load more than one page foot odd");
                    }
                } else if(!it.namedItem("even").isNull()) {
                    if(getPageFootEven() == 0) {
                        insertPageFootEven();
                        rs = getPageFootEven();
                    } else {
                        qDebug("tried to load more than one page foot even");
                    }
                } else if(!it.namedItem("lastpage").isNull()) {
                    if(getPageFootLast() == 0) {
                        insertPageFootLast();
                        rs = getPageFootLast();
                    } else {
                        qDebug("tried to load more than one page foot last");
                    }
                } else {
                    // we have the any page foot
                    if(getPageFootAny() == 0) {
                        insertPageFootAny();
                        rs = getPageFootAny();
                    } else {
                        qDebug("tried to load more than one page foot any");
                    }
                }
                if(rs) rs->initFromXML(it);
            } else if(n == "section") {
                // we need to load a section.
                ORGraphicsSectionDetail * rsd = new ORGraphicsSectionDetail(this, this);
                rsd->initFromXML(it);
                insertSection(detailSectionCount(),rsd);
            }
            else if(n == "database")
            {
                _handler->loadMemDB(filename, it);
                emit modified();
            } else {
                qDebug("Encountered an unknown Element: %s", n.toLatin1().data());
            }
        }
        else {
            qDebug("Encountered a child node of root that is not an Element");
        }
    }

    pageOptionsChanged();
    if(loadFromfile) { // init stack with a first snapshot to allow undoing of first modification
        _snapshots.clear();
        recordSnapshot();
        setModified(false);
    }
    else {
        setModified(true);
    }
    _loadingInProgress = false;
}



void DocumentScene::updateSectionsOfMovedItems(Qt::KeyboardModifiers keyModifiers)
{
    if(!(keyModifiers & Qt::ShiftModifier))
    {
        QList<QGraphicsItem*> list = selectedItems();
        foreach(QGraphicsItem* item, list)
        {
            ORGraphicsSectionItem * psection = qgraphicsitem_cast<ORGraphicsSectionItem*>(item->parentItem());

            QPointF scenePos = item->mapToScene(item->boundingRect().topLeft()); // NB item->pos() does not match the real current pos of the item
            ORGraphicsSectionItem * csection = qgraphicsitem_cast<ORGraphicsSectionItem*>(getSection(scenePos));

            if(csection && (csection != psection))
            {
                QPointF p = item->scenePos();
                item->setParentItem(csection);
                item->setPos(item->mapToParent(item->mapFromScene(p)));
            }
        }
    }

    highlightSections(ORGraphicsSectionItem::Normal);
	highlightSectionTitles();
}


void DocumentScene::setSectionsColors(QGraphicsItem *item, QPointF lastPos, QPointF newPos, Qt::KeyboardModifiers keyModifiers)
{
	ORGraphicsSectionItem * psection = qgraphicsitem_cast<ORGraphicsSectionItem*>(item->parentItem());
	if(psection)
		psection->highlight(ORGraphicsSectionItem::Origin);
	ORGraphicsSectionItem * lsection = 0;
	ORGraphicsSectionItem * csection = 0;
	QList<QGraphicsItem*> list = items(lastPos);
	for(int i = 0; i < list.count(); i++)
	{
		if(list.at(i)->type() == ORGraphicsSectionItem::Type)
		{
			lsection = qgraphicsitem_cast<ORGraphicsSectionItem*>(list.at(i));
			break;
		}
	}
	list = items(newPos);
	for(int i = 0; i < list.count(); i++)
	{
		if(list.at(i)->type() == ORGraphicsSectionItem::Type)
		{
			csection = qgraphicsitem_cast<ORGraphicsSectionItem*>(list.at(i));
			break;
		}
	}
	if(lsection && (lsection != csection))
		lsection->highlight(ORGraphicsSectionItem::Normal);
	if(!((keyModifiers & Qt::ShiftModifier) == Qt::ShiftModifier)
		&& csection && (csection != psection))
	{
		csection->highlight(ORGraphicsSectionItem::Destination);
	}
	else if(csection)
	{
		csection->highlight(ORGraphicsSectionItem::Normal);
	}
}

void DocumentScene::checkValidity(QWidget * parent)
{
  for(int i = 0; i < detailSectionCount(); i++)
  {
    if(getSection(i)->query().isEmpty())
    {
      QMessageBox::warning( parent, tr("Invalid Document"),
        tr("The detail section %1 is not valid because no query is specified.").arg(getSection(i)->getTitle()));
    }
  }
}

