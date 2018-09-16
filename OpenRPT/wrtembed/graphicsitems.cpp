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

#include "graphicsitems.h"

#include "labeleditor.h"
#include "fieldeditor.h"
#include "texteditor.h"
#include "barcodeeditor.h"
#include "imageeditor.h"
#include "grapheditor.h"
#include "reportgridoptions.h"
#include "documentscene.h"
#include "graphicssection.h"
#include "reporthandler.h"
#include "crosstabeditor.h"
#include "patheditor.h"

#include <querysource.h>
#include <quuencode.h>
#include <parsexmlutils.h>
#include <builtinformatfunctions.h>

#include <QRegExp>
#include <QMenu>

int defaultZvalue = 10;
bool __dosnap = true;

void _setModified(QGraphicsScene * scene, bool mod)
{
  if(scene)
  {
    DocumentScene * ds = static_cast<DocumentScene*>(scene);
    if(ds)
      ds->setModified(mod);
  }
}

//
// buildXMLColor
//

static void buildXMLColor(QDomDocument & doc, QDomElement & entity, QColor color, QString name="color")
{
    QDomElement c = doc.createElement(name);

    QDomElement red = doc.createElement("red");
    red.appendChild(doc.createTextNode(QString::number(color.red())));
    c.appendChild(red);

    QDomElement green = doc.createElement("green");
    green.appendChild(doc.createTextNode(QString::number(color.green())));
    c.appendChild(green);

    QDomElement blue = doc.createElement("blue");
    blue.appendChild(doc.createTextNode(QString::number(color.blue())));
    c.appendChild(blue);

    entity.appendChild(c);
}

//
// ORResizeHandle
//
ORResizeHandle::ORResizeHandle(enum Role role, QGraphicsItem * parent)
  : QGraphicsRectItem(0, 0, 10, 10, parent), _moved(false)
{
  _role = role;
  reposition();
}

void ORResizeHandle::reposition()
{
  if(!parentItem())
  {
    return;
  }
  QRectF myself = rect();
  QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(parentItem());
  QRectF rect = parentItem()->boundingRect();
  if(ri)
    rect = ri->rect();
  QGraphicsLineItem * li = static_cast<QGraphicsLineItem*>(parentItem());
  QLineF line;
  if(li)
    line = li->line();

  switch(_role)
  {
    case TopLeft:
      myself.moveCenter(rect.topLeft());
      break;
    case Top:
      myself.moveCenter(QPointF(rect.left() + (rect.width() / 2.0), rect.top()));
      break;
    case TopRight:
      myself.moveCenter(rect.topRight());
      break;
    case Right:
      myself.moveCenter(QPointF(rect.right(), rect.top() + (rect.height() / 2.0)));
      break;
    case BottomRight:
      myself.moveCenter(rect.bottomRight());
      break;
    case Bottom:
      myself.moveCenter(QPointF(rect.left() + (rect.width() / 2.0), rect.bottom()));
      break;
    case BottomLeft:
      myself.moveCenter(rect.bottomLeft());
      break;
    case Left:
      myself.moveCenter(QPointF(rect.left(), rect.top() + (rect.height() / 2.0)));
      break;
    case StartLine:
      myself.moveCenter(line.p1());
      break;
    case EndLine:
      myself.moveCenter(line.p2());
      break;
    default:
      qDebug("ORResizeHandle: Unknown role encountered");
  }
  setRect(myself);
}

void ORResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  if(!parentItem())
  {
    return;
  }
  ORGraphicsRectItem * ri = 0;
  QGraphicsLineItem * li = 0;
  QRectF rect;
  QRectF sceneRect;
  QLineF line;
  QTransform transform;
  qreal rotation = 0;

  if(parentItem()->type() != ORGraphicsLineItem::Type)
  {
    ri = static_cast<ORGraphicsRectItem*>(parentItem());
    if(ri)
    {
      rect = ri->rect();
	  sceneRect = ri->sceneBoundingRect();
      transform = ri->transform();
      rotation = ri->rotation();
    }
  }
  else
  {
    li = static_cast<QGraphicsLineItem*>(parentItem());
    if(li)
    {
        line = li->line();
    }
  }

  QPointF snapPos = event->pos();
  bool snap = false;
  if(scene() && scene()->inherits("DocumentScene"))
  {
    ReportGridOptions * rgo = 0;
    DocumentScene * ds = static_cast<DocumentScene*>(scene());
    if(ds)
    {
      rgo = ds->gridOptions();
      if(rgo && rgo->isSnap()) {
        snapPos = rgo->snapPoint(snapPos);
        snap = true;
      }
    }
  }

  QPointF scenePos = event->scenePos();
  QPointF lastScenePos = event->lastScenePos();
  if(rotation != 0) {
      transform = transform.inverted();
      scenePos = transform.map(scenePos);
      lastScenePos = transform.map(lastScenePos);
  }
  qreal dX = scenePos.x()-lastScenePos.x();
  qreal dY = scenePos.y()-lastScenePos.y();
 
  switch(_role)
  {
    case TopLeft:
      if(!snap)
        rect.adjust(dX,dY,0,0);
	  else
        rect.setTopLeft(snapPos);
      break;
    case Top:
      if(!snap)
        rect.adjust(0,dY,0,0);
	  else
        rect.setTop(snapPos.y());
      break;
    case TopRight:
      if(!snap)
        rect.adjust(0,dY,dX,0);
	  else
        rect.setTopRight(snapPos);
      break;
    case Right:
      if(!snap)
        rect.adjust(0,0,dX,0);
	  else
        rect.setRight(snapPos.x());
      break;
    case BottomRight:
      if(!snap)
        rect.adjust(0,0,dX,dY);
	  else
        rect.setBottomRight(snapPos);
      break;
    case Bottom:
      if(!snap)
        rect.adjust(0,0,0,dY);
	  else
        rect.setBottom(snapPos.y());
      break;
    case BottomLeft:
      if(!snap)
        rect.adjust(dX,0,0,dY);
	  else
        rect.setBottomLeft(snapPos);
      break;
    case Left:
      if(!snap)
        rect.adjust(dX,0,0,0);
	  else
        rect.setLeft(snapPos.x());
      break;
    case StartLine:
      if(!snap)
        line.setP1(line.p1() + QPointF(dX, dY));
      else
        line.setP1(snapPos);
      if((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
        line.setP1(QPointF(line.p1().x(), line.p2().y()));
      else if((event->modifiers() & Qt::AltModifier) == Qt::AltModifier)
		line.setP1(QPointF(line.p2().x(), line.p1().y()));
      break;
    case EndLine:
      if(!snap)
        line.setP2(line.p2() + QPointF(dX, dY));
      else
        line.setP2(snapPos);
      if((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
		line.setP2(QPointF(line.p1().x(), line.p2().y()));
      else if((event->modifiers() & Qt::AltModifier) == Qt::AltModifier)
		line.setP2(QPointF(line.p2().x(), line.p1().y()));
      break;
    default:
      qDebug("ORResizeHandle: Unknown role encountered");
  }

  __dosnap = false;
  if(ri)
  {
      ri->setRect(rect);
  }

  if(li)
  {
    li->setLine(line);
  }

  _moved = true;
  __dosnap = true;

  QGraphicsRectItem::mouseMoveEvent(event);
}

void ORResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
  // simply catching this and not passing it on to the base class is sufficient
  // to accomplish what we need to do and that is to capture the click so
  // now mouseMoveEvents will be received
}

void ORResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(_moved) {
        static_cast<DocumentScene*>(scene())->updateSectionsOfMovedItems(event->modifiers());
        _setModified(scene(), true);
        _moved = false;
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}

//
//ORGraphicsRectItem
//
ORGraphicsRectItem::ORGraphicsRectItem(QGraphicsItem * parent)
  : QGraphicsRectItem(0, 0, 100, 25, parent), _rotation(0), _border(QPen(Qt::black, 0)), _moved(false)
{
  setPen(QPen(Qt::black, 0));
  setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemSendsGeometryChanges);
  setZValue(defaultZvalue-2); 

  _rhTopLeft = new ORResizeHandle(ORResizeHandle::TopLeft, this);
  _rhTop = new ORResizeHandle(ORResizeHandle::Top, this);
  _rhTopRight = new ORResizeHandle(ORResizeHandle::TopRight, this);
  _rhRight = new ORResizeHandle(ORResizeHandle::Right, this);
  _rhBottomRight = new ORResizeHandle(ORResizeHandle::BottomRight, this);
  _rhBottom = new ORResizeHandle(ORResizeHandle::Bottom, this);
  _rhBottomLeft = new ORResizeHandle(ORResizeHandle::BottomLeft, this);
  _rhLeft = new ORResizeHandle(ORResizeHandle::Left, this);

  if(!isSelected())
  {
    _rhTopLeft->setVisible(false);
    _rhTop->setVisible(false);
    _rhTopRight->setVisible(false);
    _rhRight->setVisible(false);
    _rhBottomRight->setVisible(false);
    _rhBottom->setVisible(false);
    _rhBottomLeft->setVisible(false);
    _rhLeft->setVisible(false);
  }
}

ORGraphicsRectItem::ORGraphicsRectItem(const QDomNode & entity, QGraphicsItem * parent)
  : QGraphicsRectItem(0, 0, 100, 25, parent), _rotation(0), _moved(false)
{
  setPen(QPen(Qt::black, 0));
  setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
  setZValue(defaultZvalue-2); 

  _rhTopLeft = new ORResizeHandle(ORResizeHandle::TopLeft, this);
  _rhTop = new ORResizeHandle(ORResizeHandle::Top, this);
  _rhTopRight = new ORResizeHandle(ORResizeHandle::TopRight, this);
  _rhRight = new ORResizeHandle(ORResizeHandle::Right, this);
  _rhBottomRight = new ORResizeHandle(ORResizeHandle::BottomRight, this);
  _rhBottom = new ORResizeHandle(ORResizeHandle::Bottom, this);
  _rhBottomLeft = new ORResizeHandle(ORResizeHandle::BottomLeft, this);
  _rhLeft = new ORResizeHandle(ORResizeHandle::Left, this);

  if(!isSelected())
  {
    _rhTopLeft->setVisible(false);
    _rhTop->setVisible(false);
    _rhTopRight->setVisible(false);
    _rhRight->setVisible(false);
    _rhBottomRight->setVisible(false);
    _rhBottom->setVisible(false);
    _rhBottomLeft->setVisible(false);
    _rhLeft->setVisible(false);
  }

  parseRect(entity);
}

void ORGraphicsRectItem::parseRect(const QDomNode & entity)
{
    QDomNodeList nl = entity.childNodes();
    QDomNode node;
    QString n;
    double sx = 0.0, sy = 0.0, witdh = 0.0, height = 0.0;
    QPen p = pen();
    QBrush b = brush();

    for(int i = 0; i < nl.count(); i++) {
        node = nl.item(i);
        n = node.nodeName();
        if(n == "weight") {
            _border.setWidth(node.firstChild().nodeValue().toInt());
        } else if(n == "color") {
            p.setColor(parseColor(node));
            if(type() == ORGraphicsRectItem::Type)
            {
              _border.setColor(p.color());
            }
        } else if(n == "bgcolor") {
            b.setStyle(Qt::SolidPattern);
            b.setColor(parseColor(node));
        } else if(n == "rotation") {
            setRotation(node.firstChild().nodeValue().toFloat());
        } else if(n == "x") {
            sx = node.firstChild().nodeValue().toDouble();
        } else if(n == "y") {
            sy = node.firstChild().nodeValue().toDouble();
        } else if(n == "width") {
            witdh = node.firstChild().nodeValue().toDouble();
        } else if(n == "height") {
            height = node.firstChild().nodeValue().toDouble();
        } else if(n == "bordercolor") {
            _border.setColor(parseColor(node));
        } else if(n == "borderwidth") {
            _border.setWidth(node.firstChild().nodeValue().toInt());
        } else if(n == "borderstyle") {
            _border.setStyle(static_cast<Qt::PenStyle>(node.firstChild().nodeValue().toInt()));
        } else {
            qDebug("While parsing line encountered unknown element: %s", n.toLatin1().constData());
        }
    }
    setPen(p);

    setBrush(b);
    setPos(sx, sy);
    setRect(0, 0, witdh, height);
}

void ORGraphicsRectItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  Q_UNUSED(option);
  Q_UNUSED(widget)
  if(isSelected())
  {
    // Update the positions of the resize handles if we are selected
    _rhTopLeft->reposition();
    _rhTop->reposition();
    _rhTopRight->reposition();
    _rhRight->reposition();
    _rhBottomRight->reposition();
    _rhBottom->reposition();
    _rhBottomLeft->reposition();
    _rhLeft->reposition();
  }

  painter->save();

//  const qreal pad = 0.5 + _border.widthF()/2;

  if(_border.width()>0 || type() == ORGraphicsRectItem::Type)
  {
    // we have to draw the border by hand
    painter->setPen(_border);
  }
  else
  {
    painter->setPen(Qt::NoPen);
  }
  painter->setBrush(brush());
  painter->drawRect(rect());//.adjusted(pad, pad, -pad, -pad));

  if(isSelected())
  {
    const qreal pad = 0.5;
    QRectF r = rect().adjusted(pad, pad, -pad, -pad);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(Qt::white, 0, Qt::SolidLine));
    painter->drawRect(r);
    painter->setPen(QPen(Qt::black, 0, Qt::DashLine));
    painter->drawRect(r);
  }

  painter->restore();
}

QVariant ORGraphicsRectItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  QVariant nv = value;
  if(QGraphicsItem::ItemSelectedChange == change)
  {
    bool show = value.toBool();
    _rhTopLeft->setVisible(show);
    _rhTop->setVisible(show);
    _rhTopRight->setVisible(show);
    _rhRight->setVisible(show);
    _rhBottomRight->setVisible(show);
    _rhBottom->setVisible(show);
    _rhBottomLeft->setVisible(show);
    _rhLeft->setVisible(show);
  }
  else if(QGraphicsItem::ItemPositionChange == change && __dosnap)
  {
    QPointF scenePos = mapToScene(mapFromParent(value.toPointF()));
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
    nv = QVariant(mapToParent(mapFromScene(scenePos)));
  }
  return QGraphicsRectItem::itemChange(change, nv);
}

void ORGraphicsRectItem::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds && ds->handler())
  {
    QMenu menu;
    ds->handler()->buildItemContextMenu(&menu);
    if(!menu.isEmpty())
    {
      if(!isSelected())
      {
        if(scene())
          scene()->clearSelection();
        setSelected(true);
      }
      QPointF o = ds->handler()->getCopyPoint();
      ds->handler()->setCopyPoint(event->scenePos() - QPointF(-7.0, -7.0));
      menu.exec(event->screenPos());
      ds->handler()->setCopyPoint(o);
    }
  }  

  //QGraphicsRectItem::contextMenuEvent(event);
}

void ORGraphicsRectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
  properties(event->widget());
}

void ORGraphicsRectItem::properties(QWidget * parent)
{
  borderProperties(parent);
}

void ORGraphicsRectItem::borderProperties(QWidget * parent)
{
  PathEditor* dlg = new PathEditor(parent, _border);
  if(dlg->exec() == QDialog::Accepted && dlg->pen() != _border)
  {
    _border = dlg->pen();
    _setModified(scene(), true);
  }
}

void ORGraphicsRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  if(scene())
  {
	static_cast<DocumentScene*>(scene())->setSectionsColors(this, event->lastScenePos(), event->scenePos(), event->modifiers());
    _moved = true;
  }
  QGraphicsRectItem::mouseMoveEvent(event);
}

void ORGraphicsRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
  if(scene())
  {
    if(_moved) {
        static_cast<DocumentScene*>(scene())->updateSectionsOfMovedItems(event->modifiers());
        _setModified(scene(), true);
        _moved = false;
    }
  }

  QGraphicsRectItem::mouseReleaseEvent(event);
}

void ORGraphicsRectItem::buildXML(QDomDocument & doc, QDomElement & entity)
{
  // for compatibility with older versions, the color and width parameters must be written from the pen,
  // even if the rect only uses the border for drawing
  setPen(_border);

  buildXMLCommon(doc, entity);
}

void ORGraphicsRectItem::buildXML(QGraphicsItem * item, QDomDocument & doc, QDomElement & parent)
{
  ORGraphicsRectItem * ri = 0;
  ORGraphicsLineItem * li = 0;
  switch(item->type())
  {
    case ORGraphicsLabelItem::Type:
    case ORGraphicsFieldItem::Type:
    case ORGraphicsTextItem::Type:
    case ORGraphicsRectItem::Type:
    case ORGraphicsBarcodeItem::Type:
    case ORGraphicsImageItem::Type:
    case ORGraphicsGraphItem::Type:
    case ORGraphicsCrossTabItem::Type:
      ri = static_cast<ORGraphicsRectItem*>(item);
      if(ri)
        ri->buildXML(doc, parent);
      break;
    case ORGraphicsLineItem::Type:
      li = static_cast<ORGraphicsLineItem*>(item);
      if(li)
        li->buildXML(doc, parent);
      break;
//    default:
//      qDebug("ReportEntity::buildXML(): unrecognized rtti type");
  };
}


void ORGraphicsRectItem::buildXMLFont(QDomDocument & doc, QDomElement & entity, QFont font)
{
  QDomElement element = doc.createElement("font");

  QDomElement face = doc.createElement("face");
  face.appendChild(doc.createTextNode(font.family()));
  element.appendChild(face);

  QDomElement size = doc.createElement("size");
  size.appendChild(doc.createTextNode(QString::number(font.pointSize())));
  element.appendChild(size);

  QDomElement weight = doc.createElement("weight");
  int w = font.weight();
  if(w == QFont::Normal)
    weight.appendChild(doc.createTextNode("normal"));
  else if(w == QFont::Bold)
    weight.appendChild(doc.createTextNode("bold"));
  else
    weight.appendChild(doc.createTextNode(QString::number(w)));
  element.appendChild(weight);

  if(font.italic()) {
	  QDomElement italic = doc.createElement("italic");
	  element.appendChild(italic);
  }

  entity.appendChild(element);
}


void ORGraphicsRectItem::buildXMLCommon(QDomDocument & doc, QDomElement & entity)
{
    // bounding rectangle  

    QRectF r = QRectF(mapToParent(rect().topLeft()), rect().size());

    QDomElement element = doc.createElement("rect");

    float v = 0.0;

    QDomElement x = doc.createElement("x");
    v = r.x();
    x.appendChild(doc.createTextNode(QString::number(v)));
    element.appendChild(x);
    QDomElement y = doc.createElement("y");
    v = r.y();
    y.appendChild(doc.createTextNode(QString::number(v)));
    element.appendChild(y);
    QDomElement w = doc.createElement("width");
    v = r.width();
    w.appendChild(doc.createTextNode(QString::number(v)));
    element.appendChild(w);
    QDomElement h = doc.createElement("height");
    v = r.height();
    h.appendChild(doc.createTextNode(QString::number(v)));
    element.appendChild(h);
    if(pen().width() != 0) {
        QDomElement wght = doc.createElement("weight");
        wght.appendChild(doc.createTextNode(QString::number(pen().width())));
        element.appendChild(wght);
    }
    if(rotation() != 0) {
        QDomElement rot = doc.createElement("rotation");
        qreal angle = rotation(); 
        rot.appendChild(doc.createTextNode(QString::number(angle)));
        element.appendChild(rot);
    }
    if(_border.width() != 0) {
        QDomElement w = doc.createElement("borderwidth");
        w.appendChild(doc.createTextNode(QString::number(_border.width())));
        element.appendChild(w);
    }
    if(_border.style() != Qt::SolidLine) {
        QDomElement bstyle = doc.createElement("borderstyle");
        bstyle.appendChild(doc.createTextNode(QString::number(_border.style())));
        element.appendChild(bstyle);
    }
    if(_border.color() != Qt::black) {
          buildXMLColor(doc, element, _border.color(), "bordercolor");
    }

    // Pen and brush
    if(pen().color() != Qt::black) {
        buildXMLColor(doc, element, pen().color());
    }
    if(brush().style() != Qt::NoBrush) {
        buildXMLColor(doc, element, brush().color(), "bgcolor");
    }
    entity.appendChild(element);
}


bool ORGraphicsRectItem::_readDefaultFont = false;
QFont ORGraphicsRectItem::_defaultFont = QFont();

QFont ORGraphicsRectItem::getDefaultEntityFont()
{
  if(!_readDefaultFont)
  {
    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenReports");
    _defaultFont.fromString(settings.value("/OpenMFG/rwDefaultEntityFont",_defaultFont.toString()).toString());
    _readDefaultFont = true;
  }
  return _defaultFont;
}

void ORGraphicsRectItem::setDefaultEntityFont(const QFont & f)
{
  _defaultFont = f;
  QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenReports");
  settings.setValue("/OpenMFG/rwDefaultEntityFont", _defaultFont.toString());
  _readDefaultFont = true;
}

void ORGraphicsRectItem::setRotation(qreal angle) 
{    
    _rotation = angle; 
    resetTransform(); 
    rotate(angle); 
}

//
//ORGraphicsLineItem
//
ORGraphicsLineItem::ORGraphicsLineItem(QGraphicsItem * parent)
  : QGraphicsLineItem(0, 0, 100, 0, parent), _moved(false)
{
  setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemSendsGeometryChanges);
  setZValue(defaultZvalue); 

  _rhStartPoint = new ORResizeHandle(ORResizeHandle::StartLine, this);
  _rhEndPoint = new ORResizeHandle(ORResizeHandle::EndLine, this);

  if(!isSelected())
  {
    _rhStartPoint->setVisible(false);
    _rhEndPoint->setVisible(false);
  }
}

ORGraphicsLineItem::ORGraphicsLineItem(const QDomNode & entity, QGraphicsItem * parent)
  : QGraphicsLineItem(0, 0, 100, 0, parent)
{
  setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
  setZValue(defaultZvalue); 

  _rhStartPoint = new ORResizeHandle(ORResizeHandle::StartLine, this);
  _rhEndPoint = new ORResizeHandle(ORResizeHandle::EndLine, this);

  if(!isSelected())
  {
    _rhStartPoint->setVisible(false);
    _rhEndPoint->setVisible(false);
  }

  QDomNodeList nl = entity.childNodes();
  QDomNode node;
  QString n;
  double sx = 0.0, sy = 0.0, ex = 0.0, ey = 0.0;
  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "weight") {
      QPen p = pen();
      p.setWidth(node.firstChild().nodeValue().toInt());
      setPen(p);
    } else if(n == "style") {
      QPen p = pen();
      p.setStyle(static_cast<Qt::PenStyle>(node.firstChild().nodeValue().toInt()));
      setPen(p);
    } else if(n == "xstart") {
      sx = node.firstChild().nodeValue().toDouble();
    } else if(n == "ystart") {
      sy = node.firstChild().nodeValue().toDouble();
    } else if(n == "xend") {
      ex = node.firstChild().nodeValue().toDouble();
    } else if(n == "yend") {
      ey = node.firstChild().nodeValue().toDouble();
    } else if(n == "color") {
      QPen p = pen();
      p.setColor(parseColor(node));
      setPen(p);
    } else {
      qDebug("While parsing line encountered unknown element: %s", n.toLatin1().constData());
    }
  }
  setLine(QLineF(sx, sy, ex, ey));
}

void ORGraphicsLineItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  if(scene())
  {
	static_cast<DocumentScene*>(scene())->setSectionsColors(this, event->lastScenePos(), event->scenePos(), event->modifiers());
  }
  _moved = true;
  QGraphicsLineItem::mouseMoveEvent(event);
}

void ORGraphicsLineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if(scene())
    {
        if(_moved) {
                        static_cast<DocumentScene*>(scene())->updateSectionsOfMovedItems(event->modifiers());
            _setModified(scene(), true);
            _moved = false;
        }
    }

    QGraphicsLineItem::mouseReleaseEvent(event);
}

void ORGraphicsLineItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  QDomElement entity = doc.createElement("line");

  if(pen().color() != Qt::black) {
      buildXMLColor(doc, entity, pen().color());
  }

  double sx, sy, ex, ey;

  QPointF sp = mapToParent(line().p1());
  QPointF ep = mapToParent(line().p2());
  sx = sp.x();
  sy = sp.y();
  ex = ep.x();
  ey = ep.y();

  QDomElement e;

  e = doc.createElement("xstart");
  e.appendChild(doc.createTextNode(QString::number(sx)));
  entity.appendChild(e);

  e = doc.createElement("ystart");
  e.appendChild(doc.createTextNode(QString::number(sy)));
  entity.appendChild(e);

  e = doc.createElement("xend");
  e.appendChild(doc.createTextNode(QString::number(ex)));
  entity.appendChild(e);

  e = doc.createElement("yend");
  e.appendChild(doc.createTextNode(QString::number(ey)));
  entity.appendChild(e);

  // weight
  QDomElement wght = doc.createElement("weight");
  wght.appendChild(doc.createTextNode(QString::number(pen().width())));
  entity.appendChild(wght);

  // style
  if(pen().style() != Qt::SolidLine)
  {
    QDomElement style = doc.createElement("style");
    style.appendChild(doc.createTextNode(QString::number(pen().style())));
    entity.appendChild(style);
  }

  parent.appendChild(entity);
}

void ORGraphicsLineItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  if(isSelected())
  {
    // Update the positions of the resize handles if we are selected
    _rhStartPoint->reposition();
    _rhEndPoint->reposition();
  }

  QGraphicsLineItem::paint(painter, option, widget);
}

void ORGraphicsLineItem::contextMenuEvent(QGraphicsSceneContextMenuEvent * event)
{
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds && ds->handler())
  {
    QMenu menu;
    ds->handler()->buildItemContextMenu(&menu);
    if(!menu.isEmpty())
    {
      if(!isSelected())
      {
        if(scene())
          scene()->clearSelection();
        setSelected(true);
      }
      QPointF o = ds->handler()->getCopyPoint();
      ds->handler()->setCopyPoint(event->scenePos() - QPointF(-7.0, -7.0));
      menu.exec(event->screenPos());
      ds->handler()->setCopyPoint(o);
    }
  }  

  //QGraphicsLineItem::contextMenuEvent(event);
}

void ORGraphicsLineItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
  properties(event->widget());
}

void ORGraphicsLineItem::properties(QWidget * parent)
{
  PathEditor* dlg = new PathEditor(parent, pen());
  if(dlg->exec() == QDialog::Accepted)
  {
    setPen(dlg->pen());
  }
}

QRectF ORGraphicsLineItem::boundingRect() const
{
  return shape().controlPointRect();
}

QPainterPath ORGraphicsLineItem::shape() const
{
  QPainterPath path;

  path.moveTo(line().p1());
  path.lineTo(line().p2());

  QPainterPathStroker ps;

  qreal minWidth = 7; // a semi-arbitrary minimum width for easier line grabbing

  if (pen().widthF() <= minWidth)
    ps.setWidth(minWidth);
  else
    ps.setWidth(pen().widthF());

  QPainterPath res = ps.createStroke(path);
  res.addPath(path);
  return res;
}


QVariant ORGraphicsLineItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  QVariant nv = value;
  if (QGraphicsItem::ItemSelectedChange == change)
  {
    bool show = value.toBool();
    _rhStartPoint->setVisible(show);
    _rhEndPoint->setVisible(show);
  }
  else if(QGraphicsItem::ItemPositionChange == change && __dosnap)
  {
    QPointF scenePos = mapToScene(mapFromParent(value.toPointF()));
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
    nv = QVariant(mapToParent(mapFromScene(scenePos)));
  }
  return QGraphicsLineItem::itemChange(change, nv);
}


//
//ORGraphicsLabelItem
//
ORGraphicsLabelItem::ORGraphicsLabelItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue);
  _font = getDefaultEntityFont();
  _txt = QObject::tr("Label");
  _flags = 0;

  // TODO: add code to calculate text size and make default item size equal to that
}

ORGraphicsLabelItem::ORGraphicsLabelItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  _font = getDefaultEntityFont();
  _flags = 0;
  setZValue(defaultZvalue); 

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  for(int i = 0; i < nl.count(); i++)
  {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "string") {
      _txt = node.firstChild().nodeValue();
    } else if(n == "left") {
      _flags |= Qt::AlignLeft;
    } else if(n == "hcenter") {
      _flags |= Qt::AlignHCenter;
    } else if(n == "right") {
      _flags |= Qt::AlignRight;
    } else if(n == "top") {
      _flags |= Qt::AlignTop;
    } else if(n == "vcenter") {
      _flags |= Qt::AlignVCenter;
    } else if(n == "bottom") {
      _flags |= Qt::AlignBottom;
    } else if(n == "rect") {
      parseRect(node);
    } else if(n == "font") {
      QDomNodeList fnl = node.childNodes();
      for(int fi = 0; fi < fnl.count(); fi++) {
        node = fnl.item(fi);
        n = node.nodeName();
        if(n == "face") {
          _font.setFamily(node.firstChild().nodeValue());
        } else if(n == "size") {
          _font.setPointSize(node.firstChild().nodeValue().toInt());
        } else if(n == "weight") {
          QString v = node.firstChild().nodeValue();
          if(v == "bold") _font.setBold(true);
          else if(v == "normal") _font.setBold(false);
          else _font.setWeight(node.firstChild().nodeValue().toInt());
		} else if(n == "italic") {
		  _font.setItalic(true);
        } else {
          qDebug("while parsing font encountered unknown element: %s", n.toLatin1().constData());
        }
      }
    } else {
      qDebug("while parsing label element encountered unknow element: %s",n.toLatin1().constData());
    }
  }
}

void ORGraphicsLabelItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  //qDebug("ReportEntityLabel::buildXML()");
  QDomElement entity = doc.createElement("label");

  buildXMLCommon(doc,entity);
  // font info
  buildXMLFont(doc,entity,font());

  // text alignment
  int align = textFlags();
  // horizontal
  if((align & Qt::AlignRight) == Qt::AlignRight)
    entity.appendChild(doc.createElement("right"));
  else if((align & Qt::AlignHCenter) == Qt::AlignHCenter)
    entity.appendChild(doc.createElement("hcenter"));
  else // Qt::AlignLeft
    entity.appendChild(doc.createElement("left"));
  // vertical
  if((align & Qt::AlignBottom) == Qt::AlignBottom)
    entity.appendChild(doc.createElement("bottom"));
  else if((align & Qt::AlignVCenter) == Qt::AlignVCenter)
    entity.appendChild(doc.createElement("vcenter"));
  else // Qt::AlignTop
    entity.appendChild(doc.createElement("top"));

  // the text string
  QDomElement string = doc.createElement("string");
  string.appendChild(doc.createTextNode(text()));
  entity.appendChild(string);

  parent.appendChild(entity);
}

void ORGraphicsLabelItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setFont(_font);
  painter->setPen(pen());
  painter->setBrush(brush());
  painter->drawText(rect(), _flags, _txt);

  painter->restore();

}

void ORGraphicsLabelItem::properties(QWidget * parent)
{
  LabelEditor * le = new LabelEditor(parent);
  le->setFont(font());
  le->tbText->setText(text());
  le->setLabelFlags(textFlags());
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));
  if(le->exec() == QDialog::Accepted) {
      setFont(le->font());
      setText(le->text());
      setTextFlags(le->alignment());

      double dt;
      bool ok;
      dt = le->leXPos->text().toDouble(&ok);
      if(ok) dx = dt * 100.0;
      dt = le->leYPos->text().toDouble(&ok);
      if(ok) dy = dt * 100.0;
      dt = le->leWidth->text().toDouble(&ok);
      if(ok) dw = dt * 100.0;
      dt = le->leHeight->text().toDouble(&ok);
      if(ok) dh = dt * 100.0;

      if(pos().x() != dx || pos().y() != dy)
      {
          setPos(dx, dy);
          _setModified(scene(), true);
      }
      if(rect().width() != dw || rect().height() != dh) {
          setRect(0, 0, dw, dh);
          _setModified(scene(), true);
      }

      update();
  }
}

void ORGraphicsLabelItem::setText(const QString& t)
{
  if(_txt != t)
  {
    _txt = t;
    _setModified(scene(), true);
  }
  update();
}

void ORGraphicsLabelItem::setFont(const QFont& f)
{
  if(_font != f)
  {
    _font = f;
    _setModified(scene(), true);
    static_cast<DocumentScene*>(scene())->refreshFontToolBox();
  }
  update();
}

void ORGraphicsLabelItem::setTextFlags(int tf)
{
  if(_flags != tf)
  {
    _flags = tf;
    _setModified(scene(), true);
  }
  update();
}

//
//ORGraphicsFieldItem
//
ORGraphicsFieldItem::ORGraphicsFieldItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  _flags = 0;
  setZValue(defaultZvalue); 
  _font = getDefaultEntityFont();
  _trackTotal = false;
  _trackBuiltinFormat = false;
  _useSubTotal = false;
  _lines = 1;
  _columns = 1;
  _xSpacing = 0;
  _ySpacing = 0;
  _triggerPageBreak = false;
  _leftToRight = false;
}

ORGraphicsFieldItem::ORGraphicsFieldItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  _flags = 0;
  setZValue(defaultZvalue); 
  _font = getDefaultEntityFont();
  _trackTotal = false;
  _trackBuiltinFormat = false;
  _useSubTotal = false;
  _lines = 1;
  _columns = 1;
  _xSpacing = 0;
  _ySpacing = 0;
  _triggerPageBreak = false;
  _leftToRight = false;

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  for(int i = 0; i < nl.count(); i++)
  {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "data") {
      QDomNodeList dnl = node.childNodes();
      for(int di = 0; di < dnl.count(); di++) {
        node = dnl.item(di);
        n = node.nodeName();
        if(n == "query") {
          _qry = node.firstChild().nodeValue();
        } else if(n == "column") {
          _clmn = node.firstChild().nodeValue();
        } else {
          qDebug("while parsing field data encountered and unknown element: %s", n.toLatin1().constData());
        }
      }
      //txt = node.firstChild().nodeValue();
    } else if(n == "left") {
      _flags |= Qt::AlignLeft;
    } else if(n == "hcenter") {
      _flags |= Qt::AlignHCenter;
    } else if(n == "right") {
      _flags |= Qt::AlignRight;
    } else if(n == "top") {
      _flags |= Qt::AlignTop;
    } else if(n == "vcenter") {
      _flags |= Qt::AlignVCenter;
    } else if(n == "bottom") {
      _flags |= Qt::AlignBottom;
    } else if(n == "wordwrap") {
      _flags |= Qt::TextWordWrap;
    } else if(n == "lines") {
      _lines = node.firstChild().nodeValue().toInt();
    } else if(n == "columns") {
      _columns = node.firstChild().nodeValue().toInt();
    } else if(n == "xSpacing") {
      _xSpacing = node.firstChild().nodeValue().toDouble();
    } else if(n == "ySpacing") {
      _ySpacing = node.firstChild().nodeValue().toDouble();
    } else if(n == "triggerPageBreak") {
      _triggerPageBreak = true;
    } else if(n == "leftToRight") {
      _leftToRight = true;
    } else if(n == "rect") {
      parseRect(node);
    } else if(n == "font") {
      QDomNodeList fnl = node.childNodes();
      for(int fi = 0; fi < fnl.count(); fi++) {
        node = fnl.item(fi);
        n = node.nodeName();
        if(n == "face") {
          _font.setFamily(node.firstChild().nodeValue());
        } else if(n == "size") {
          _font.setPointSize(node.firstChild().nodeValue().toInt());
        } else if(n == "weight") {
          QString v = node.firstChild().nodeValue();
          if(v == "bold") _font.setBold(true);
          else if(v == "normal") _font.setBold(false);
          else _font.setWeight(node.firstChild().nodeValue().toInt());
		} else if(n == "italic") {
			_font.setItalic(true);
        } else {
          qDebug("while parsing font encountered unknown element: %s", n.toLatin1().constData());
        }
      }
    } else if(n == "format") {
      _trackBuiltinFormat = (node.toElement().attribute("builtin")=="true"?true:false);
      _format = node.firstChild().nodeValue();
    } else if(n == "tracktotal") {
        // NB for compatibility with reports V <= 3.0 format info is also read from the total tag
                if(!_trackBuiltinFormat)
          _trackBuiltinFormat = (node.toElement().attribute("builtin")=="true"?true:false);
        _useSubTotal = (node.toElement().attribute("subtotal")=="true"?true:false);
        if(!node.firstChild().nodeValue().isEmpty())
            _format = node.firstChild().nodeValue();
        if(_format.length() > 0) _trackTotal = true;
    } else {
      qDebug("while parsing field element encountered unknow element: %s",n.toLatin1().data());
    }
  }
}

void ORGraphicsFieldItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  //qDebug("ReportEntityField::buildXML()");
  QDomElement entity = doc.createElement("field");

  buildXMLCommon(doc,entity);
  // font info
  buildXMLFont(doc,entity,font());

  // text alignment
  int align = textFlags();

  if((align & Qt::TextWordWrap) == Qt::TextWordWrap)
    entity.appendChild(doc.createElement("wordwrap"));

  // horizontal
  if((align & Qt::AlignRight) == Qt::AlignRight)
    entity.appendChild(doc.createElement("right"));
  else if((align & Qt::AlignHCenter) == Qt::AlignHCenter)
    entity.appendChild(doc.createElement("hcenter"));
  else // Qt::AlignLeft
    entity.appendChild(doc.createElement("left"));
  // vertical
  if((align & Qt::AlignBottom) == Qt::AlignBottom)
    entity.appendChild(doc.createElement("bottom"));
  else if((align & Qt::AlignVCenter) == Qt::AlignVCenter)
    entity.appendChild(doc.createElement("vcenter"));
  else // Qt::AlignTop
    entity.appendChild(doc.createElement("top"));

  // the field data
  QDomElement data = doc.createElement("data");
  QDomElement dquery = doc.createElement("query");
  dquery.appendChild(doc.createTextNode(query()));
  data.appendChild(dquery);
  QDomElement dcolumn = doc.createElement("column");
  dcolumn.appendChild(doc.createTextNode(column()));
  data.appendChild(dcolumn);
  entity.appendChild(data);

  // formatting
  if(!_format.isEmpty()) {
    QDomElement format = doc.createElement("format");
    if(_trackBuiltinFormat)
      format.setAttribute("builtin","true");
    format.appendChild(doc.createTextNode(_format));
    entity.appendChild(format);
  }

  if(_trackTotal) {
    QDomElement tracktotal = doc.createElement("tracktotal");
    if(_useSubTotal)
      tracktotal.setAttribute("subtotal","true");
    entity.appendChild(tracktotal);
  }

  if(_lines != 1) {
      QDomElement e = doc.createElement("lines");
      e.appendChild(doc.createTextNode(QString::number(_lines)));
      entity.appendChild(e);
  }
  if(_columns != 1) {
      QDomElement e = doc.createElement("columns");
      e.appendChild(doc.createTextNode(QString::number(_columns)));
      entity.appendChild(e);
  }
  if(_xSpacing != 0) {
      QDomElement e = doc.createElement("xSpacing");
      e.appendChild(doc.createTextNode(QString::number(_xSpacing)));
      entity.appendChild(e);
  }
  if(_ySpacing != 0) {
      QDomElement e = doc.createElement("ySpacing");
      e.appendChild(doc.createTextNode(QString::number(_ySpacing)));
      entity.appendChild(e);
  }
  if(_triggerPageBreak) {
      entity.appendChild(doc.createElement("triggerPageBreak"));
  }
  if(_leftToRight) {
      entity.appendChild(doc.createElement("leftToRight"));
  }

  parent.appendChild(entity);
}

void ORGraphicsFieldItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setFont(_font);
  painter->setPen(pen());
  painter->setBrush(brush());
  QString text = _clmn+QObject::tr(":")+_qry+(_trackTotal? QObject::tr(" field total"):QObject::tr(" field"));
  painter->drawText(rect(), _flags, text);

  QColor colorForOtherCells(pen().color());
  colorForOtherCells.setAlphaF(0.3);
  painter->setPen(QPen(colorForOtherCells));

  for (int line=0; line<_lines; line++) {
    for (int col=0; col<_columns; col++) {
      if(col != 0 || line != 0) {
        QRectF r = rect();
        r.translate((100*_xSpacing + r.width())*col, (100*_ySpacing + r.height())*line);
        painter->drawText(r, _flags, text);
      }
    }
  }

  painter->restore();

}

QRectF ORGraphicsFieldItem::boundingRect () const
{
  QRectF r = QGraphicsRectItem::boundingRect();
  r.setHeight(r.height()*_lines + 100*_ySpacing*(_lines-1));
  r.setWidth(r.width()*_columns + 100*_xSpacing*(_columns-1));
  return r;
}

void ORGraphicsFieldItem::properties(QWidget * parent)
{
  FieldEditor * le = new FieldEditor(parent);
  le->labelPreview->setFont(font());
  
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds)
    le->cbQuery->init(ds->qsList,query());

  le->tbColumn->setText(column());
  le->_cbRTotal->setChecked(_trackTotal);
  if(_trackBuiltinFormat) {
    le->_rbBuiltinFormat->setChecked(true);
    le->_cbBuiltinFormat->insertItem(0, getNameFromTag(_format));
    le->_cbBuiltinFormat->setCurrentIndex(0);
  } else {
    le->_rbStringFormat->setChecked(true);
    le->_leRTotalFormat->setText(_format);
  }
  le->_cbSubTotal->setChecked(_useSubTotal);
  le->setLabelFlags(textFlags());
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));
  le->leLines->setText(QString::number(_lines));
  le->leColumns->setText(QString::number(_columns));
  le->leXSpacing->setText(QString::number(_xSpacing));
  le->leYSpacing->setText(QString::number(_ySpacing));
  le->_cbPageBreak->setChecked(_triggerPageBreak);
  le->_cbLeftToRight->setChecked(_leftToRight);

  if(le->exec() == QDialog::Accepted)
  {
    setFont(le->labelPreview->font());
    setQuery(le->cbQuery->currentQuery());
    setColumn(le->tbColumn->text());
    setTrackTotal(le->_cbRTotal->isChecked());
    if(trackTotal()) {
      setUseSubTotal(le->_cbSubTotal->isChecked());
    }
    if(le->_rbStringFormat->isChecked()) {
        setFormat(le->_leRTotalFormat->text(), false);
        if(format().isEmpty() && (trackTotal() || useSubTotal())) {
            setFormat("%0.2f"); // a default format is necessary to correctly handle the totalization
        }
    } else {
        setFormat(getTagFromName(le->_cbBuiltinFormat->currentText()), true);
    }
    if(le->labelPreview->wordWrap())
      setTextFlags(le->labelPreview->alignment() | Qt::TextWordWrap);
    else
      setTextFlags(le->labelPreview->alignment());

    setArray(le->leLines->text().toInt(),       le->leColumns->text().toInt(),
             le->leXSpacing->text().toDouble(), le->leYSpacing->text().toDouble(),
             le->_cbPageBreak->isChecked(),    le->_cbLeftToRight->isChecked() );

    double dt;
    bool ok;
    dt = le->leXPos->text().toDouble(&ok);
    if(ok) dx = dt * 100.0;
    dt = le->leYPos->text().toDouble(&ok);
    if(ok) dy = dt * 100.0;
    dt = le->leWidth->text().toDouble(&ok);
    if(ok) dw = dt * 100.0;
    dt = le->leHeight->text().toDouble(&ok);
    if(ok) dh = dt * 100.0;

    if(pos().x() != dx || pos().y() != dy) {
      setPos(dx, dy);
      _setModified(scene(), true);
    }
    if(rect().width() != dw || rect().height() != dh) {
      setRect(0, 0, dw, dh);
      _setModified(scene(), true);
    }

    update();
  }
}

void ORGraphicsFieldItem::setQuery(const QString& q)
{
  if(_qry != q)
  {
    _qry = q;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsFieldItem::setColumn(const QString& c)
{
  if(_clmn != c)
  {
    _clmn = c;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsFieldItem::setFont(const QFont& f)
{
  if(_font != f)
  {
    _font = f;
    _setModified(scene(), true);
    static_cast<DocumentScene*>(scene())->refreshFontToolBox();
    update();
  }
}

void ORGraphicsFieldItem::setTextFlags(int f)
{
  if(_flags != f)
  {
    _flags = f;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsFieldItem::setArray(int lines, int columns, double xSpacing, double ySpacing, bool pageBreak, bool leftToRight)
{
    if(     _lines != lines || _columns != columns
        ||  _xSpacing != xSpacing || _ySpacing != ySpacing
        ||  _triggerPageBreak != pageBreak || _leftToRight != leftToRight )
    {
        _lines = lines;
        _columns = columns;
        _xSpacing = xSpacing;
        _ySpacing = ySpacing;
        _triggerPageBreak = pageBreak;
        _leftToRight = leftToRight;
        _setModified(scene(), true);
        update();
    }
}


void ORGraphicsFieldItem::setTrackTotal(bool tt)
{
  if(_trackTotal != tt)
  {
    _trackTotal = tt;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsFieldItem::setFormat(const QString & str, bool builtin)
{
  if(_trackBuiltinFormat != builtin || _format != str)
  {
    _trackBuiltinFormat = builtin;
    _format = str;
    _setModified(scene(), true);
  }

}

void ORGraphicsFieldItem::setUseSubTotal(bool yes)
{
  if(_useSubTotal != yes)
  {
    _useSubTotal = yes;
    _setModified(scene(), true);
  }
}



//
//ORGraphicsTextItem
//
ORGraphicsTextItem::ORGraphicsTextItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  _flags = 0;
  setZValue(defaultZvalue); 
  _bpad = 0.0;
  _font = getDefaultEntityFont();
}

ORGraphicsTextItem::ORGraphicsTextItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  _flags = 0;
  setZValue(defaultZvalue); 
  _bpad = 0.0;
  _font = getDefaultEntityFont();

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "data") {
      QDomNodeList dnl = node.childNodes();
      for(int di = 0; di < dnl.count(); di++) {
        node = dnl.item(di);
        n = node.nodeName();
        if(n == "query") {
          _qry = node.firstChild().nodeValue();
        } else if(n == "column") {
          _clmn = node.firstChild().nodeValue();
        } else {
          qDebug("while parsing field data encountered and unknown element: %s", n.toLatin1().constData());
        }
      }
    } else if(n == "bottompad") {
      _bpad = node.firstChild().nodeValue().toDouble() / 100.0;
    } else if(n == "left") {
      _flags |= Qt::AlignLeft;
    } else if(n == "hcenter") {
      _flags |= Qt::AlignHCenter;
    } else if(n == "right") {
      _flags |= Qt::AlignRight;
    } else if(n == "top") {
      _flags |= Qt::AlignTop;
    } else if(n == "vcenter") {
      _flags |= Qt::AlignVCenter;
    } else if(n == "bottom") {
      _flags |= Qt::AlignBottom;
    } else if(n == "rect") {
      parseRect(node);
    } else if(n == "font") {
      QDomNodeList fnl = node.childNodes();
      for(int fi = 0; fi < fnl.count(); fi++) {
        node = fnl.item(fi);
        n = node.nodeName();
        if(n == "face") {
          _font.setFamily(node.firstChild().nodeValue());
        } else if(n == "size") {
          _font.setPointSize(node.firstChild().nodeValue().toInt());
        } else if(n == "weight") {
          QString v = node.firstChild().nodeValue();
          if(v == "bold") _font.setBold(true);
          else if(v == "normal") _font.setBold(false);
          else _font.setWeight(node.firstChild().nodeValue().toInt());
		} else if(n == "italic") {
			_font.setItalic(true);
        } else {
          qDebug("while parsing font encountered unknown element: %s", n.toLatin1().constData());
        }
      }
    } else {
      qDebug("while parsing text element encountered unknow element: %s",n.toLatin1().constData());
    }
  }
}

void ORGraphicsTextItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  //qDebug("ReportEntityText::buildXML()");
  QDomElement entity = doc.createElement("text");

  buildXMLCommon(doc,entity);
  // bottompad
  QDomElement bottompad = doc.createElement("bottompad");
  double h = _bpad * 100.0;
  bottompad.appendChild(doc.createTextNode(QString::number((int)h)));
  entity.appendChild(bottompad);
  // font info
  buildXMLFont(doc,entity,font());

  // text alignment
  int align = textFlags();
  // horizontal
  if((align & Qt::AlignRight) == Qt::AlignRight)
    entity.appendChild(doc.createElement("right"));
  else if((align & Qt::AlignHCenter) == Qt::AlignHCenter)
    entity.appendChild(doc.createElement("hcenter"));
  else // Qt::AlignLeft
    entity.appendChild(doc.createElement("left"));
  // vertical
  if((align & Qt::AlignBottom) == Qt::AlignBottom)
    entity.appendChild(doc.createElement("bottom"));
  else if((align & Qt::AlignVCenter) == Qt::AlignVCenter)
    entity.appendChild(doc.createElement("vcenter"));
  else // Qt::AlignTop
    entity.appendChild(doc.createElement("top"));

  // the field data
  QDomElement data = doc.createElement("data");
  QDomElement dquery = doc.createElement("query");
  dquery.appendChild(doc.createTextNode(query()));
  data.appendChild(dquery);
  QDomElement dcolumn = doc.createElement("column");
  dcolumn.appendChild(doc.createTextNode(column()));
  data.appendChild(dcolumn);
  entity.appendChild(data);

  parent.appendChild(entity);
}

void ORGraphicsTextItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setFont(_font);
  painter->setPen(pen());
  painter->setBrush(brush());
  painter->drawText(rect(), _flags, _clmn+QObject::tr(":")+_qry+QObject::tr(" textarea"));

  painter->restore();
}

void ORGraphicsTextItem::properties(QWidget * parent)
{
  TextEditor * le = new TextEditor(parent);
  le->labelPreview->setFont(font());
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds)
    le->cbQuery->init(ds->qsList,query());

  le->tbColumn->setText(column());
  le->setLabelFlags(textFlags());
  le->tbBottompad->setText(QString::number(_bpad,'f',3));
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));
  if(le->exec() == QDialog::Accepted)
  {
    setFont(le->labelPreview->font());
    setQuery(le->cbQuery->currentQuery());
    setColumn(le->tbColumn->text());
    setTextFlags(le->labelPreview->alignment());
    setBottomPadding(le->tbBottompad->text().toDouble());

    double dt;
    bool ok;
    dt = le->leXPos->text().toDouble(&ok);
    if(ok) dx = dt * 100.0;
    dt = le->leYPos->text().toDouble(&ok);
    if(ok) dy = dt * 100.0;
    dt = le->leWidth->text().toDouble(&ok);
    if(ok) dw = dt * 100.0;
    dt = le->leHeight->text().toDouble(&ok);
    if(ok) dh = dt * 100.0;

    if(pos().x() != dx || pos().y() != dy)
    {
      setPos(dx, dy);
      _setModified(scene(), true);
    }
    if(rect().width() != dw || rect().height() != dh)
    {
      setRect(0, 0, dw, dh);
      _setModified(scene(), true);
    }

    update();
  }
}

void ORGraphicsTextItem::setBottomPadding(double bp)
{
  if(_bpad != bp)
  {
    _bpad = bp;
    _setModified(scene(), true);
  }
}

void ORGraphicsTextItem::setQuery(const QString& q)
{
  if(_qry != q)
  {
    _qry = q;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsTextItem::setColumn(const QString& c)
{
  if(_clmn != c)
  {
    _clmn = c;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsTextItem::setFont(const QFont& f)
{
  if(_font != f)
  {
    _font = f;
    _setModified(scene(), true);
    static_cast<DocumentScene*>(scene())->refreshFontToolBox();
    update();
  }
}

void ORGraphicsTextItem::setTextFlags(int f)
{
  if(_flags != f)
  {
    _flags = f;
    _setModified(scene(), true);
    update();
  }
}


//
//ORGraphicsBarcodeItem
//
ORGraphicsBarcodeItem::ORGraphicsBarcodeItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  _frmt = QObject::tr("3of9");
  _align = 0;
  _narrowBarWidth = ORBarcodeData::defaultNarrowBarWidth();
  setMaxLength(5);
  setRect(0, 0, (_min_width_total*100.0), (_min_height*100.0));
}

ORGraphicsBarcodeItem::ORGraphicsBarcodeItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  _frmt = QObject::tr("3of9");
  _align = 0;
  _narrowBarWidth = ORBarcodeData::defaultNarrowBarWidth();
  setMaxLength(5);
  setRect(0, 0, (_min_width_total*100.0), (_min_height*100.0));

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "data") {
      // see "string" just below for comments on String vs. Data
      QDomNodeList dnl = node.childNodes();
      for(int di = 0; di < dnl.count(); di++) {
        node = dnl.item(di);
        n = node.nodeName();
        if(n == "query") {
          _qry = node.firstChild().nodeValue();
        } else if(n == "column") {
          _clmn = node.firstChild().nodeValue();
        } else {
          qDebug("while parsing field data encountered and unknown element: %s", n.toLatin1().constData());
        }
      }
    } else if(n == "string") {
      // ok -- this entity wasn't really part of the initial spec for work
      // and from what i understand the data should be pulled from the database
      // however this string field as part of the xml def i received implies that it
      // is static.
    } else if(n == "format") {
      _frmt = node.firstChild().nodeValue();
    } else if(n == "rect") {
      parseRect(node);
    } else if(n == "maxlength") {
      // this is the maximum length of a barcode value so that we can determine reasonably
      // what the minimum height of the barcode will be
      int i = node.firstChild().nodeValue().toInt();
      if(i < 1) i = 5;
      setMaxLength(i);
    } else if(n == "left") {
      setAlignment(0);
    } else if(n == "center") {
      setAlignment(1);
    } else if(n == "right") {
      setAlignment(2);
    } else if(n == "narrowBarWidth") {
        setNarrowBarWidth(node.firstChild().nodeValue().toDouble());
    } else {
      qDebug("while parsing barcode element encountered unknow element: %s",n.toLatin1().constData());
    }
  }
}

void ORGraphicsBarcodeItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  //qDebug("ReportEntityField::buildXML()");
  QDomElement entity = doc.createElement("barcode");

  buildXMLCommon(doc,entity);

  // format
  QDomElement fmt = doc.createElement("format");
  fmt.appendChild(doc.createTextNode(format()));
  entity.appendChild(fmt);

  QDomElement maxl = doc.createElement("maxlength");
  maxl.appendChild(doc.createTextNode(QString::number(maxLength())));
  entity.appendChild(maxl);

  // alignment
  if(alignment() < 1)
    entity.appendChild(doc.createElement("left"));
  else if(alignment() == 1)
    entity.appendChild(doc.createElement("center"));
  else // if(alignment() > 1)
    entity.appendChild(doc.createElement("right"));

  QDomElement bw = doc.createElement("narrowBarWidth");
  bw.appendChild(doc.createTextNode(QString::number(narrowBarWidth())));
  entity.appendChild(bw);

  // the field data
  QDomElement data = doc.createElement("data");
  QDomElement dquery = doc.createElement("query");
  dquery.appendChild(doc.createTextNode(query()));
  data.appendChild(dquery);
  QDomElement dcolumn = doc.createElement("column");
  dcolumn.appendChild(doc.createTextNode(column()));
  data.appendChild(dcolumn);
  entity.appendChild(data);

  parent.appendChild(entity);
}

void ORGraphicsBarcodeItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setPen(pen());
  painter->setBrush(brush());
  painter->drawText(rect(), 0, _clmn+QObject::tr(":")+_qry+QObject::tr(" barcode"));

  painter->restore();
}

void ORGraphicsBarcodeItem::properties(QWidget * parent)
{
  BarcodeEditor * le = new BarcodeEditor(parent);
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds)
    le->cbQuery->init(ds->qsList,query());

  if(alignment() < 1)
      le->rbAlignLeft->setChecked(true);
  else if(alignment() == 1)
      le->rbAlignCenter->setChecked(true);
  else //if(alignment > 1)
      le->rbAlignRight->setChecked(true);

  le->tbColumn->setText(column());

  if(le->cbFormat->findText(format()) >= 0)
    le->cbFormat->setCurrentIndex(le->cbFormat->findText(format()));
  else if(format().contains("datamatrix",Qt::CaseInsensitive))
  {
      le->setDatamatrixEditor(format());
  }
  else if (format().contains("PDF417", Qt::CaseInsensitive)) {
      le->setPDF417Editor(format());
      le->cbFormat->setCurrentIndex(le->cbFormat->findText("PDF417"));
      le->cbFormat_ViewConfig(le->cbFormat->findText("PDF417"));
  }
  else if (format().contains("QR", Qt::CaseInsensitive)) {
      le->setQREditor(format());
      le->cbFormat->setCurrentIndex(le->cbFormat->findText("QR"));
      le->cbFormat_ViewConfig(le->cbFormat->findText("QR"));
  }
  else
      le->cbFormat->insertItem(le->cbFormat->count(),format());

  //le->sliderMaxVal->setValue(maxLength());
  le->setCBSliderMaxValue(this->maxLength());
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));
  le->leNarrowBarWidth->setText(QString::number(narrowBarWidth(),'g',3));
  if(le->exec() == QDialog::Accepted)
  {
    setQuery(le->cbQuery->currentQuery());
    setColumn(le->tbColumn->text());

    //Seb Infflux
    //setFormat(le->cbFormat->currentText());
    this->setFormat(le->format());
    //Seb Infflux

    //setMaxLength(le->sliderMaxVal->value());
    this->setMaxLength(le->getCBSlideMaxValue());

    double dt;
    bool ok;
    dt = le->leXPos->text().toDouble(&ok);
    if(ok) dx = dt * 100.0;
    dt = le->leYPos->text().toDouble(&ok);
    if(ok) dy = dt * 100.0;
    dt = le->leWidth->text().toDouble(&ok);
    if(ok) dw = dt * 100.0;
    dt = le->leHeight->text().toDouble(&ok);
    if(ok) dh = dt * 100.0;

    if(pos().x() != dx || pos().y() != dy)
    {
      setPos(dx, dy);
      _setModified(scene(), true);
    }
    if(rect().width() != dw || rect().height() != dh)
    {
      setRect(0, 0, dw, dh);
      _setModified(scene(), true);
    }

    if(le->rbAlignLeft->isChecked())
      setAlignment(0);
    else if(le->rbAlignCenter->isChecked())
      setAlignment(1);
    else if(le->rbAlignRight->isChecked())
      setAlignment(2);

    double bw = le->leNarrowBarWidth->text().toDouble();
    if(bw != narrowBarWidth())
    {
      setNarrowBarWidth(bw);
      _setModified(scene(), true);
    }

    update();
  }
}

void ORGraphicsBarcodeItem::setFormat(const QString& f)
{
  if(_frmt != f)
  {
    _frmt = f;
    setMaxLength(_maxlength); // so we get the readjusted values
    _setModified(scene(), true);
  }
}

void ORGraphicsBarcodeItem::setQuery(const QString& t)
{
  if(_qry != t)
  {
    _qry = t;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsBarcodeItem::setColumn(const QString& t)
{
  if(_clmn != t)
  {
    _clmn = t;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsBarcodeItem::setMaxLength(int i)
{
  if(i > 0)
  {
    if(_maxlength != i)
    {
      _maxlength = i;
      _setModified(scene(), true);
    }
    if(_frmt == "3of9")
    {
      int C = i; // number of characters
      int N = 2; // narrow mult for wide line
      int X = 1; // narrow line width
      int I = 1; // interchange line width
      _min_width_data = ( ((C + 2) * ((3 * N) + 6) * X) + ((C + 1) * I) ) / 100.0;
      _min_height = _min_width_data * 0.15;
      /*if(_min_height < 0.25)*/ _min_height = 0.25;
      _min_width_total = _min_width_data + 0.22; // added a little buffer to make sure we don't loose any
                                               // of our required quiet zone in conversions
    }
    else if(_frmt == "3of9+")
    {
      int C = i*2; // number of characters
      int N = 2; // narrow mult for wide line
      int X = 1; // 1px narrow line
      int I = 1; // 1px narrow line interchange
      _min_width_data = ( ((C + 2) * ((3 * N) + 6) * X) + ((C + 1) * I) ) / 100.0;
      _min_height = _min_width_data * 0.15;
      /*if(_min_height < 0.25)*/ _min_height = 0.25;
      _min_width_total = _min_width_data + 0.22; // added a little buffer to make sure we don't loose any
                                               // of our required quiet zone in conversions
    }
    else if(_frmt == "i2of5")
    {
      _min_height = 0.25;
    }
    else if(_frmt == "128")
    {
      int C = i; // assuming 1:1 ratio of data passed in to data actually used in encoding
      int X = 1; // 1px wide
      _min_width_data = (((11 * C) + 35) * X) / 100.0; // assuming CODE A or CODE B
      _min_height = _min_width_data * 0.15;
      /*if(_min_height < 0.25)*/ _min_height = 0.25;
      _min_width_total = _min_width_data + 0.22; // added a little bugger to make sure we don't loose any
                                               // of our required quiet zone in conversions
    }
    else if(_frmt == "upc-a")
    {
      _min_width_data = 0.95;
      _min_width_total = 1.15;
      _min_height = 0.25;
    }
    else if(_frmt == "upc-e")
    {
      _min_width_data = 0.52;
      _min_width_total = 0.70;
      _min_height = 0.25;
    }
    else if(_frmt == "ean13")
    {
      _min_width_data = 0.95;
      _min_width_total = 1.15;
      _min_height = 0.25;
    }
    else if(_frmt == "ean8")
    {
      _min_width_data = 0.67;
      _min_width_total = 0.90;
      _min_height = 0.25;
    }
    else if(_frmt.contains("datamatrix") || _frmt.contains("PDF417") || _frmt.contains("QR"))
    {}
    else
    {
      qDebug("Unknown format encountered: %s", _frmt.toLatin1().constData());
    }
    setRect(0, 0, rect().width(),rect().height());
  }
}

void ORGraphicsBarcodeItem::setAlignment(int i)
{
  if(_align != i) {
    _align = i;
    _setModified(scene(), true);
  }
}

void ORGraphicsBarcodeItem::setNarrowBarWidth(double bw)
{
    if(_narrowBarWidth != bw) {
        _narrowBarWidth = bw;
        update();
    }
}



//
//ORGraphicsImageItem
//
ORGraphicsImageItem::ORGraphicsImageItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue-1); 
  _img_inline = false;
}

ORGraphicsImageItem::ORGraphicsImageItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue-1); 
  _img_inline = false;

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  _img_inline = false;
  for(int i = 0; i < nl.count(); i++) {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "data") {
      // see "string" just below for comments on String vs. Data
      QDomNodeList dnl = node.childNodes();
      for(int di = 0; di < dnl.count(); di++) {
        node = dnl.item(di);
        n = node.nodeName();
        if(n == "query") {
          _qry = node.firstChild().nodeValue();
        } else if(n == "column") {
          _clmn = node.firstChild().nodeValue();
        } else {
          qDebug("while parsing field data encountered and unknown element: %s", n.toLatin1().constData());
        }
      }
      _img_inline = false;
    } else if(n == "mode") {
      _mode = node.firstChild().nodeValue();
    } else if(n == "map") {
      // should read the format in but it will just be reset by the setImageData
      // method
      setInlineImageData(node.firstChild().nodeValue());
      _img_inline = true;
    } else if(n == "rect") {
      parseRect(node);
    } else {
      qDebug("while parsing image element encountered unknown element: %s",n.toLatin1().constData());
    }
  }
}

void ORGraphicsImageItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  QDomElement entity = doc.createElement("image");

  buildXMLCommon(doc,entity);

  // mode
  QDomElement md = doc.createElement("mode");
  md.appendChild(doc.createTextNode(_mode));
  entity.appendChild(md);

  if(isInline()) {
    QDomElement map = doc.createElement("map");
    map.setAttribute("format",_frmt);
    map.appendChild(doc.createTextNode(inlineImageData()));
    entity.appendChild(map);
  } else {
    // the field data
    QDomElement data = doc.createElement("data");
    QDomElement dquery = doc.createElement("query");
    dquery.appendChild(doc.createTextNode(query()));
    data.appendChild(dquery);
    QDomElement dcolumn = doc.createElement("column");
    dcolumn.appendChild(doc.createTextNode(column()));
    data.appendChild(dcolumn);
    entity.appendChild(data);
  }

  parent.appendChild(entity);
}

void ORGraphicsImageItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  if(isInline())
  {
    QImage t_img = img;
    if(_mode == "stretch")
    {
      t_img = img.scaled(rect().width(), rect().height(), Qt::KeepAspectRatio);
    }
    painter->drawImage(rect().left(), rect().top(), t_img, 0, 0, rect().width(), rect().height());
  }
  else
  {
    painter->drawText(rect(), 0, _clmn+QObject::tr(":")+_qry+QObject::tr(" image"));
  }

  painter->restore();

}

void ORGraphicsImageItem::properties(QWidget * parent)
{
  ImageEditor * le = new ImageEditor(parent);
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  if(ds)
    le->cbQuery->init(ds->qsList,query());

  le->tbColumn->setText(column());
  if(isInline())
  {
    le->setInline(true);
    le->setImageData(inlineImageData());
  }
  else
  {
    le->setInline(false);
  }
  le->setMode(mode());
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));

  if(le->exec() == QDialog::Accepted)
  {
    setQuery(le->cbQuery->currentQuery());
    setColumn(le->tbColumn->text());

    setMode(le->getMode());

    if(le->isInline()) {
      setInline(true);
      setInlineImageData(le->getImageData());
    } else {
      setInline(false);
    }

    double dt;
    bool ok;
    dt = le->leXPos->text().toDouble(&ok);
    if(ok) dx = dt * 100.0;
    dt = le->leYPos->text().toDouble(&ok);
    if(ok) dy = dt * 100.0;
    dt = le->leWidth->text().toDouble(&ok);
    if(ok) dw = dt * 100.0;
    dt = le->leHeight->text().toDouble(&ok);
    if(ok) dh = dt * 100.0;

    if(pos().x() != dx || pos().y() != dy) {
      setPos(dx, dy);
      _setModified(scene(), true);
    }
    if(rect().width() != dw || rect().height() != dh) {
      setRect(0, 0, dw, dh);
      _setModified(scene(), true);
    }

    update();
  }
}

void ORGraphicsImageItem::setInline(bool yes)
{
  if(_img_inline != yes)
  {
    _img_inline = yes;
    _setModified(scene(), true);

    update();
  }
}

void ORGraphicsImageItem::setInlineImageData(QString dat)
{
  if(img_data != dat)
  {
    img_data = dat;
    QByteArray bytes = QUUDecode(img_data);
    img.loadFromData(bytes);
    _frmt = QString(QImageReader::imageFormat(QDataStream(&bytes,QIODevice::ReadOnly).device()));

    update();

    _setModified(scene(), true);
  }
}

void ORGraphicsImageItem::setQuery(const QString& t)
{
  if(_qry != t)
  {
    _qry = t;
    _setModified(scene(), true);
  }
}

void ORGraphicsImageItem::setColumn(const QString& t)
{
  if(_clmn != t) {
    _clmn = t;
    _setModified(scene(), true);
  }
}

void ORGraphicsImageItem::setMode(QString m)
{
  if(_mode != m) {
    _mode = m;
    _setModified(scene(), true);

    update();
  }
}



//
//ORGraphicsGraphItem
//
ORGraphicsGraphItem::ORGraphicsGraphItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  _graphData.font = getDefaultEntityFont();
  _graphData.data.query = QString::null;
  _graphData.data.column = QString::null;
  _graphData.title.string = QString::null;
  _graphData.title.font_defined = false;
  _graphData.dataaxis.column = QString::null;
  _graphData.dataaxis.font_defined = false;
  _graphData.dataaxis.title.string = QString::null;
  _graphData.dataaxis.title.font_defined = false;
  _graphData.valueaxis.min = 0;
  _graphData.valueaxis.max = 100;
  _graphData.valueaxis.autominmax = true;
  _graphData.valueaxis.font_defined = false;
  _graphData.valueaxis.title.string = QString::null;
  _graphData.valueaxis.title.font_defined = false;
}

ORGraphicsGraphItem::ORGraphicsGraphItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  const QDomElement elem = element.toElement();
  if(!parseReportGraphData(elem, _graphData)) {
      qDebug("Error parsing graph data. Some data may be missing or incorrect.");
  }

  setPos(_graphData.rect.x(), _graphData.rect.y());

  double dw = _graphData.rect.width();
  double dh = _graphData.rect.height();

  setRect(0, 0, dw, dh);
}

void ORGraphicsGraphItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  QDomElement entity = doc.createElement("graph");

  buildXMLCommon(doc,entity);
  buildXMLFont(doc,entity,_graphData.font);

  QDomElement elem;

  // the field data
  QDomElement data = doc.createElement("data");
  QDomElement dquery = doc.createElement("query");
  dquery.appendChild(doc.createTextNode(query()));
  data.appendChild(dquery);
  QDomElement dcolumn = doc.createElement("column");
  data.appendChild(dcolumn);
  entity.appendChild(data);

  if(_graphData.title.string.length() > 0) {
    QDomElement title = doc.createElement("title");
    QDomElement string = doc.createElement("string");
    string.appendChild(doc.createTextNode(_graphData.title.string));
    title.appendChild(string);
    if(_graphData.title.font_defined) {
      buildXMLFont(doc, title, _graphData.title.font);
    }
    entity.appendChild(title);
  }

  if(_graphData.dataaxis.title.string.length() > 0 || _graphData.dataaxis.column.length() > 0) {
    QDomElement dataaxis = doc.createElement("dataaxis");
    entity.appendChild(dataaxis);

    if(_graphData.dataaxis.title.string.length() > 0) {
      QDomElement title = doc.createElement("title");
      QDomElement string = doc.createElement("string");
      string.appendChild(doc.createTextNode(_graphData.dataaxis.title.string));
      title.appendChild(string);
      if(_graphData.dataaxis.title.font_defined) {
        buildXMLFont(doc, title, _graphData.dataaxis.title.font);
      }
      dataaxis.appendChild(title);
    }

    if(_graphData.dataaxis.column.length() > 0) {
      QDomElement clmn = doc.createElement("column");
      clmn.appendChild(doc.createTextNode(_graphData.dataaxis.column));
      dataaxis.appendChild(clmn);
      if(_graphData.dataaxis.font_defined) {
        buildXMLFont(doc, dataaxis, _graphData.dataaxis.font);
      }
    }
  }

  if(_graphData.valueaxis.font_defined || _graphData.valueaxis.title.string.length() > 0 ||
     _graphData.valueaxis.min != 0 || _graphData.valueaxis.max != 100 ||
     _graphData.valueaxis.autominmax != true) {
    QDomElement valueaxis = doc.createElement("valueaxis");

    if(_graphData.valueaxis.title.string.length() > 0) {
      QDomElement title = doc.createElement("title");
      QDomElement string = doc.createElement("string");
      string.appendChild(doc.createTextNode(_graphData.valueaxis.title.string));
      title.appendChild(string);
      if(_graphData.valueaxis.title.font_defined) {
        buildXMLFont(doc, title, _graphData.valueaxis.title.font);
      }
      valueaxis.appendChild(title);
    }

    elem = doc.createElement("min");
    elem.appendChild(doc.createTextNode(QString::number(_graphData.valueaxis.min)));
    valueaxis.appendChild(elem);

    elem = doc.createElement("max");
    elem.appendChild(doc.createTextNode(QString::number(_graphData.valueaxis.max)));
    valueaxis.appendChild(elem);

    elem = doc.createElement("autominmax");
    elem.appendChild(doc.createTextNode((_graphData.valueaxis.autominmax?"true":"false")));
    valueaxis.appendChild(elem);

    if(_graphData.valueaxis.font_defined) {
      buildXMLFont(doc, valueaxis, _graphData.valueaxis.font);
    }
    entity.appendChild(valueaxis);
  }

  if(_graphData.padding.horizontal!=5 || _graphData.padding.vertical!=5)
  {
    QDomElement padding = doc.createElement("padding");
    if(_graphData.padding.horizontal!=5)
    {
      elem = doc.createElement("horizontal");
      elem.appendChild(doc.createTextNode(QString::number(_graphData.padding.horizontal)));
      padding.appendChild(elem);
    }
    if(_graphData.padding.vertical!=5)
    {
      elem = doc.createElement("vertical");
      elem.appendChild(doc.createTextNode(QString::number(_graphData.padding.vertical)));
      padding.appendChild(elem);
    }
    entity.appendChild(padding);
  }

  for(int snum = 0; snum < _graphData.series.count(); snum++) {
    ORSeriesData * series = _graphData.series.at(snum);
    if(series) {
      QDomElement selem = doc.createElement("series");

      elem = doc.createElement("name");
      elem.appendChild(doc.createTextNode(series->name));
      selem.appendChild(elem);

      elem = doc.createElement("column");
      elem.appendChild(doc.createTextNode(series->column));
      selem.appendChild(elem);

      elem = doc.createElement("color");
      elem.appendChild(doc.createTextNode(series->color));
      selem.appendChild(elem);

      if(series->style.bar || series->style.line || series->style.point) {
        elem = doc.createElement("style");
        if(series->style.bar) elem.appendChild(doc.createElement("bar"));
        if(series->style.line) elem.appendChild(doc.createElement("line"));
        if(series->style.point) elem.appendChild(doc.createElement("point"));
        selem.appendChild(elem);
      }

      entity.appendChild(selem);
    }
  }

  parent.appendChild(entity);
}

void ORGraphicsGraphItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setPen(pen());
  painter->setBrush(brush());
  painter->drawText(rect(), 0, _graphData.data.query+QObject::tr(" graph"));

  painter->restore();
}

void ORGraphicsGraphItem::properties(QWidget * parent)
{
  GraphEditor * le = new GraphEditor(parent);
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  int i = 0;
  if(ds)
  {
    le->setColorMap(&(ds->_colorMap));
    le->cbQuery->init(ds->qsList,query());
  }

  //le->tbColumn->setText(column());
  double dx = pos().x() / 100.0;
  le->leXPos->setText(QString::number(dx,'g',3));
  double dy = pos().y() / 100.0;
  le->leYPos->setText(QString::number(dy,'g',3));
  double dw = rect().width() / 100.0;
  le->leWidth->setText(QString::number(dw,'g',3));
  double dh = rect().height() / 100.0;
  le->leHeight->setText(QString::number(dh,'g',3));

  le->setBaseFont(_graphData.font);
  le->setTitleFont(_graphData.font);
  le->setUseTitleFont(false);
  le->setDataTitleFont(_graphData.font);
  le->setUseDataTitleFont(false);
  le->setDataFont(_graphData.font);
  le->setUseDataFont(false);
  le->setValueTitleFont(_graphData.font);
  le->setUseValueTitleFont(false);
  le->setValueFont(_graphData.font);
  le->setUseValueFont(false);

  le->setTitle(_graphData.title.string);
  if(_graphData.title.font_defined)
  {
    le->setTitleFont(_graphData.title.font);
    le->setUseTitleFont(true);
  }

  le->setDataColumn(_graphData.dataaxis.column);
  if(_graphData.dataaxis.font_defined)
  {
    le->setDataFont(_graphData.dataaxis.font);
    le->setUseDataFont(true);
  }

  le->setDataTitle(_graphData.dataaxis.title.string);
  if(_graphData.dataaxis.title.font_defined)
  {
    le->setDataTitleFont(_graphData.dataaxis.title.font);
    le->setUseDataTitleFont(true);
  }

  le->setHPadding(_graphData.padding.horizontal);
  le->setVPadding(_graphData.padding.vertical);

  le->setMinValue(_graphData.valueaxis.min);
  le->setMaxValue(_graphData.valueaxis.max);
  le->setAutoMinMax(_graphData.valueaxis.autominmax);
  if(_graphData.valueaxis.font_defined)
  {
    le->setValueFont(_graphData.valueaxis.font);
    le->setUseValueFont(true);
  }
  le->setValueTitle(_graphData.valueaxis.title.string);
  if(_graphData.valueaxis.title.font_defined)
  {
    le->setValueTitleFont(_graphData.valueaxis.title.font);
    le->setUseValueTitleFont(true);
  }

  ORSeriesData * sd1 = 0;
  ORSeriesData * sd2 = 0;
  for(i = 0; i < _graphData.series.count(); i++)
  {
    sd1 = _graphData.series.at(i);
    sd2 = new ORSeriesData();
    *sd2 = *sd1;
    le->getSeriesList().append(sd2);
    le->_cbSeries->addItem(sd2->name);
  }
  le->_cbSeries_activated(le->_cbSeries->itemText(le->_cbSeries->currentIndex()));

  // add in the code to do the value label stuff as well

  if(le->exec() == QDialog::Accepted)
  {
    setQuery(le->cbQuery->currentQuery());

    _graphData.font = le->getBaseFont();
    _graphData.title.string = le->getTitle();
    _graphData.title.font = le->getTitleFont();
    _graphData.title.font_defined = le->getUseTitleFont();
    _graphData.dataaxis.column = le->getDataColumn();
    _graphData.dataaxis.font = le->getDataFont();
    _graphData.dataaxis.font_defined = le->getUseDataFont();
    _graphData.dataaxis.title.string = le->getDataTitle();
    _graphData.dataaxis.title.font = le->getDataTitleFont();
    _graphData.dataaxis.title.font_defined = le->getUseDataTitleFont();
    _graphData.valueaxis.min = le->getMinValue();
    _graphData.valueaxis.max = le->getMaxValue();
    _graphData.valueaxis.autominmax = le->getAutoMinMax();
    _graphData.valueaxis.font = le->getValueFont();
    _graphData.valueaxis.font_defined = le->getUseValueFont();
    _graphData.valueaxis.title.string = le->getValueTitle();
    _graphData.valueaxis.title.font = le->getValueTitleFont();
    _graphData.valueaxis.title.font_defined = le->getUseValueTitleFont();
    _graphData.padding.horizontal = le->getHPadding();
    _graphData.padding.vertical = le->getVPadding();

    sd1 = sd2 = 0;
    _graphData.series.clear();
    for(int ii = 0; ii < le->getSeriesList().count(); ii++)
    {
      sd2 = le->getSeriesList().at(ii);
      sd1 = new ORSeriesData();
      *sd1 = *sd2;
      if(sd1->color.length() < 1) sd1->color = le->_cbColors->currentText();
      _graphData.series.append(sd1);
    }

    double dt;
    bool ok;
    dt = le->leXPos->text().toDouble(&ok);
    if(ok) dx = dt * 100.0;
    dt = le->leYPos->text().toDouble(&ok);
    if(ok) dy = dt * 100.0;
    dt = le->leWidth->text().toDouble(&ok);
    if(ok) dw = dt * 100.0;
    dt = le->leHeight->text().toDouble(&ok);
    if(ok) dh = dt * 100.0;

    setPos(dx, dy);
    setRect(0, 0, dw, dh);

    update();

    // eek -- this one is just out of control. I'm just going to say
    // that the document has been modified and be done with it for now.
    // the graph entity is fairly new and not in standard use so it
    // shouldn't be that big of a deal.
    _setModified(scene(), true);
  }
}

void ORGraphicsGraphItem::setData(const ORGraphData & gData)
{
  _graphData = gData;
}

void ORGraphicsGraphItem::copyData(ORGraphData & gData)
{
  gData.data = _graphData.data;
  gData.font = _graphData.font;
  gData.rect = _graphData.rect;
  gData.title = _graphData.title;
  gData.dataaxis = _graphData.dataaxis;
  gData.valueaxis = _graphData.valueaxis;
  gData.padding = _graphData.padding;
  gData.series.clear();
  for(int i = 0; i < _graphData.series.count(); i++) {
      ORSeriesData * sd = new ORSeriesData();
      *sd = *_graphData.series.at(i);
      gData.series.append(sd);
  }
  _setModified(scene(), true);
}

void ORGraphicsGraphItem::setQuery(const QString & str)
{
  if(_graphData.data.query != str)
  {
    _graphData.data.query = str;
    _setModified(scene(), true);
  }
}

QString ORGraphicsGraphItem::query() const
{
  return _graphData.data.query;
}


//
//ORGraphicsCrossTabItem
//
ORGraphicsCrossTabItem::ORGraphicsCrossTabItem(QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  m_font = getDefaultEntityFont();

  setColumn("column",  "column");
  setColumn("row",     "row");
  setColumn("value", "value");

  setHAlign("column",  "Center");
  setHAlign("row",     "Center");
  setHAlign("value", "Center");

  setVAlign("column",  "Center");
  setVAlign("row",     "Center");
  setVAlign("value", "Center");

  setTableWrapping(false);
  setCellMargins(0.01, 0.01, 0.01, 0.01);
  setDisplayHeaderColumn(true);
  setDisplayHeaderRow(true);

  setTableRect();
}

ORGraphicsCrossTabItem::ORGraphicsCrossTabItem(const QDomNode & element, QGraphicsItem * parent)
  : ORGraphicsRectItem(parent)
{
  setZValue(defaultZvalue); 
  m_font = getDefaultEntityFont();

  QDomNodeList nl = element.childNodes();
  QString n;
  QDomNode node;
  for(int i = 0; i < nl.count(); i++)
  {
    node = nl.item(i);
    n = node.nodeName();
    if(n == "data")
    {
      QDomNodeList dnl = node.childNodes();
      for(int di = 0; di < dnl.count(); di++)
      {
        node = dnl.item(di);
        n = node.nodeName();
        if(n == "query")
        {
          m_query = node.firstChild().nodeValue();
        }
        else
        {
          qDebug("while parsing field data encountered and unknown element: %s", n.toLatin1().data());
        }
      }
      //txt = node.firstChild().nodeValue();
    }
    else if(n == "rect")
    {
      parseRect(node);
    }
    else if(n == "font")
    {
      QFont savedFont("Calibri", 12, QFont::Normal);

      QDomNodeList fnl = node.childNodes();
      for(int fi = 0; fi < fnl.count(); fi++)
      {
        node = fnl.item(fi);
        n = node.nodeName();
        if(n == "face")
        {
          savedFont.setFamily(node.firstChild().nodeValue());
        }
        else if(n == "size")
        {
          savedFont.setPointSize(node.firstChild().nodeValue().toInt());
        }
        else if(n == "weight")
        {
          QString v = node.firstChild().nodeValue();
          if(v == "bold")
          {
            savedFont.setBold(true);
          }
          else if(v == "normal")
          {
            savedFont.setBold(false);
          }
          else
          {
            savedFont.setWeight(node.firstChild().nodeValue().toInt());
          }
        }
	    else if(n == "italic") {
		  savedFont.setItalic(true);
		}
        else
        {
          qDebug("while parsing font encountered unknown element: %s", n.toLatin1().data());
        }
        setFont(savedFont);
      }
    }
    else if ( (n == "column") ||
              (n == "row") ||
              (n == "value") )
    {
      QDomNodeList child_dnl = node.childNodes();
      for(int child_i = 0; child_i < child_dnl.count(); child_i++)
      {
        node = child_dnl.item(child_i);
        QString child_n = node.nodeName();
        if(child_n == "queryColumn")
        {
          setColumn(n, node.firstChild().nodeValue());
        }
        else if(child_n == "HAlign")
        {
          setHAlign(n, node.firstChild().nodeValue());
        }
        else if(child_n == "VAlign")
        {
          setVAlign(n, node.firstChild().nodeValue());
        }
        else
        {
          qDebug("while parsing field data encountered and unknown element: %s", child_n.toLatin1().data());
        }
      }
    }
    else if (n == "table")
    {
      QDomNodeList child_dnl = node.childNodes();
      for(int child_i = 0; child_i < child_dnl.count(); child_i++)
      {
        node = child_dnl.item(child_i);
        QString child_n = node.nodeName();
        if(child_n == "wrappolicy")
        {
          if (QString("columns") == QString(node.firstChild().nodeValue()))
          {
            setTableWrapping(true);
          }
          else
          {
            setTableWrapping(false);
          }
        }
        else if(child_n == "showcolumnheaderagain")
        {
          if ("yes" == node.firstChild().nodeValue())
          {
            setDisplayHeaderColumn(true);
          }
          else
          {
            setDisplayHeaderColumn(false);
          }
        }
        else if(child_n == "showrowheaderagain")
        {
          if ("yes" == node.firstChild().nodeValue())
          {
            setDisplayHeaderRow(true);
          }
          else
          {
            setDisplayHeaderRow(false);
          }
        }
        else if(child_n == "CellMargins")
        {
          QDomNodeList rnl = node.childNodes();
          double l = 0.0;
          double r = 0.0;
          double t = 0.0;
          double b = 0.0;
          for(int ri = 0; ri < rnl.count(); ri++)
          {
            node = rnl.item(ri);
            n = node.nodeName();
            if(n == "Left")
            {
              l = node.firstChild().nodeValue().toDouble();
            }
            else if(n == "Right")
            {
              r = node.firstChild().nodeValue().toDouble();
            }
            else if(n == "Top")
            {
              t = node.firstChild().nodeValue().toDouble();
            }
            else if(n == "Bottom")
            {
              b = node.firstChild().nodeValue().toDouble();
            } else {
              qDebug("While parsing cell margins encountered unknown element: %s", n.toLatin1().data());
            }
          }
          l = (l / 100.0);
          r = (r / 100.0);
          t = (t / 100.0);
          b = (b / 100.0);
          setCellMargins(l, r, t, b);
        }
        else
        {
          qDebug("while parsing field data encountered and unknown element: %s", child_n.toLatin1().data());
        }
      }
    }
    else
    {
      qDebug("while parsing field element encountered unknow element: %s",n.toLatin1().data());
    }
  }
}

void ORGraphicsCrossTabItem::buildXML(QDomDocument & doc, QDomElement & parent)
{
  //qDebug("ReportEntityCrossTab::buildXML()");
  QDomElement entity = doc.createElement("crosstab");

  buildXMLCommon(doc,entity);
  // font info
  buildXMLFont(doc,entity,font());

  // The data field
  {
    QDomElement data = doc.createElement("data");
    QDomElement dquery = doc.createElement("query");
    dquery.appendChild(doc.createTextNode(query()));
    data.appendChild(dquery);
    entity.appendChild(data);
  }

  // Table properties
  {
    QDomElement table = doc.createElement("table");
    QDomElement wrap = doc.createElement("wrappolicy");
    if (tableWrapping())
    {
      wrap.appendChild(doc.createTextNode("columns"));
    }
    else
    {
      wrap.appendChild(doc.createTextNode("rows"));
    }
    table.appendChild(wrap);

    QDomElement showHeaderCol = doc.createElement("showcolumnheaderagain");
    if (displayHeaderColumn())
    {
      showHeaderCol.appendChild(doc.createTextNode("yes"));
    }
    else
    {
      showHeaderCol.appendChild(doc.createTextNode("no"));
    }
    table.appendChild(showHeaderCol);

    QDomElement showHeaderRow = doc.createElement("showrowheaderagain");
    if (displayHeaderRow())
    {
      showHeaderRow.appendChild(doc.createTextNode("yes"));
    }
    else
    {
      showHeaderRow.appendChild(doc.createTextNode("no"));
    }
    table.appendChild(showHeaderRow);

    double l, r, t, b;
    cellMargins (l, r, t, b);
    l = l * 100.0;
    r = r * 100.0;
    t = t * 100.0;
    b = b * 100.0;
    QDomElement cellMargins = doc.createElement("CellMargins");
    QDomElement left = doc.createElement("Left");
    left.appendChild(doc.createTextNode(QString::number(l)));
    cellMargins.appendChild(left);
    QDomElement right = doc.createElement("Right");
    right.appendChild(doc.createTextNode(QString::number(r)));
    cellMargins.appendChild(right);
    QDomElement top = doc.createElement("Top");
    top.appendChild(doc.createTextNode(QString::number(t)));
    cellMargins.appendChild(top);
    QDomElement bottom = doc.createElement("Bottom");
    bottom.appendChild(doc.createTextNode(QString::number(b)));
    cellMargins.appendChild(bottom);
    table.appendChild(cellMargins);
    entity.appendChild(table);
  }

  // Column data - SeriesAxis
  {
    QDomElement lcolumn = doc.createElement("column");
    QDomElement qColumn = doc.createElement("queryColumn");
    qColumn.appendChild(doc.createTextNode(column("column")));
    lcolumn.appendChild(qColumn);
    QDomElement hAlign = doc.createElement("HAlign");
    hAlign.appendChild(doc.createTextNode(HAlign("column")));
    lcolumn.appendChild(hAlign);
    QDomElement vAlign = doc.createElement("VAlign");
    vAlign.appendChild(doc.createTextNode(VAlign("column")));
    lcolumn.appendChild(vAlign);
    entity.appendChild(lcolumn);
  }

  // Row data - DataAxis
  {
    QDomElement row = doc.createElement("row");
    QDomElement qColumn = doc.createElement("queryColumn");
    qColumn.appendChild(doc.createTextNode(column("row")));
    row.appendChild(qColumn);
    QDomElement hAlign = doc.createElement("HAlign");
    hAlign.appendChild(doc.createTextNode(HAlign("row")));
    row.appendChild(hAlign);
    QDomElement vAlign = doc.createElement("VAlign");
    vAlign.appendChild(doc.createTextNode(VAlign("row")));
    row.appendChild(vAlign);
    entity.appendChild(row);
  }

  // value data - Values
  {
    QDomElement value = doc.createElement("value");
    QDomElement qColumn = doc.createElement("queryColumn");
    qColumn.appendChild(doc.createTextNode(column("value")));
    value.appendChild(qColumn);
    QDomElement hAlign = doc.createElement("HAlign");
    hAlign.appendChild(doc.createTextNode(HAlign("value")));
    value.appendChild(hAlign);
    QDomElement vAlign = doc.createElement("VAlign");
    vAlign.appendChild(doc.createTextNode(VAlign("value")));
    value.appendChild(vAlign);
    entity.appendChild(value);
  }
  parent.appendChild(entity);
}

//////////////////////////////////////////////////////////////////////////////
// Description:
//
// This is what we need to print. If it is not within the selection it should
// not be printed.'
// Note: The behaviour that the view can be smaller than the actual size is
//       adopted from the other entities.
//
// QueryName : QueryValue - CrossTab
//  ---------------
// |     | Column  |
//  ---------------
// | Row | value |
//  ---------------
//
// Using the painter we can define the area in which to paint. This is used to
// reduce the calculation on the end of the items.
//
// Below we use that functionallity to draw the item from top left to bottom
// right. As long as the top left of an item is within the current
// intersection at least part of the item can be painted.
//
// TODO:
//   1. Currently there are no margins within the printed table.
//   2. Everything is shown and calculated as if all text is aligned left.
//      In fact for only the designer this might be correct.
//////////////////////////////////////////////////////////////////////////////
void ORGraphicsCrossTabItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  // let the base class draw the selection and box
  ORGraphicsRectItem::paint(painter, option, widget);

  painter->save();

  painter->setFont(font());
  painter->setPen(Qt::SolidLine);

  QRect columnRect = QFontMetrics(font()).boundingRect(column("column"));
  QRect rowRect = QFontMetrics(font()).boundingRect(column("row"));

  QRectF selectionRect = rect();
  // Paint rect
  painter->setPen (Qt::DotLine);
  painter->drawRect(selectionRect);
  painter->setPen (Qt::SolidLine);

  // Draw query
  QRectF queryRect = QFontMetrics(font()).boundingRect(query());
  painter->drawText(selectionRect, Qt::AlignLeft, query());

  // Continue painting
  if ((selectionRect.height() > queryRect.height()))
  {
    // Space left for (part of) table
    queryRect = QFontMetrics(font()).boundingRect(int(x()), int(y()), 0, 0, Qt::AlignLeft, query());
    QRectF sectRect(selectionRect);
    sectRect.setY(sectRect.y() + queryRect.height());

    // Draw table rect
    QRectF ltableRect = tableRect();
      // Y may have changed
    ltableRect.moveTop(sectRect.y());

    if (sectRect.intersects(ltableRect))
    {
      QRectF intRect = sectRect.intersect(ltableRect);
      painter->drawRect(intRect);

      // Draw table empty header
        // Nothing to do

      // Draw table column header
      // Not outside rectangle
      painter->setBackgroundMode(Qt::OpaqueMode);
      if (intRect.width() >  rowRect.width())
      {
        painter->setBackground(Qt::darkCyan);
        painter->setPen(Qt::white);
        painter->drawText(intRect.x()+rowRect.width(), intRect.y(), intRect.width()-rowRect.width(), intRect.height(), Qt::AlignLeft, column("column"));
        painter->setBackground(Qt::NoBrush);
        painter->setPen(Qt::black);

        // Also draw vertical line
        painter->drawLine(intRect.x() + rowRect.width(), intRect.y(), intRect.x() + rowRect.width(), intRect.y()+intRect.height());
      }
      // Draw table row header
      if (intRect.height() >  columnRect.height())
      {
        painter->setBackground(Qt::darkMagenta);
        painter->setPen(Qt::white);
        painter->drawText(intRect.x(), intRect.y() + columnRect.height(), intRect.width(), intRect.height()-columnRect.height(), Qt::AlignLeft, column("row"));
        painter->setBackground(Qt::NoBrush);
        painter->setPen(Qt::black);

        // Also draw horizontal line
        painter->drawLine(intRect.x(), intRect.y() + columnRect.height(), intRect.x() + intRect.width(), intRect.y() + columnRect.height());

        // Draw table value
        if (intRect.width() >  rowRect.width())
        {
          painter->setBackground(Qt::darkYellow);
          painter->setPen(Qt::white);
          painter->drawText(intRect.x()+rowRect.width(), intRect.y() + columnRect.height(), intRect.width()-rowRect.width(), intRect.height()-columnRect.height(), Qt::AlignLeft, column("value"));
          painter->setBackground(Qt::NoBrush);
          painter->setPen(Qt::black);
        }
      }
    }
  }

  painter->restore();
}

void ORGraphicsCrossTabItem::properties(QWidget * parent)
{
  CrossTabEditor * le = new CrossTabEditor(parent);
  DocumentScene * ds = static_cast<DocumentScene*>(scene());
  // Populate the data sources
  if(ds)
    le->cbQuery->init(ds->qsList,query());

  le->_font = font();

  // Populate Table query properties
  {
    // Margins
    double l, r, t, b;
    cellMargins(l, r, t, b);
    le->m_cellLeftMarginDoubleSpinBox->setValue(l);
    le->m_cellRightMarginDoubleSpinBox->setValue(r);
    le->m_cellTopMarginDoubleSpinBox->setValue(t);
    le->m_cellBottomMarginDoubleSpinBox->setValue(b);

    // Wrapping
    bool columnWrapping(tableWrapping());
    if (columnWrapping)
    {
      le->m_wrapColumnsFirstRadioButton->setChecked(true);
    }
    else
    {
      le->m_wrapRowsFirstRadioButton->setChecked(true);
    }

    // Headers
    bool columnHeader(displayHeaderColumn());
    le->m_tableDisplayColumnHeaderCheckBox->setChecked(columnHeader);
    bool rowHeader(displayHeaderRow());
    le->m_tableDisplayRowHeaderCheckBox->setChecked(rowHeader);
  }

  // Populate Column query properties
  le->columnQueryLineEdit->setText(column("column"));
  le->columnQueryHAlignComboBox->setCurrentIndex(le->columnQueryHAlignComboBox->findText(HAlign("column")));
  le->columnQueryVAlignComboBox->setCurrentIndex(le->columnQueryVAlignComboBox->findText(VAlign("column")));

  // Populate Row query properties
  le->rowQueryLineEdit->setText(column("row"));
  le->rowQueryHAlignComboBox->setCurrentIndex(le->rowQueryHAlignComboBox->findText(HAlign("row")));
  le->rowQueryVAlignComboBox->setCurrentIndex(le->rowQueryVAlignComboBox->findText(VAlign("row")));

  // Populate value query properties
  le->valueQueryLineEdit->setText(column("value"));
  le->valueQueryHAlignComboBox->setCurrentIndex(le->valueQueryHAlignComboBox->findText(HAlign("value")));
  le->valueQueryVAlignComboBox->setCurrentIndex(le->valueQueryVAlignComboBox->findText(VAlign("value")));


  if(le->exec() == QDialog::Accepted)
  {
    // Save query
    setQuery(le->cbQuery->currentQuery());
    setFont(le->_font);

    // Populate Table query properties
    {
      // Margins
      setCellMargins(le->m_cellLeftMarginDoubleSpinBox->value(),
                     le->m_cellRightMarginDoubleSpinBox->value(),
                     le->m_cellTopMarginDoubleSpinBox->value(),
                     le->m_cellBottomMarginDoubleSpinBox->value());

      // Wrapping
      setTableWrapping(le->m_wrapColumnsFirstRadioButton->isChecked());

      // Headers
      setDisplayHeaderColumn(le->m_tableDisplayColumnHeaderCheckBox->isChecked());
      setDisplayHeaderRow(le->m_tableDisplayRowHeaderCheckBox->isChecked());
    }

    m_queryColumn.clear();
    setColumn("column",  le->columnQueryLineEdit->text ());
    setColumn("row",     le->rowQueryLineEdit->text ());
    setColumn("value", le->valueQueryLineEdit->text ());

    m_hAlign.clear();
    setHAlign("column",  le->columnQueryHAlignComboBox->currentText());
    setHAlign("row",     le->rowQueryHAlignComboBox->currentText());
    setHAlign("value", le->valueQueryHAlignComboBox->currentText());

    m_vAlign.clear();
    setVAlign("column",  le->columnQueryVAlignComboBox->currentText());
    setVAlign("row",     le->rowQueryVAlignComboBox->currentText());
    setVAlign("value", le->valueQueryVAlignComboBox->currentText());

    // Set modified if some display value changed
    _setModified(scene(), true);

    update();
  }
}

void ORGraphicsCrossTabItem::setQuery(const QString& q)
{
  if(m_query != q)
  {
    m_query = q;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsCrossTabItem::setColumn(const QString& type, const QString & name)
{
  if (!m_queryColumn.contains(type, name))
  {
    m_queryColumn.insert(type, name);
    //SetRect();
    _setModified(scene(), true);
  }
}

void ORGraphicsCrossTabItem::setFont(const QFont& f)
{
  if(m_font != f)
  {
    m_font = f;
    _setModified(scene(), true);
    static_cast<DocumentScene*>(scene())->refreshFontToolBox();
    update();
  }
}

void ORGraphicsCrossTabItem::setHAlign(const QString& type, const QString& name)
{
  if (!m_hAlign.contains(type))
  {
    m_hAlign.insert(type, name);
    //SetRect();
    _setModified(scene(), true);
  }
}

void ORGraphicsCrossTabItem::setVAlign(const QString& type, const QString& name)
{
  if (!m_vAlign.contains(type))
  {
    m_vAlign.insert(type, name);
    //SetRect();
    _setModified(scene(), true);
  }
}

void ORGraphicsCrossTabItem::setTableWrapping(const bool& b)
{
  if(m_tableWrapPolicyColumnsFirst != b)
  {
    m_tableWrapPolicyColumnsFirst = b;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsCrossTabItem::setCellMargins(const double& left, const double& right, const double& top,  const double& bottom)
{
  if (m_leftMargin != left)
  {
    m_leftMargin = left;
    _setModified(scene(), true);
  }
  if (m_rightMargin != right)
  {
    m_rightMargin = right;
    _setModified(scene(), true);
  }
  if (m_topMargin != top)
  {
    m_topMargin = top;
    _setModified(scene(), true);
  }
  if (m_bottomMargin != bottom)
  {
    m_bottomMargin = bottom;
    _setModified(scene(), true);
  }
  update();
}

void ORGraphicsCrossTabItem::setDisplayHeaderColumn(const bool& b)
{
  if(m_columnHeaderOnEachPart != b)
  {
    m_columnHeaderOnEachPart = b;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsCrossTabItem::setDisplayHeaderRow(const bool& b)
{
  if(m_rowHeaderOnEachPart != b)
  {
    m_rowHeaderOnEachPart = b;
    _setModified(scene(), true);
    update();
  }
}

void ORGraphicsCrossTabItem::setTableRect()
{
  setRect(tableRect());
}

QRectF ORGraphicsCrossTabItem::tableRect()
{
  //Set current font
  QFont currentFont(font());

  // get current fontmetrics for values
  QRectF columnRect  = QFontMetrics(currentFont).boundingRect(column("column"));
  QRectF rowRect     = QFontMetrics(currentFont).boundingRect(column("row"));
  QRectF valueRect   = QFontMetrics(currentFont).boundingRect(column("value"));

  // Define needed height
  qreal rowHeight    = rowRect.height();
  qreal rowWidth     = rowRect.width();
  qreal columnHeight = columnRect.height();
  qreal columnWidth  = (valueRect.width() > columnRect.width()) ? valueRect.width() : columnRect.width();

  QRectF resultRect;

  resultRect.setX (rect().x());
  resultRect.setY (rect().y());
  resultRect.setWidth (rowWidth + columnWidth);
  resultRect.setHeight (columnHeight + rowHeight);

  return resultRect;
}


