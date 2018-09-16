#include <QtGui>

#include "orprerender.h"
#include "renderobjects.h"
#include "orutils.h"
#include <xsqlquery.h>
#include <parsexmlutils.h>
#include "textelementsplitter.h"

#include <QPrinter>
#include <QFontMetrics>
#include <QPainter>

#define CLIPMARGIN 10

TextElementSplitter::TextElementSplitter(ORObject *textelem, QString text, qreal leftMargin, qreal yOffset, qreal pageBottom) :
  _text(text), _leftMargin(leftMargin), _yOffset(yOffset), _pageBottom(pageBottom), _lineCounter(0)
{   
  _element = textelem->toText();

  if (_text.isEmpty())
  {
    return;
  }

  QPointF pos = _element->rect.topLeft();
  QSizeF size(_element->rect.width(), _element->rect.height());
  pos /= 100.0;
  pos += QPointF(_leftMargin, 0);
  size /= 100.0;

  QImage prnt(1, 1, QImage::Format_RGB32);

  _baseElementRect = QRectF(pos, size);

#ifdef Q_WS_MAC // bug 13284, 15118
  if(_element->align & Qt::AlignRight)
    _baseElementRect.setLeft(_baseElementRect.left() - CLIPMARGIN / 100.0);
  else
    _baseElementRect.setRight(_baseElementRect.right() + CLIPMARGIN / 100.0);
#endif

  _lineClipWidth = (int)(_baseElementRect.width() * prnt.logicalDpiX()) - CLIPMARGIN;

  _fm = QSharedPointer<QFontMetrics>(new QFontMetrics(_element->font, &prnt));

  // insert spaces into text to allow it to wrap
  QPainter imagepainter(&prnt);
  OROTextBox tmpbox(textelem);
  tmpbox.setPosition(_baseElementRect.topLeft());
  tmpbox.setSize(_baseElementRect.size());
  tmpbox.setFont(_element->font);
  tmpbox.setText(_text);
  tmpbox.setFlags(_element->align | Qt::TextWordWrap);
  tmpbox.setRotation(_element->rotation());
  _text = tmpbox.textForcedToWrap(&imagepainter);
}

void TextElementSplitter::nextLine()
{
  QRegExp re("\\s");
  int currentPos = 0;
  bool endOfLine = false;
  _currentLine.clear();

  while(!endOfLine)
  {
    int idx = re.indexIn(_text, currentPos);
    bool endOfText = (idx == -1);
    if(idx >=0 && _text[idx] == '\n')
    {
      currentPos = idx + 1;
      endOfLine = true;
    }
    else {
      endOfLine = _fm->boundingRect(_text.left(idx)).width() > _lineClipWidth;
    }
    if(endOfText && !endOfLine)
    {
      currentPos = _text.length() + 1;
      endOfLine = true;
    }
    if(endOfLine && currentPos==0)
    {
      currentPos = _text.length() + 1;
    }
    if(endOfLine)
    {
      _currentLine = _text.left(currentPos - 1);
      _text = _text.mid(currentPos, _text.length());
    }
    else
    {
      currentPos = idx + 1;
    }

  }

  _lineCounter++;
}

void TextElementSplitter::newPage(qreal offset)
{
  _yOffset = offset;
  _lineCounter = 0;
}

QString TextElementSplitter::currentLine() const
{
  return _currentLine;
}

QRectF TextElementSplitter::currentLineRect() const
{
  return _baseElementRect.translated(0, _yOffset + _baseElementRect.height()*(_lineCounter-1));
}

qreal TextElementSplitter::textBottomRelativePos() const
{
  return currentLineRect().bottom() + (_element->bottompad / 100.0) - _yOffset;
}

ORTextData *TextElementSplitter::element() const
{
  return _element;
}

bool TextElementSplitter::endOfPage() const
{
  return currentLineRect().bottom() + _baseElementRect.height() > _pageBottom;
}

bool TextElementSplitter::endOfText() const
{
  return _text.isEmpty();
}


