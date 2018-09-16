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

#include "graphicssection.h"
#include "graphicsitems.h"
#include "documentscene.h"
#include "reportgridoptions.h"

#include <parsexmlutils.h>

//
//ORSectionHandle
//
ORSectionHandle::ORSectionHandle(QGraphicsItem * parent)
  : QGraphicsRectItem(-5, -5, 10, 10, parent)
{
}

void ORSectionHandle::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  if(!parentItem())
    return;

  QPointF scenePos = event->scenePos();
  if(scene() && scene()->inherits("DocumentScene"))
  {
    ReportGridOptions * rgo = 0;
    DocumentScene * ds = static_cast<DocumentScene*>(scene());
    if(ds)
    {
      rgo = ds->gridOptions();
      if(rgo)
        scenePos = rgo->snapPoint(scenePos);
    }
  }

  setPos(pos().x(), qMax(mapToParent(mapFromScene(scenePos)).y(), (qreal)0.0));
}

void ORSectionHandle::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
  // don't need to do anything; just catching the event
}

void ORSectionHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent * /*event*/)
{
  if(!parentItem())
    return;

  // When the mouse is released we want to resize our parent to have
  // a new height according to the position we ended up at
  ORGraphicsSectionItem * sec = static_cast<ORGraphicsSectionItem*>(parentItem());
  bool changed = (sec->rect().height() != qMax(pos().y(), (qreal)0.0));
  sec->setRect(0, 0, sec->rect().width(), qMax(pos().y(), (qreal)0.0));
  DocumentScene * ds = static_cast<DocumentScene*>(sec->scene());
  if(ds)
  {
    if(changed)
      ds->setModified(true);
    ds->repositionSections();
  }
}

//
//ORGraphicsSectionItem
//
ORGraphicsSectionItem::ORGraphicsSectionItem(QGraphicsItem * parent)
  : QGraphicsRectItem(0, 0, 650, 100, parent)
{
  _previousWidth = 0; // set to zero so the first paint will adjust the handle and title into the correct positions
  _handle = new ORSectionHandle(this);
  _title = new QGraphicsSimpleTextItem(QObject::tr("SECTION TITLE"), this);
  _hightLighted = true;
  QPen p(Qt::DashDotLine);
  setPen(p);
  highlight(Normal);
  highlightTitle(false);
}

void ORGraphicsSectionItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  if(rect().width() != _previousWidth)
  {
    _previousWidth = rect().width();
    _title->setPos(rect().width() + 5, 0);
    _handle->setPos(rect().bottomRight());
  }
  QGraphicsRectItem::paint(painter, option, widget);
}

void ORGraphicsSectionItem::highlight(enum Highlight mode)
{
  QPen p = pen();
  p.setWidth(2);
  switch(mode)
  {
    case Origin:
      p.setColor("orange");
      break;
    case Destination:
      p.setColor("green");
      break;
    case Normal:
    default:
      p.setColor("blue");
  };
  setPen(p);
}

void ORGraphicsSectionItem::highlightTitle(bool v)
{
    if(_hightLighted == v)
        return;

    _hightLighted = v;
    QString color = v ? "blue" : "darkgray";
    _title->setBrush(QBrush(QColor(color)));
}

void ORGraphicsSectionItem::setTitle(const QString & s)
{
  _title->setText(s);
}

QString ORGraphicsSectionItem::title() const
{
    if(_title)
            return _title->text();
    else    return "";
}


void ORGraphicsSectionItem::buildXML(QDomDocument & doc, QDomElement & section)
{
  //qDebug("ReportSection::buildXML()");
  float f = rect().height();
  QDomElement height = doc.createElement("height");
  height.appendChild(doc.createTextNode(QString::number(f)));
  section.appendChild(height);

  // now get a list of all the QCanvasItems on this canvas and output them.
  QList<QGraphicsItem *> list = children();
  for(int i = 0; i < list.size(); i++)
  {
    ORGraphicsRectItem::buildXML(list.at(i), doc, section);
  }
}

void ORGraphicsSectionItem::initFromXML(QDomNode & section)
{
  QDomNodeList nl = section.childNodes();
  QDomNode node;
  QString n;
  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "height")
    {
      double h = node.firstChild().nodeValue().toDouble();
      setRect(0, 0, rect().width(), h);
    } else if(n == "label") {
      new ORGraphicsLabelItem(node, this);
    } else if(n == "field") {
      new ORGraphicsFieldItem(node, this);
    } else if(n == "text") {
      new ORGraphicsTextItem(node, this);
    } else if(n == "line") {
      new ORGraphicsLineItem(node, this);
    } else if(n == "rect") {
      new ORGraphicsRectItem(node, this);
    } else if(n == "barcode") {
      new ORGraphicsBarcodeItem(node, this);
    } else if(n == "image") {
      new ORGraphicsImageItem(node, this);
    } else if(n == "graph") {
      new ORGraphicsGraphItem(node, this);
    } else if(n == "crosstab") {
      new ORGraphicsCrossTabItem(node, this);
    } else if(n == "key" || n == "firstpage" || n == "lastpage"
              || n == "odd" || n == "even") {
      // these are all handled elsewhere but we don't want to show errors
      // because they are expected sometimes
    } else {
      qDebug("Encountered unknown node while parsing section: %s", n.toLatin1().constData());
    }
  }
}


//
//ORGraphicsSectionDetailGroup
//
ORGraphicsSectionDetailGroup::ORGraphicsSectionDetailGroup(const QString & title, ORGraphicsSectionDetail * rsd, QObject * parent)
  : QObject(parent), _rsd(rsd)
{
  _pagebreak = BreakNone;
  _head = new ORGraphicsSectionItem();
  _foot = new ORGraphicsSectionItem();
  showGroupHead(false);
  showGroupFoot(false);

  setTitle(title);
}

ORGraphicsSectionDetailGroup::~ORGraphicsSectionDetailGroup()
{
  if(_head)
    delete _head;
  if(_foot)
    delete _foot;
}

void ORGraphicsSectionDetailGroup::setTitle(const QString & s)
{
  if(_name != s) {
    _name = s;
    if(_rsd && _rsd->reportWindow()) _rsd->reportWindow()->setModified(true);
  }
  _head->setTitle(_name + tr(" Group Header"));
  _foot->setTitle(_name + tr(" Group Footer"));
}

void ORGraphicsSectionDetailGroup::setColumn(const QString & s)
{
  if(_column != s) {
    _column = s;
    if(_rsd && _rsd->reportWindow()) _rsd->reportWindow()->setModified(true);
  }
}

void ORGraphicsSectionDetailGroup::showGroupHead(bool yes)
{
  if(isGroupHeadShowing() != yes) {
    if(_rsd && _rsd->reportWindow()) _rsd->reportWindow()->setModified(true);
  }
  _head->setVisible(yes);
  _rsd->adjustSize();
}

void ORGraphicsSectionDetailGroup::showGroupFoot(bool yes)
{
  if(isGroupFootShowing() != yes) {
    if(_rsd && _rsd->reportWindow()) _rsd->reportWindow()->setModified(true);
  }
  _foot->setVisible(yes);
  _rsd->adjustSize();
}

void ORGraphicsSectionDetailGroup::setPageBreak(int pb)
{
  if(_pagebreak != pb)
  {
    _pagebreak = pb;
    if(_rsd && _rsd->reportWindow()) _rsd->reportWindow()->setModified(true);
  }
}

bool ORGraphicsSectionDetailGroup::isGroupHeadShowing() const { return _head->isVisible(); }
bool ORGraphicsSectionDetailGroup::isGroupFootShowing() const { return _foot->isVisible(); }


//
//ORGraphicsSectionDetail
//
ORGraphicsSectionDetail::ORGraphicsSectionDetail(DocumentScene * rptwin, QObject * parent)
  : QObject(parent)
{
  _pagebreak = BreakNone;
  _rw = rptwin;
  _detail = new ORGraphicsSectionItem();
  if(_rw)
    _rw->addItem(_detail);
  setTitle(tr("unnamed"));
}

ORGraphicsSectionDetail::~ORGraphicsSectionDetail()
{
  if(_detail)
    delete _detail;
  while (!groupList.isEmpty())
    delete groupList.takeFirst();
}

void ORGraphicsSectionDetail::setTitle(const QString & s)
{
  if(_name != s) {
    _name = s;
    if(_rw) _rw->setModified(true);
  }
  _detail->setTitle(_name + " Detail");
}

void ORGraphicsSectionDetail::setQuery(const QString & s)
{
  if(_query != s) {
    _query = s;
    if(_rw) _rw->setModified(true);
  }
}

void ORGraphicsSectionDetail::setPageBreak(int pb)
{
  if(_pagebreak != pb)
  {
    _pagebreak = pb;
    if(_rw) _rw->setModified(true);
  }
}

void ORGraphicsSectionDetail::buildXML(QDomDocument & doc, QDomElement & section)
{
  // name/title
  QDomElement name = doc.createElement("name");
  name.appendChild(doc.createTextNode(getTitle()));
  section.appendChild(name);

  if(pageBreak() != ORGraphicsSectionDetail::BreakNone)
  {
    QDomElement spagebreak = doc.createElement("pagebreak");
    if(pageBreak() == ORGraphicsSectionDetail::BreakAtEnd)
      spagebreak.setAttribute("when", "at end");
    section.appendChild(spagebreak);
  }

  for(int i = 0; i < groupList.count(); i++)
  {
    ORGraphicsSectionDetailGroup * rsdg = groupList.at(i);
    QDomNode grp = doc.createElement("group");

    QDomNode gname = doc.createElement("name");
    gname.appendChild(doc.createTextNode(rsdg->getTitle()));
    grp.appendChild(gname);

    QDomNode gcol = doc.createElement("column");
    gcol.appendChild(doc.createTextNode(rsdg->column()));
    grp.appendChild(gcol);

    if(rsdg->pageBreak() != ORGraphicsSectionDetailGroup::BreakNone)
    {
      QDomElement pagebreak = doc.createElement("pagebreak");
      if(rsdg->pageBreak() == ORGraphicsSectionDetailGroup::BreakAfterGroupFooter)
        pagebreak.setAttribute("when", "after foot");
      grp.appendChild(pagebreak);
    }

    //group head
    if(rsdg->isGroupHeadShowing())
    {
      QDomElement ghead = doc.createElement("head");
      rsdg->getGroupHead()->buildXML(doc,ghead);
      grp.appendChild(ghead);
    }
    // group foot
    if(rsdg->isGroupFootShowing())
    {
      QDomElement gfoot = doc.createElement("foot");
      rsdg->getGroupFoot()->buildXML(doc,gfoot);
      grp.appendChild(gfoot);
    }

    section.appendChild(grp);
  }

  // detail section
  QDomElement gdetail = doc.createElement("detail");
  QDomElement key = doc.createElement("key");
  QDomElement kquery = doc.createElement("query");
  kquery.appendChild(doc.createTextNode(query()));
  key.appendChild(kquery);
  gdetail.appendChild(key);
  _detail->buildXML(doc,gdetail);
  section.appendChild(gdetail);
}

void ORGraphicsSectionDetail::initFromXML(QDomNode & section)
{
  QDomNodeList nl = section.childNodes();
  QDomNode node;
  QString n;

  // some code to handle old style defs
  QString o_name = "unnamed";
  QString o_column = QString::null;
  bool old_head = false;
  QDomNode o_head;
  bool old_foot = false;
  QDomNode o_foot;

  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "name") {
      o_name = node.firstChild().nodeValue();
      setTitle(o_name);
    } else if(n == "pagebreak") {
      QDomElement eThis = node.toElement();
      if(eThis.attribute("when") == "at end")
        setPageBreak(BreakAtEnd);
    } else if(n == "group") {
      ORGraphicsSectionDetailGroup * rsdg = new ORGraphicsSectionDetailGroup("unnamed", this, this);
      QDomNodeList gnl = node.childNodes();
      QDomNode gnode;
      bool show_head = false;
      bool show_foot = false;
      for(int gi = 0; gi < gnl.count(); gi++) {
        gnode = gnl.item(gi);
        if(gnode.nodeName() == "name") {
          rsdg->setTitle(gnode.firstChild().nodeValue());
        } else if(gnode.nodeName() == "column") {
          rsdg->setColumn(gnode.firstChild().nodeValue());
        } else if(gnode.nodeName() == "pagebreak") {
          QDomElement elemThis = gnode.toElement();
          QString n = elemThis.attribute("when");
          if("after foot" == n)
            rsdg->setPageBreak(ORGraphicsSectionDetailGroup::BreakAfterGroupFooter);
        } else if(gnode.nodeName() == "head") {
          rsdg->getGroupHead()->initFromXML(gnode);
          rsdg->showGroupHead(true);
          show_head = true;
        } else if(gnode.nodeName() == "foot") {
          rsdg->getGroupFoot()->initFromXML(gnode);
          rsdg->showGroupFoot(true);
          show_foot = true;
        } else {
          qDebug("encountered unknown element while parsing group element: %s", gnode.nodeName().toLatin1().constData());
        }
      }
      insertSection(groupSectionCount(), rsdg);
      rsdg->showGroupHead(show_head);
      rsdg->showGroupFoot(show_foot);
    } else if(n == "grouphead") {
      o_head = node;
      old_head = true;
    } else if(n == "groupfoot") {
      o_foot = node;
      old_foot = true;
    } else if(n == "detail") {
      // need to pull out the query key values
      QDomNode key = node.namedItem("key");
      if(key.isNull()) {
        qDebug("Did not find a key element while parsing detail section");
      } else {
        QDomNodeList knl = key.childNodes();
        QDomNode knode;
        for(int ki = 0; ki < knl.count(); ki++) {
          knode = knl.item(ki);
          if(knode.nodeName() == "query") {
            setQuery(knode.firstChild().nodeValue());
          } else if(knode.nodeName() == "column") {
            o_column = knode.firstChild().nodeValue();
          } else {
            qDebug("encountered unknown element while parsing key element: %s", knode.nodeName().toLatin1().constData());
          }
        }
      }
      _detail->initFromXML(node);
    } else {
      // unknown element
      qDebug("while parsing section encountered and unknown element: %s", n.toLatin1().constData());
    }
  }

  if(old_head || old_foot) {
    ORGraphicsSectionDetailGroup * rsdg = new ORGraphicsSectionDetailGroup(o_name, this, this);

    rsdg->setColumn(o_column);
    if(old_head)
      rsdg->getGroupHead()->initFromXML(o_head);
    if(old_foot)
      rsdg->getGroupFoot()->initFromXML(o_foot);

    // if we encountered this situation then we shouldn't have
    // any other sections but to be sure we will just tack this one
    // onto the end
    insertSection(groupSectionCount(), rsdg);

    rsdg->showGroupHead(old_head);
    rsdg->showGroupFoot(old_foot);
  }
}

int ORGraphicsSectionDetail::groupSectionCount() const
{
  return groupList.count();
}

ORGraphicsSectionDetailGroup * ORGraphicsSectionDetail::getSection(int i)
{
  return groupList.at(i);
}

void ORGraphicsSectionDetail::insertSection(int idx, ORGraphicsSectionDetailGroup * rsd)
{
  groupList.insert(idx, rsd);

  if(_rw) _rw->setModified(true);
  adjustSize();
}

int ORGraphicsSectionDetail::findSection(const QString & name)
{
  ORGraphicsSectionDetailGroup * rsd = 0;
  for(int i = 0; i < groupList.count(); i++) {
    rsd = groupList.at(i);
    if(name == rsd->getTitle()) return i;
  }
  return -1;
}

void ORGraphicsSectionDetail::removeSection(int idx, bool del)
{
  ORGraphicsSectionDetailGroup * rsd = groupList.at(idx);
  groupList.removeAt(idx);
  if(_rw) _rw->setModified(true);
  if(del == true) delete rsd;
  adjustSize();
}

void ORGraphicsSectionDetail::adjustSize()
{
  _rw->repositionSections();
}


