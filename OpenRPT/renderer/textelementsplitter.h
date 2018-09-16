#ifndef TEXTELEMENTSPLITTER_H
#define TEXTELEMENTSPLITTER_H

#include <QSharedPointer>

class QFontMetrics;
class ORTextData;

class TextElementSplitter
{
public:
    TextElementSplitter(ORObject* textelem, QString text, qreal leftMargin, qreal yOffset, qreal pageBottom=0);

    void nextLine();
    void newPage(qreal offset);
    QString currentLine() const;
    QRectF currentLineRect() const;
    qreal textBottomRelativePos() const;
    ORTextData * element() const;
    bool endOfPage() const;
    bool endOfText() const;

private:

    QString _text;
    QString _currentLine;
    ORTextData * _element;
    QRectF _baseElementRect;
    int    _lineClipWidth;
    qreal _leftMargin;
    qreal _yOffset;
    qreal _pageBottom;
    QSharedPointer<QFontMetrics> _fm;
    int _lineCounter;
};

#endif // TEXTELEMENTSPLITTER_H
