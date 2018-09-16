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

//////////////////////////////////////////////////////////////////////////////
// Known issues
// Data exceeds 2 pages and wrapping equals rows first. Header row not drawn.
// m_columnIndexStoredIteration and m_rowIndexStoredIteration created for that.
// But functionality not implemented.
//
// Image size fixed and corrected after calculations. But displaying large
// tables shows display problems there. Try turning off scaling.
//
// Colors in release mode strange.
//
//////////////////////////////////////////////////////////////////////////////
#include <QDebug>

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

#include "orcrosstab.h" // TODO: renderCrossTab can be static function of CrossTab
#include "crosstab.h"

//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
CrossTab::CrossTab(const QFont& f) : m_rowIndexStored(0) , m_columnIndexStored(0)
{
  m_autoRepaint = true;

  m_commonFont = f;

  // We should have an index at (0,0) even in the column and row descriptions
  // indexes
  m_rowIndex   .insert ("0", CrossTabRow());
  m_columnIndex.insert ("0", CrossTabColumn());

  m_columnIndexStored = 0;
  m_rowIndexStored    = 0;
  m_columnIndexStoredIteration = 0; // See CrossTab::Iterate
  m_rowIndexStoredIteration    = 0; // See CrossTab::Iterate
  m_columnIndexStoredLast = 0;      // See CrossTab::Iterate
  m_rowIndexStoredLast = 0;         // See CrossTab::Iterate

  m_dpiX = 100;
  m_dpiY = 100;

  //Initialize margins
  m_cellLeftMargin   = 3;
  m_cellRightMargin  = 3;
  m_cellTopMargin    = 3;
  m_cellBottomMargin = 3;

  // Redisplay headers
  m_columnHeaderEachPage = true;
  m_rowHeaderEachPage = true;

  // Wrapping
  m_tableWrapDisplayAllColumnsFirst = false;

  // Set populated to false
  m_populated = false;
}


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
CrossTab::~CrossTab()
{
}

//////////////////////////////////////////////////////////////////////////////
// Initializer
//////////////////////////////////////////////////////////////////////////////
void CrossTab::Initialize(ORCrossTabData& ctData, const QMap<QString,QColor>& colorMap)
{
  m_crossTabData = ctData;
  m_colorMap = colorMap;

  // Initialize margins
  m_cellLeftMargin   = ctData.m_tableProperties.m_cellLeftMargin * m_dpiX;
  m_cellRightMargin  = ctData.m_tableProperties.m_cellRightMargin * m_dpiX;
  m_cellTopMargin    = ctData.m_tableProperties.m_cellTopMargin * m_dpiY;
  m_cellBottomMargin = ctData.m_tableProperties.m_cellBottomMargin * m_dpiY;

  // Redisplay headers
  m_columnHeaderEachPage = ctData.m_tableProperties.m_showColumnHeaderOnEachPart;
  m_rowHeaderEachPage    = ctData.m_tableProperties.m_showRowHeaderOnEachPart;

  // Wrapping
  m_tableWrapDisplayAllColumnsFirst = ctData.m_tableProperties.m_wrapPolicyColumnsFirst;

  if ("Left" == ctData.m_column.m_hAlign)
    m_hAlignMap.insert("column", Qt::AlignLeft);
  else if ("Right" == ctData.m_column.m_hAlign)
    m_hAlignMap.insert("column", Qt::AlignRight);
  else
    m_hAlignMap.insert("column", Qt::AlignHCenter);

  if ("Top" == ctData.m_column.m_vAlign)
    m_vAlignMap.insert("column", Qt::AlignTop);
  else if ("Bottom" == ctData.m_column.m_vAlign)
    m_vAlignMap.insert("column", Qt::AlignBottom);
  else
    m_vAlignMap.insert("column", Qt::AlignVCenter);

  if ("Left" == ctData.m_row.m_hAlign)
    m_hAlignMap.insert("row", Qt::AlignLeft);
  else if ("Right" == ctData.m_row.m_hAlign)
    m_hAlignMap.insert("row", Qt::AlignRight);
  else
    m_hAlignMap.insert("row", Qt::AlignHCenter);

  if ("Top" == ctData.m_row.m_vAlign)
    m_vAlignMap.insert("row", Qt::AlignTop);
  else if ("Bottom" == ctData.m_row.m_vAlign)
    m_vAlignMap.insert("row", Qt::AlignBottom);
  else
    m_vAlignMap.insert("row", Qt::AlignVCenter);

  if ("Left" == ctData.m_value.m_hAlign)
    m_hAlignMap.insert("value", Qt::AlignLeft);
  else if ("Right" == ctData.m_value.m_hAlign)
    m_hAlignMap.insert("value", Qt::AlignRight);
  else
    m_hAlignMap.insert("value", Qt::AlignHCenter);

  if ("Top" == ctData.m_value.m_vAlign)
    m_vAlignMap.insert("value", Qt::AlignTop);
  else if ("Bottom" == ctData.m_value.m_vAlign)
    m_vAlignMap.insert("value", Qt::AlignBottom);
  else
    m_vAlignMap.insert("value", Qt::AlignVCenter);
}

//////////////////////////////////////////////////////////////////////////////
// Iterate
//   Reset the indexes at the beginning of a new iteration.
//
//   Current implementatiom has a wrapping policy of first all columns than all
//   rows.
//
//   m_rowIndexStored             - Next index to be written
//                                  Stores the start of the next display.
//   m_columnIndexStored          - NextIndexToBeWritten
//                                  Stores the start of the next display.
//   m_rowIndexStoredLast         - Used when we display the next wrapping part.
//                                  Stores the start of the previous display.
//   m_columnIndexStoredLast      - Used when we display the next wrapping part.
//                                  Stores the start of the previous display.
//   m_columnIndexStoredIteration - Not used - If the wrapping policy changes
//                                  to first all rows this paramter is needed.
//                                  Stores the end of the next display.
//   m_rowIndexStoredIteration    - If on the last page 8 of X rows could be
//                                  displayed than on the next page the table
//                                  should continue with 8 rows.
//                                  Stores the end of the next display.
//////////////////////////////////////////////////////////////////////////////
void CrossTab::Iterate()
{
  if (!m_populated)
  {
    qDebug("CrossTab storage not populated");
    return;
  }

  if (m_tableWrapDisplayAllColumnsFirst)
  {
    // We need to display the next columns
    if ((m_columnIndexStored != 0) && (m_columnIndexStored < m_columnIndex.count()))
    {
      // Current column index is correct
      m_columnIndexStoredLast = m_columnIndexStored;

      // Row index should be reset 
      m_rowIndexStoredLast = m_rowIndexStored;
      m_rowIndexStored = m_rowIndexStoredIteration;
    }
    // Or if the last columns have been displayed we should continue with the
    // next group of rows.

    else if ((m_columnIndexStored != 0) &&
             (m_columnIndexStored == m_columnIndex.count()) &&
             (m_rowIndexStored < m_rowIndexStoredLast))
    {
      // Store the row iteration index
      m_rowIndexStoredIteration    = m_rowIndexStored;
      // Reset the column indexes
      m_columnIndexStored          = m_columnIndexStoredLast;
    }
    else if (((m_columnIndexStored != 0) && (m_columnIndexStored == m_columnIndex.count())))
    {
      // Store the row iteration index
      m_rowIndexStoredIteration    = m_rowIndexStored;
      // Reset the column indexes
      m_columnIndexStored          = 0;
      m_columnIndexStoredLast      = 0;
    }
  }
  else
  {
    // We need to display the next columns
    if ((m_rowIndexStored != 0) && (m_rowIndexStored < m_rowIndex.count()))
    {
      // Current row index is correct

      // Column index should be reset 
      m_columnIndexStoredLast = m_columnIndexStored;
      m_columnIndexStored     = m_columnIndexStoredIteration;
    }
    // Or if the last columns have been displayed we should continue with the
    // next group of rows.
    else if (((m_rowIndexStored != 0) && (m_rowIndexStored == m_rowIndex.count())))
    {
      // Store the column iteration index
      m_columnIndexStoredIteration    = m_columnIndexStored;
      // Reset the column indexes
      m_rowIndexStored          = 0;
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Get/set bounded rectangle
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetRect(const QRect& boundedRectangle)
{
  m_boundedRectangle = boundedRectangle;
  m_rect = boundedRectangle;
}

QRect CrossTab::rect()
{
  return m_boundedRectangle;
}

//////////////////////////////////////////////////////////////////////////////
// Get/set wrapping
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetTableWrappingPolicy(const bool& displayAllColumnsFirst)
{
  m_tableWrapDisplayAllColumnsFirst = displayAllColumnsFirst;
}


bool CrossTab::GetTableWrappingPolicy()
{
  return m_tableWrapDisplayAllColumnsFirst;
}


//////////////////////////////////////////////////////////////////////////////
// Get/set width
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetWidth(const int& width)
{
  m_rect.setWidth(width);
}


int CrossTab::width()
{
  return m_rect.width();
}


//////////////////////////////////////////////////////////////////////////////
// Get/set height
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetHeight(const int& height)
{
  m_rect.setHeight(height);
}

int CrossTab::height()
{
  return m_rect.height();
}

//////////////////////////////////////////////////////////////////////////////
// clear
//////////////////////////////////////////////////////////////////////////////
void CrossTab::clear()
{
  m_data.clear();
  if(AutoRepaint())
  {
    repaint();
  }
}

//////////////////////////////////////////////////////////////////////////////
// Get / set autorepaint
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetAutoRepaint(bool& autoRepaint)
{
  m_autoRepaint = autoRepaint;
}

bool CrossTab::AutoRepaint()
{
  return m_autoRepaint; 
}

//////////////////////////////////////////////////////////////////////////////
// Get / set rows /columns and values
//
// TODO: Row and column index are provided as it is expected to be usefull.
//       That should be checked at the end of the implementation.
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetValue(const QString& column, const QString& row, const QString& value)
{
  // Add to row index
  if (!m_rowIndex.contains (row))
  {
    m_rowIndex.insert (row, CrossTabRow());
  }

  // Add to value index
  if (!m_columnIndex.contains (column))
  {
    m_columnIndex.insert (column, CrossTabColumn());
  }

  // Store value
  CrossTabStorageKey key(row, column);
  if (!m_data.contains (key))
  {
    m_data.insert(key, value);
  }
  else
  {
    qDebug("CrossTab data contains duplicate values: (%s, %s): %s. Not used.", row.toLatin1().data(), column.toLatin1().data(), value.toLatin1().data());
  }
}

QString CrossTab::GetValue(const QString& column, const QString& row)
{
  QString returnValue;
  CrossTabStorageKey key(row, column);

  if (m_data.contains(key))
  {
    returnValue = m_data.value(key);
  }

  return returnValue;
}

//////////////////////////////////////////////////////////////////////////////
// Get / set rows /columns and values
//
// TODO: Row and column index are provided as it is expected to be usefull.
//       That should be checked at the end of the implementation.
//////////////////////////////////////////////////////////////////////////////
void CrossTab::SetFont(const QFont& font)
{
  m_commonFont = font;
}

QFont CrossTab::GetFont() const
{
  return m_commonFont;
}


//////////////////////////////////////////////////////////////////////////////
// If the index maps are filled we can calculate the min size of the rows and
// columns
//////////////////////////////////////////////////////////////////////////////
void CrossTab::CalculateCrossTabMeasurements()
{
  // Calculate rows measurements
  {
    CrossTabRowIndex::iterator    itRow;
    for (itRow = m_rowIndex.begin(); itRow != m_rowIndex.end(); ++itRow)
    {
      QRect rect;
      QFontMetrics fm(GetFont());
      QString data;
      CrossTabColumnIndex::iterator    itCol;
      for (itCol = m_columnIndex.begin(); itCol != m_columnIndex.end(); ++itCol)
      {
        // TODO: Implicite assumption that first row is index
        // Get data from index
        if ((itRow == m_rowIndex.begin()) && (itCol != m_columnIndex.begin()))
        {
          data = itCol.key();
        }
        else if ((itRow != m_rowIndex.begin()) && (itCol == m_columnIndex.begin()))
        {
          data = itRow.key();
        }
        if ((itRow == m_rowIndex.begin()) && (itCol == m_columnIndex.begin()))
        {
          data = "0";
        }
        // Get data from storage
        else
        {
          data = GetValue(itCol.key(), itRow.key());
        }

        // Determine bouding rectangle
        rect = fm.boundingRect(data);
        // Adjust row properties
        if (itRow.value().m_rowMaxHeight < rect.height())
        {
          itRow.value().m_rowMaxHeight = rect.height();
        }
        // Adjust table properties
        m_tableProperties.m_tableMaxHeight += rect.height();
      }
      // TODO: Statically set 
      itRow.value().m_rowVAlign = Qt::AlignVCenter;
    }
  }

  // Calculate column measurements
  {
    CrossTabColumnIndex::iterator    itCol;
    for (itCol = m_columnIndex.begin(); itCol != m_columnIndex.end(); ++itCol)
    {
      QRect rect;
      QFontMetrics fm(GetFont());
      QString data;
      CrossTabRowIndex::iterator    itRow;
      for (itRow = m_rowIndex.begin(); itRow != m_rowIndex.end(); ++itRow)
      {
        // TODO: Implicite assumption that first col is index
        // Get data from row index
        if ((itCol == m_columnIndex.begin()) && (itRow != m_rowIndex.begin()))
        {
          data = itRow.key();
        }
        // Get data from col index
        else if ((itCol != m_columnIndex.begin()) && (itRow == m_rowIndex.begin()))
        {
          data = itCol.key();
        }
        // Get data from row/col index
        else if ((itCol == m_columnIndex.begin()) && (itRow == m_rowIndex.begin()))
        {
          data = "0";
        }
        // Get data from storage
        else
        {
          data = GetValue(itCol.key(), itRow.key());
        }
        // Determine bouding rectangle
        rect = fm.boundingRect(data);
        // Determine column properties
        if (itCol.value().m_columnMaxWidth < rect.width())
        {
          itCol.value().m_columnMaxWidth = rect.width();
        }
        // Adjust table properties
        m_tableProperties.m_tableMaxWidth += rect.width();
      }
      // TODO: Statically set 
      itCol.value().m_columnHAlign = Qt::AlignHCenter;
    }
  }

  // TODO: Should be done somewhere else
 SetHeight(m_tableProperties.m_tableMaxHeight);

 return;
}


//////////////////////////////////////////////////////////////////////////////
// Calculate the last index of column and row that can be displayed
//
// Parameters
//   lastColumn [out] - last column Id that can be displayed
//   lastRow    [out] - last row Id that can be displayed
//   rect       [in]  - available space for display of table
//              [out] - used space for display of table
//
// TODO: Width and height initialised with 2 which is the border of the table.
//       There is no way to change the border/inner line of the table.
//       Thus assumed 1 pixel.
//////////////////////////////////////////////////////////////////////////////
void CrossTab::CalculateDisplayedRowsAndColumns(int& lastColumn, int& lastRow, QRect& rect)
{
  // Keep interation in mind
  lastColumn = m_columnIndexStored;
  lastRow    = m_rowIndexStored;

  // Calculate columns
  {
    int width(1);
    bool filled(false);
    CrossTabColumnIndex::iterator columnIt(m_columnIndex.begin());
    for (int j=0 ; !filled && (columnIt != m_columnIndex.end()); ++j, ++columnIt)
    {
      //        Keep header in mind             Keep iteration in mind
      if ((m_rowHeaderEachPage && (j==0)) || (j >= m_columnIndexStored))
      {
        // Calculate new size and index
        width += columnIt.value().m_columnMaxWidth + m_cellLeftMargin + m_cellRightMargin;
        lastColumn = j;

        // Exceeding the size
        if (width > rect.width() && j > m_columnIndexStored)
        {
          // Revert and stop
          lastColumn = j-1;
          width -= (columnIt.value().m_columnMaxWidth + m_cellLeftMargin + m_cellRightMargin);
          filled = true;
        }
      }
    }
    rect.setWidth(width);
  }

  // Calculate rows
  {
    int height(1);
    bool filled(false);
    CrossTabRowIndex::iterator    rowIt(m_rowIndex.begin());
    for (int i=0 ; !filled && (rowIt != m_rowIndex.end()); ++i, ++rowIt)
    {
      // If the table was wrapped to a new page we should not print more rows
      // than the first part of the table
      if (m_tableWrapDisplayAllColumnsFirst && (i >= m_rowIndexStoredLast) && (0 != m_columnIndexStored))
      {
        break;
      }

      //        Keep header in mind             Keep iteration in mind
      if ((m_columnHeaderEachPage && (i==0)) || (i >= m_rowIndexStored))
      {
        // Calculate new size and index
        height += rowIt.value().m_rowMaxHeight + m_cellTopMargin + m_cellBottomMargin;
        lastRow = i;

        // Exceeding the size
        if (height > rect.height())
        {
          // Revert and stop
          lastRow = i-1;
          height -= (rowIt.value().m_rowMaxHeight + m_cellTopMargin + m_cellBottomMargin);
          filled = true;
        }
      }
    }
    rect.setHeight(height);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Calculate space the table will use
//   Kind of overloaded function of: CalculateDisplayedRowsAndColumns
// Parameters
//   rect       [in]  - available space for display of table
//              [out] - used space for display of table
//////////////////////////////////////////////////////////////////////////////
void CrossTab::CalculateSize(QRect& rect)
{
  int tempColumnId(0);
  int tempRowId(0);

  CalculateDisplayedRowsAndColumns(tempColumnId, tempRowId, rect);
}


//////////////////////////////////////////////////////////////////////////////
// Calculate the size the remaining table needs
//
// Parameters
//   rect       [out] - used space for display of table
//
// TODO
//   Redisplay of headers columns and rows not calculated.
//
//////////////////////////////////////////////////////////////////////////////
void CrossTab::CalculateTableSize(QRect& rect)
{
  // Calculate columns
  {
    int width(1);
    CrossTabColumnIndex::iterator columnIt(m_columnIndex.begin());
    for (int j=0 ; columnIt != m_columnIndex.end() ; ++j, ++columnIt)
    {
      //         Keep header in mind            Keep iteration in mind
      if ((m_rowHeaderEachPage && (j==0)) || ((j >= m_columnIndexStored)))
      {
        // Calculate new size and index
        width += columnIt.value().m_columnMaxWidth + m_cellLeftMargin + m_cellRightMargin;
      }
    }
    rect.setWidth(width);
  }

  // Calculate rows
  {
    int height(1);
    CrossTabRowIndex::iterator    rowIt(m_rowIndex.begin());
    for (int i=0 ; rowIt != m_rowIndex.end() ; ++i, ++rowIt)
    {
      //        Keep header in mind             Keep iteration in mind
      if ((m_columnHeaderEachPage && (i==0)) || (i >= m_rowIndexStored))
      {
        // Calculate new size and index
        height += rowIt.value().m_rowMaxHeight + m_cellTopMargin + m_cellBottomMargin;
      }
    }
    rect.setHeight(height);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Calculate the height of the next row to be displayed.
//   If a header row should be included this is calculated also.
//
// Assumptions
//   This function should be called in an itteration after Drawing and before
//   calling the Iteration function.
//
// Parameters
//   height [out] - Calculated height
//
// TODO: Width and height initialised with 1 which is the border of the table.
//       There is no way to change the border/inner line of the table.
//       Thus assumed 1 pixel.
//////////////////////////////////////////////////////////////////////////////
void CrossTab::CalculateNextRowSize(int& height)
{
  // Calculate which row should be calculated
  int startIndex = m_rowIndexStored;
  if (m_rowIndexStored == m_rowIndex.count())
  {
     startIndex = m_rowIndexStoredLast;
  }

  // Calculate rows
  height = 1;
  bool includedRow(false);
  CrossTabRowIndex::iterator    rowIt(m_rowIndex.begin());
  for (int i=0 ; rowIt != m_rowIndex.end() ; ++i, ++rowIt)
  {
    //   Keep header in mind                  Keep iteration in mind
    if ((m_columnHeaderEachPage && (i==0)) || (i >= startIndex))
    {
      // Calculate new size and index
      height += rowIt.value().m_rowMaxHeight + m_cellTopMargin + m_cellBottomMargin;
      if (i >= startIndex)
      {
        includedRow = true;
        break;
      }
    }
  }

  // Height is zero when no rows should be displayed anymore
  if (!includedRow)
  {
    height = 0;
  }
}


//////////////////////////////////////////////////////////////////////////////
// Calculate the last index of column and row that can be displayed
//////////////////////////////////////////////////////////////////////////////
bool CrossTab::AllDataRendered()
{
  return ((m_rowIndex.count() == (m_rowIndexStored)) &&
          (m_columnIndex.count() == (m_columnIndexStored)));
}


//////////////////////////////////////////////////////////////////////////////
// draw
//////////////////////////////////////////////////////////////////////////////
void CrossTab::Draw(QPainter & paint)
{
  paint.save();

  // Calculate the number of rows and columns that we can display
  int lastColumn(0);
  int lastRow(0);

  CalculateDisplayedRowsAndColumns(lastColumn, lastRow, m_rect);

  paint.drawRect(m_rect);

  QRect sampleRect(0,0,0,0); // Name itemRectangle
  QRect saveRect(0,0,0,0);

  int idRow=m_rowIndexStored;
  int idCol=m_columnIndexStored;

  // Start at the correct place
  CrossTabRowIndex::iterator    rowIt(m_rowIndex.begin());
  CrossTabColumnIndex::iterator columnIt(m_columnIndex.begin());
  for (idRow = 0, rowIt = m_rowIndex.begin() ; ((idRow <= lastRow) && (rowIt != m_rowIndex.end())); ++idRow, ++rowIt)
  {
    // Should this row cell be displayed
    //    Skip header column if not wanted
    if ( ((0 == idRow) && (0 != m_rowIndexStored) && !m_columnHeaderEachPage) ||
         //Skip already displayed rows          
         ((0 < idRow) && (idRow < m_rowIndexStored)) ||
         //Skip rows that are not part of the already drawn table part
         (m_tableWrapDisplayAllColumnsFirst && (0 < idRow) && (idRow > m_rowIndexStoredLast) && (0 != m_columnIndexStored)) )
    {
      continue;
    }

    for (idCol = 0, columnIt = m_columnIndex.begin() ; ((idCol <= lastColumn) && (columnIt != m_columnIndex.end())); ++idCol, ++columnIt)
    {
      // Should this column cell be displayed
           // Skip header row if not wanted
      if (((0 == idCol) && (0 != m_columnIndexStored) && !m_rowHeaderEachPage) ||
           // Skip already displayed columns
          ((0 < idCol) && (idCol < m_columnIndexStored)))
      {
        continue;
      }

      // Get width of this column
      QString dataTekst (columnIt.key());
      sampleRect.setWidth  (columnIt.value().m_columnMaxWidth + m_cellLeftMargin + m_cellRightMargin);
      sampleRect.setHeight (rowIt.value().m_rowMaxHeight + m_cellTopMargin + m_cellBottomMargin);

      // Save rectangle
      saveRect = sampleRect;

      paint.setBackgroundMode(Qt::OpaqueMode);
      // Draw headers row
      if  (idRow == 0)
      {
        paint.setBrush(QBrush(QColor(0, 0, 255, 127), Qt::SolidPattern));
      }
      // Header column
      else if  ((idRow != 0) && (idCol == 0))
      {
        // Odd rows
        if ((idRow%2) != 0 )
        {
          paint.setBrush(QBrush(QColor(0, 0, 255, 200), Qt::SolidPattern));
        }
        // Even rows
        else
        {
          paint.setBrush(QBrush(QColor(90, 210, 255, 150), Qt::SolidPattern));
        }
      }
      // Values
      else if  ((idRow != 0) && (idCol != 0))
      {
        // Odd rows
        if ((idRow%2) != 0 )
        {
          paint.setBrush(QBrush(QColor(0, 0, 0, 0), Qt::SolidPattern));
        }
        // Even rows
        else
        {
          paint.setBrush(QBrush(QColor(90, 210, 255, 127), Qt::SolidPattern));
        }
      }

      // Draw rectangle
      paint.drawRect(sampleRect);

      paint.setBackgroundMode(Qt::TransparentMode);
      paint.setPen(Qt::SolidLine);

      // Skip margins: Adjust rectangle for data insertion
      sampleRect.setX(sampleRect.x() + m_cellLeftMargin);
      sampleRect.setY(sampleRect.y() + m_cellTopMargin);
      sampleRect.setWidth  (columnIt.value().m_columnMaxWidth);
      sampleRect.setHeight (rowIt.value().m_rowMaxHeight);

      // get Font
      QFontMetrics fm(GetFont());
      QRect        dataRect;

      // Header
      if ((idRow == 0) && (idCol == 0))
      {
        // Do nothing
      }
      // Headerrow
      else if ((idRow == 0) && (idCol != 0))
      {
        dataRect = fm.boundingRect(sampleRect, m_hAlignMap["column"] | m_vAlignMap["column"], columnIt.key());
        paint.drawText(dataRect, m_hAlignMap["column"] | m_vAlignMap["column"], columnIt.key());
      }
      // Headercol
      else if ((idCol == 0) && (idRow != 0))
      {
        dataRect = fm.boundingRect(sampleRect, m_hAlignMap["row"] | m_vAlignMap["row"], rowIt.key());
        paint.drawText(dataRect, m_hAlignMap["row"] | m_vAlignMap["row"], rowIt.key());
      }
      // Value
      else
      {
        dataRect = fm.boundingRect(sampleRect, m_hAlignMap["value"] | m_vAlignMap["value"], GetValue(columnIt.key(), rowIt.key()));
        paint.drawText(dataRect, m_hAlignMap["value"] | m_vAlignMap["value"], GetValue(columnIt.key(), rowIt.key()));
      }
      // Restore rectangle
      sampleRect = saveRect;
      sampleRect.setX(sampleRect.x() + sampleRect.width());
    } // end for

    // Reset basic column id
    idCol = m_columnIndexStored;

    // Adjust item display rectangle
    sampleRect.setY(sampleRect.y()+sampleRect.height());
    sampleRect.setX(0);
  }

  // Store indexes for iterative purpose
  m_rowIndexStored    = lastRow + 1;
  m_columnIndexStored = lastColumn + 1;

  // Now that we are done return the paint device back to the state
  // it was when we started to mess with it
  paint.restore();
}

//////////////////////////////////////////////////////////////////////////////
// repaint event
//////////////////////////////////////////////////////////////////////////////
void CrossTab::repaint() 
{
}

//////////////////////////////////////////////////////////////////////////////
// renderCrossTab
//
//   populate the internal storage from the query
//////////////////////////////////////////////////////////////////////////////
void CrossTab::PopulateFromQuery(XSqlQuery* query)
{
  // Query for the data
  if(query->first())
  {
    do
    {
      QString columnValue = query->value(m_crossTabData.m_column.m_query).toString();
      QString rowValue    = query->value(m_crossTabData.m_row.m_query).toString();
      QString valueValue  = query->value(m_crossTabData.m_value.m_query).toString();
      SetValue(columnValue, rowValue, valueValue);
    } while(query->next());
    m_populated = true;
  }
}
