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

#ifndef METASQLHIGHLIGHTER_H
#define METASQLHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class QColor;
class QTextDocument;
class QTextEdit;

class MetaSQLHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

  public:
    MetaSQLHighlighter(QObject *parent);
    MetaSQLHighlighter(QTextDocument *document);
    MetaSQLHighlighter(QTextEdit *editor);
    ~MetaSQLHighlighter();

  protected:
    enum State { NormalState = -1, InsideCStyleComment, InsideString, InsideMetaSQL, InsideMetaSQLString };
    virtual void highlightBlock(const QString &text);

    QColor       _commentColor;
    QColor       _errorColor;
    QColor       _extensionColor;
    QColor       _keywordColor;
    QColor       _literalColor;

  private:
    virtual void init();

    QRegExp _kwtest;
    QRegExp _extest;
    QRegExp _numerictest;
    QRegExp _wordtest;
    QRegExp _metasqlquotetest;
    QRegExp _sqlquotetest;

};

#endif // METASQLHIGHLIGHTER_H
