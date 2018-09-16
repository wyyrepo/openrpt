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

// TODO: remove copyright() and version() methods

#include "reporthandler.h"
#include "reportgridoptions.h"
#include "data.h"
#include "documentwindow.h"
#include "documentview.h"
#include "documentscene.h"
#include "graphicsitems.h"
#include "graphicssection.h"
#include "fontutils.h"

// dialogs
#include "dbfiledialog.h"
#include "editpreferences.h"

// common
#include "login.h"
#include "parsexmlutils.h"
#include "parameter.h"
#include "parameteredit.h"
#include "previewdialog.h"
#include "xsqlquery.h"
#include "memdbloader.h"
#include "labelsizeinfo.h"

// renderer
#include <../renderer/openreports.h>
#include <../renderer/renderobjects.h>
#include <../renderer/orprerender.h>
#include <../renderer/orprintrender.h>

// qt
#include <QApplication>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QCursor>
#include <QEvent>
#include <QToolBar>
#include <QMenuBar>
#include <QRect>
#include <QMainWindow>
#include <QAction>
#include <QPixmap>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QSpinBox>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QPrintDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QColorDialog>
#include <QMdiArea>
#include <QMdiSubWindow>

extern bool __dosnap;

//int dpi  = 72; // assume a 72 dpi until we can get to the code that sets the proper value
int dpiX = 72;
int dpiY = 72;


QRect padRect(QRect r)
{
  return QRect(r.x() - 3, r.y() - 3, r.width() + 6, r.height() + 6);
}

class ReportWriterCopyData
{
  public:
    ReportWriterCopyData()
    {
      copy_item = 0;
    }

    // copy information
    QPointF copy_offset;

    int copy_item; // the item type that we have information on

    QString copy_str1;     // string 1 (label:text  field/text/barcode/image:queryname)
    QString copy_str2;     // string 2 (field/text/barcode/image:column)
    QString copy_str3;     // string 3 (barcode:format image:inlineImageData field:trackTotalFormat)
    QString copy_str4;     // string 4 (image:mode)

    QFont copy_font;       // font (label/field/text:font)
    QRectF copy_rect;       // rect (all but line)

    double copy_dbl1;       // text bottom padding
    double copy_dbl2;
    int    copy_int1;      // line:startx barcode:maxlength
    int    copy_int2;      // line:starty barcode:alignment
    int    copy_int3;      // line:endx
    int    copy_int4;      // line:endy
    int    copy_int5;      // line:weight

    QPointF copy_line_start;
    QPointF copy_line_end;

    bool   copy_bool1;     // image:isInline field:trackTotal
    bool   copy_bool2;     // field: trackBuiltinFormat
    bool   copy_bool3;     // field: useSubTotal
    bool   copy_bool4;
    bool   copy_bool5;

    QBrush copy_brush;     // all entities
    QPen   copy_pen;       // all entities
    QPen   copy_border;    // all entities

    ORGraphData copy_graph; // graph
};

//
// define and implement the ReportWriterSectionData class
// a simple class to hold/hide data in the ReportHandler class
//
class ReportWriterSectionData
{
  public:
    ReportWriterSectionData()
    {
      mouseAction = ReportWriterSectionData::MA_None;
      insertItem = NoItem;
    }
    virtual ~ReportWriterSectionData()
    {
    }

    enum ItemType
    {
      NoItem = 0,
      LabelItem,
      FieldItem,
      TextItem,
      LineItem,
      BarcodeItem,
      ImageItem,
      GraphItem,
      RectItem,
      CrossTabItem
    };
    enum MouseAction
    {
      MA_None = 0,
      MA_Insert = 1,
      MA_Grab = 2,
      MA_MoveStartPoint,
      MA_MoveEndPoint,
      MA_ResizeNW = 8,
      MA_ResizeN,
      MA_ResizeNE,
      MA_ResizeE,
      MA_ResizeSE,
      MA_ResizeS,
      MA_ResizeSW,
      MA_ResizeW
    };

    int selected_x_offset;
    int selected_y_offset;

    MouseAction mouseAction;
    ItemType insertItem;

    // copy data
    QPointF copy_pos;      // the base position of the copy (typically the first items original pos)
    QList<ReportWriterCopyData> copy_list;
};


//
// Constructor(s)
//
ReportHandler::ReportHandler(QObject * parent, const char * name)
  : QObject(parent)
{
  if(name)
    setObjectName(name);

  // initialize data
  _parentWindow = 0;
  _mdiParentWindow = 0;
  _noDatabase = false;
  _allowDBConnect = true;
  _placeMenusOnWindows = false;
  _placeToolbarsOnWindows = false;

  _strMenuConnect = QObject::tr("Connect to Database");
  _strMenuDisconnect = QObject::tr("Disconnect from Database");


  Q_INIT_RESOURCE(OpenRPTWrtembed);

  gridOptions = new ReportGridOptions(qApp->desktop()->logicalDpiX(),
                                      qApp->desktop()->logicalDpiY(),
                                      this, "grid options");
  sectionData = new ReportWriterSectionData();

  grpDoc = new QActionGroup(this);
  grpDoc->setExclusive(false);
  fileNewAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/document_size.png")),
                              tr("&New File"),this);
  fileNewAction->setShortcut(QKeySequence::New);
  fileOpenAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/files_zoom_size.png")),
                               tr("&Open File..."),this);
  fileOpenAction->setShortcut(QKeySequence::Open);
  fileSaveAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/diskette_size.png")),
                               tr("&Save File"),grpDoc);
  fileSaveAction->setShortcut(QKeySequence::Save);
  fileSaveAsAction = new QAction( tr("Save &As..."),grpDoc);
  fileCloseAction = new QAction( tr("&Close"),grpDoc);
  fileCloseAction->setShortcut(QKeySequence::Close);
  filePreviewAction = new QAction( QIcon(getIconName(QString(":/OpenRPT/icons/icons_size/diskette_size.png"))),
                                   tr("Print Preview..."), grpDoc);
  filePrintAction = new QAction(tr("Print..."), grpDoc);
  filePrintAction->setShortcut(QKeySequence::Print);
  filePrintToPDFAction = new QAction(tr("Print to PDF..."), grpDoc);
  fileExitAction = new QAction( tr("E&xit"),this);
  fileExitAction->setShortcut(Qt::ALT+Qt::Key_F4);

  grpEdit = new QActionGroup(this);
  grpEdit->setExclusive(false);

  editUndoAction = new QAction(tr("Undo"), grpEdit);
  editUndoAction->setShortcut(QKeySequence::Undo);
  editRedoAction = new QAction(tr("Redo"), grpEdit);
  editRedoAction->setShortcut(QKeySequence::Redo);

  editCutAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/scissors_size.png")),
                              tr("Cu&t"), grpEdit);
  editCutAction->setShortcut(QKeySequence::Cut);
  editCopyAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/copy_size.png")),
                               tr("&Copy"), grpEdit);
  editCopyAction->setShortcut(QKeySequence::Copy);
  editPasteAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/paste_size.png")),
                                tr("&Paste"), grpEdit);
  editPasteAction->setShortcut(QKeySequence::Paste);
  editDeleteAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/delete_size.png")),
                                 tr("&Delete"), grpEdit);
  editDeleteAction->setShortcut(QKeySequence::Delete);
  editPreferencesAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/font_size.png")),
                                      tr("Preferences"), this);
  editSelectAllAction = new QAction( tr("Select All"), this);
  editPropertiesAction = new QAction( tr("Properties"), this);

  editZoomInAction = new QAction( tr("Zoom In"), this);
  editZoomInAction->setShortcut(QKeySequence::ZoomIn);
  editZoomOutAction = new QAction( tr("Zoom Out"), this);
  editZoomOutAction->setShortcut(QKeySequence::ZoomOut);

  grpGrid = new QActionGroup(this);
  grpGrid->setExclusive(false);
  gridShowAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/numeric_field_size.png")),
                               tr("Show Grid"), grpGrid);
  gridShowAction->setCheckable(true);
  gridShowAction->setChecked(gridOptions->isVisible());

  gridSnapAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/numeric_field_ok_size.png")),
                               tr("Snap to Grid"), grpGrid);
  gridSnapAction->setCheckable(true);
  gridSnapAction->setChecked(gridOptions->isSnap());

  grpItem = new QActionGroup(this);
  grpItem->setExclusive(true);
  itemLabelAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/attribute_size.png")),
                                tr("Insert Label"), grpItem);
  itemFieldAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/field_size.png")),
                                tr("Insert Field"), grpItem);
  itemTextAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/text_field_size.png")),
                               tr("Insert Text"), grpItem);
  itemLineAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/line_size.png")),
                               tr("Insert Line"), grpItem);
  itemRectAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/box_size.png")),
                                tr("Insert Rectangle"), grpItem);
  itemCrossTabAction = new QAction( QIcon(getIconName(QString(":/OpenRPT/icons/icons_size/crosstab_size.png"))),
                                    tr("Insert CrossTab"), grpItem);
  itemBarcodeAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/barcode_size.png")),
                                  tr("Insert Bar Code"), grpItem);
  itemImageAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/image_field_size.png")),
                                tr("Insert Image"), grpItem);
  itemGraphAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/statistic_size.png")),
                                tr("Insert Chart/Graph"), grpItem);

  foreach(QAction *action, grpItem->actions())
      action->setCheckable(true);

  dbConnectAction = new QAction( _strMenuConnect, this);
  dbLoadAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/export_database_size.png")),
                             tr("Load from Database"), this);
  dbSaveAction = new QAction( QIcon(getIconName(":/OpenRPT/icons/icons_size/database_add_size.png")),
                             tr("Save to Database"), this);

  docTitleAction           = new QAction(tr("Properties..."),         grpDoc);
  docPageSetupAction       = new QAction(tr("&Page Setup..."),        grpDoc);
  docQuerySourceListAction = new QAction(tr("Query &Sources..."),     grpDoc);
  docSectionEditorAction   = new QAction(tr("Section Editor..."),     grpDoc);
  docColorListAction       = new QAction(tr("Color Definitions..."),  grpDoc);
  docDefinedParamsAction   = new QAction(tr("Defined Parameters..."), grpDoc);

  // Added label definitions action
  docLabelDefinitionsAction = new QAction(tr("Label Definitions..."), grpDoc);

  grpAlign = new QActionGroup(this);
  alignTopAction = new QAction(tr("Align Top"), grpAlign);
  alignVCenterAction = new QAction(tr("Align V. Center"), grpAlign);
  alignBottomAction = new QAction(tr("Align Bottom"), grpAlign);
  alignLeftAction = new QAction(tr("Align Left"), grpAlign);
  alignHCenterAction = new QAction(tr("Align H. Center"), grpAlign);
  alignRightAction = new QAction(tr("Align Right"), grpAlign);
  evenHSpacingAction = new QAction(tr("Even Horizontal Spacing"), grpAlign);
  evenVSpacingAction = new QAction(tr("Even Vertical Spacing"), grpAlign);

/*
  cbFont = new QFontComboBox();
  cbFont->clearEditText();

  cbFontSize = new QComboBox();
  cbFontSize->setEditable(true);
  QStringList sizesList;
  sizesList << "6" << "7" << "8" << "9" << "10" << "11" << "12" << "14" << "16" << "18" << "20" << "24" << "28" << "36" << "48" << "72";
  cbFontSize->addItems(sizesList);
  cbFontSize->clearEditText();

  btFontWeight = new QPushButton(tr("B"));
  btFontWeight->setFixedWidth(24);
  btFontWeight->setCheckable(true);
  QFont font = btFontWeight->font();
  font.setBold(true);
  btFontWeight->setFont(font);

  btFontStyle = new QPushButton(tr("i"));
  btFontStyle->setFixedWidth(24);
  btFontStyle->setCheckable(true);
  font = btFontStyle->font();
  font.setItalic(true);
  btFontStyle->setFont(font);
*/

  grpFont = new QActionGroup(this);
  grpFont->setExclusive(false);
  
  grpProperties = new QActionGroup(this);
  colorAction = new QAction(tr("Color"), grpProperties);
  fillAction = new QAction(tr("Fill"), grpProperties);
  borderAction = new QAction(tr("Border"), grpProperties);
  rotationAction = new QAction(tr("Rotation"), grpProperties);


  // connect the actions to various slots so they functions properly
  connect(fileNewAction, SIGNAL(triggered()), this, SLOT(fileNew()));
  connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(fileOpen()));
  connect(fileSaveAction, SIGNAL(triggered()), this, SLOT(fileSave()));
  connect(fileSaveAsAction, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
  connect(fileCloseAction, SIGNAL(triggered()), this, SLOT(fileClose()));
  connect(filePreviewAction, SIGNAL(triggered()), this, SLOT(filePreview()));
  connect(filePrintAction, SIGNAL(triggered()), this, SLOT(filePrint()));
  connect(filePrintToPDFAction, SIGNAL(triggered()), this, SLOT(filePrintToPDF()));
  connect(editUndoAction, SIGNAL(triggered()), this, SLOT(editUndo()));
  connect(editRedoAction, SIGNAL(triggered()), this, SLOT(editRedo()));
  connect(editZoomInAction, SIGNAL(triggered()), this, SLOT(editZoomIn()));
  connect(editZoomOutAction, SIGNAL(triggered()), this, SLOT(editZoomOut()));
  connect(editCutAction, SIGNAL(triggered()), this, SLOT(editCut()));
  connect(editCopyAction, SIGNAL(triggered()), this, SLOT(editCopy()));
  connect(editPasteAction, SIGNAL(triggered()), this, SLOT(editPaste()));
  connect(editDeleteAction, SIGNAL(triggered()), this, SLOT(editDelete()));
  connect(editSelectAllAction, SIGNAL(triggered()), this, SLOT(editSelectAll()));
  connect(editPreferencesAction, SIGNAL(triggered()), this, SLOT(editPreferences()));
  connect(editPropertiesAction, SIGNAL(triggered()), this, SLOT(editProperties()));
  connect(gridShowAction, SIGNAL(toggled(bool)), gridOptions, SLOT(setVisible(bool)));
  connect(gridSnapAction, SIGNAL(toggled(bool)), gridOptions, SLOT(setSnap(bool)));
  connect(itemLabelAction, SIGNAL(triggered()), this, SLOT(itemLabel()));
  connect(itemFieldAction, SIGNAL(triggered()), this, SLOT(itemField()));
  connect(itemTextAction, SIGNAL(triggered()), this, SLOT(itemText()));
  connect(itemLineAction, SIGNAL(triggered()), this, SLOT(itemLine()));
  connect(itemRectAction, SIGNAL(triggered()), this, SLOT(itemRect()));
  connect(itemCrossTabAction, SIGNAL(triggered()), this, SLOT(itemCrossTab()));
  connect(itemBarcodeAction, SIGNAL(triggered()), this, SLOT(itemBarcode()));
  connect(itemImageAction, SIGNAL(triggered()), this, SLOT(itemImage()));
  connect(itemGraphAction, SIGNAL(triggered()), this, SLOT(itemGraph()));
  connect(dbConnectAction, SIGNAL(triggered()), this, SLOT(dbConnect()));
  connect(dbLoadAction, SIGNAL(triggered()), this, SLOT(dbLoadDoc()));
  connect(dbSaveAction, SIGNAL(triggered()), this, SLOT(dbSaveDoc()));
  connect(docTitleAction, SIGNAL(triggered()), this, SLOT(docTitle()));
  connect(docPageSetupAction, SIGNAL(triggered()), this, SLOT(docPageSetup()));
  connect(docQuerySourceListAction, SIGNAL(triggered()), this, SLOT(docQuerySourceList()));
  connect(docSectionEditorAction, SIGNAL(triggered()), this, SLOT(docSectionEditor()));
  connect(docColorListAction, SIGNAL(triggered()), this, SLOT(docColorList()));
  connect(docDefinedParamsAction, SIGNAL(triggered()), this, SLOT(docDefinedParams()));
  connect(docLabelDefinitionsAction, SIGNAL(triggered()), this, SLOT(docLabelDefinitions()));
  connect(alignTopAction, SIGNAL(triggered()), this, SLOT(alignTop()));
  connect(alignVCenterAction, SIGNAL(triggered()), this, SLOT(alignVCenter()));
  connect(alignBottomAction, SIGNAL(triggered()), this, SLOT(alignBottom()));
  connect(alignLeftAction, SIGNAL(triggered()), this, SLOT(alignLeft()));
  connect(alignHCenterAction, SIGNAL(triggered()), this, SLOT(alignHCenter()));
  connect(alignRightAction, SIGNAL(triggered()), this, SLOT(alignRight()));
  connect(evenHSpacingAction, SIGNAL(triggered()), this, SLOT(evenHSpacing()));
  connect(evenVSpacingAction, SIGNAL(triggered()), this, SLOT(evenVSpacing()));
  connect(colorAction, SIGNAL(triggered()), this, SLOT(color()));
  connect(fillAction, SIGNAL(triggered()), this, SLOT(fill()));
  connect(borderAction, SIGNAL(triggered()), this, SLOT(border()));
  connect(rotationAction, SIGNAL(triggered()), this, SLOT(rotation()));
}

ReportHandler::~ReportHandler()
{
    if (sectionData != 0)
        delete sectionData;
}

//
// toolbar and menu creation methods
//
void ReportHandler::docToolBars(QMainWindow * mw, int /*edge*/, bool /*newLine*/)
{
  if(mw)
  {
    // create the toolbars
    QToolBar * tbFiles = mw->addToolBar(tr("File Operations"));
    tbFiles->setObjectName("tbFiles");
    tbFiles->setIconSize(QSize(32, 32));
    tbFiles->addAction(fileNewAction);
    tbFiles->addAction(fileOpenAction);
    tbFiles->addAction(fileSaveAction);

    if(!_noDatabase)
    {
      QToolBar * tbDatabase = mw->addToolBar(tr("Database Operations"));
      tbDatabase->setObjectName("tbDatabase");
      tbDatabase->setIconSize(QSize(32, 32));
      tbDatabase->addAction(dbLoadAction);
      tbDatabase->addAction(dbSaveAction);
    }

    QToolBar * tbEdit = mw->addToolBar(tr("Edit Operations"));
    tbEdit->setObjectName("tbEdit");
    tbEdit->setIconSize(QSize(32, 32));
    tbEdit->addAction(editCutAction);
    tbEdit->addAction(editCopyAction);
    tbEdit->addAction(editPasteAction);
    tbEdit->addAction(editDeleteAction);
  
    QToolBar * tbOptions = mw->addToolBar(tr("Layout Options"));
    tbOptions->setObjectName("tbOptions");
    tbOptions->setIconSize(QSize(32, 32));
    tbOptions->addAction(gridShowAction);
    tbOptions->addAction(gridSnapAction);
    tbOptions->addAction(editPreferencesAction);

    QToolBar * tbItems = mw->addToolBar(tr("Report Elements"));
    tbItems->setObjectName("tbItems");
    mw->addToolBar(Qt::RightToolBarArea, tbItems);
    tbItems->setIconSize(QSize(32, 32));
    tbItems->addAction(itemLabelAction);
    tbItems->addAction(itemFieldAction);
    tbItems->addAction(itemTextAction);
    tbItems->addAction(itemLineAction);
    tbItems->addAction(itemCrossTabAction);
    tbItems->addAction(itemRectAction);
    tbItems->addAction(itemBarcodeAction);
    tbItems->addAction(itemImageAction);
    tbItems->addAction(itemGraphAction);

    QFontComboBox * lcbFont = new QFontComboBox();
    lcbFont->setObjectName("font_family");
    lcbFont->clearEditText();

    QComboBox * lcbFontSize = new QComboBox();
    lcbFontSize->setObjectName("font_size");
    lcbFontSize->setEditable(true);
    QStringList sizesList;
    sizesList << "6" << "7" << "8" << "9" << "10" << "11" << "12" << "14" << "16" << "18" << "20" << "24" << "28" << "36" << "48" << "72";
    lcbFontSize->addItems(sizesList);
    lcbFontSize->clearEditText();
  
    QPushButton * lbtFontWeight = new QPushButton(tr("B"));
    lbtFontWeight->setObjectName("font_weight");
    lbtFontWeight->setFixedWidth(24);
    lbtFontWeight->setCheckable(true);
    QFont font = lbtFontWeight->font();
    font.setBold(true);
    lbtFontWeight->setFont(font);
  
    QPushButton * lbtFontStyle = new QPushButton(tr("i"));
    lbtFontStyle->setObjectName("font_style");
    lbtFontStyle->setFixedWidth(24);
    lbtFontStyle->setCheckable(true);
    font = lbtFontStyle->font();
    font.setItalic(true);
    lbtFontStyle->setFont(font);

    connect(lcbFont, SIGNAL(currentIndexChanged (const QString&)), this, SLOT(fontChange(const QString&)));
    connect(lcbFontSize, SIGNAL(currentIndexChanged (const QString&)), this, SLOT(fontSizeChange(const QString&)));
    connect(lbtFontStyle, SIGNAL(clicked (bool)), this, SLOT(fontStyleChange(bool)));
    connect(lbtFontWeight, SIGNAL(clicked (bool)), this, SLOT(fontWeightChange(bool)));

    QToolBar * tbFont = mw->addToolBar(tr("Font"));
    tbFont->setObjectName("tbFont");
    grpFont->addAction(tbFont->addWidget(lcbFont));
    grpFont->addAction(tbFont->addWidget(lcbFontSize));
    grpFont->addAction(tbFont->addWidget(lbtFontWeight));   
    grpFont->addAction(tbFont->addWidget(lbtFontStyle));       
  }
}

QAction * ReportHandler::populateMenuBar(QMenuBar * menubar, QAction * exitAction)
{
  QMenu * mFile = menubar->addMenu(tr("&File"));
  mFile->addAction(fileNewAction);
  mFile->addAction(fileOpenAction);
  mFile->addAction(fileSaveAction);
  mFile->addAction(fileSaveAsAction);
  mFile->addAction(fileCloseAction);

  mFile->addSeparator();
  mFile->addAction(filePreviewAction);
  mFile->addAction(filePrintAction);
  mFile->addAction(filePrintToPDFAction);

  if(exitAction)
  {
    mFile->addSeparator();
    mFile->addAction(exitAction);
  }

  if(!_noDatabase)
  {
    QMenu * mDb = menubar->addMenu(tr("Data&base"));
    connect(mDb, SIGNAL(aboutToShow()), this, SLOT(dbMenuAboutToShow()));
    mDb->addAction(dbLoadAction);
    mDb->addAction(dbSaveAction);
    if(_allowDBConnect)
    {
      mDb->addSeparator();
      mDb->addAction(dbConnectAction);
    }
  }

  QMenu * mEdit = menubar->addMenu(tr("&Edit"));
  mEdit->addAction(editUndoAction);
  mEdit->addAction(editRedoAction);
  mEdit->addAction(editCutAction);
  mEdit->addAction(editCutAction);
  mEdit->addAction(editCopyAction);
  mEdit->addAction(editPasteAction);
  mEdit->addAction(editDeleteAction);
  mEdit->addSeparator();
  mEdit->addAction(editSelectAllAction);
  mEdit->addSeparator();
  mEdit->addAction(editZoomInAction);
  mEdit->addAction(editZoomOutAction);
  mEdit->addSeparator();
  mEdit->addAction(editPreferencesAction);

  QMenu * mInsert = menubar->addMenu(tr("&Insert"));
  mInsert->addAction(itemLabelAction);
  mInsert->addAction(itemFieldAction);
  mInsert->addAction(itemTextAction);
  mInsert->addAction(itemLineAction);
  mInsert->addAction(itemCrossTabAction);
  mInsert->addAction(itemRectAction);
  mInsert->addAction(itemBarcodeAction);
  mInsert->addAction(itemImageAction);
  mInsert->addAction(itemGraphAction);

  QMenu * mDoc = menubar->addMenu(tr("&Document"));
  mDoc->addAction(docTitleAction);
  mDoc->addAction(docPageSetupAction);
  mDoc->addAction(docQuerySourceListAction);
  mDoc->addAction(docSectionEditorAction);
  mDoc->addAction(docColorListAction);
  mDoc->addAction(docDefinedParamsAction);

  // Add Label Definitions Action
  mDoc->addAction(docLabelDefinitionsAction);

  QAction * sid = menubar->addSeparator();


  /*QMenu * mHelp =*/ menubar->addMenu(tr("&Help"));

  return sid;
}

void ReportHandler::setParentWindow(QWidget * pw)
{
  _parentWindow = pw;
  _mdiParentWindow = qobject_cast<QMdiArea *>(pw);
}


void ReportHandler::updateSelectedEntity()
{
  if(selectionCount() > 0)
  {
    QString message = QString::null;
    if(selectionCount() == 1)
    {
      DocumentWindow * gw = activeDocumentWindow();
      if(!gw)
        return;
      QGraphicsItem * item = gw->_scene->selectedItems().at(0);

      QRectF rect;
      QLineF line;
      double dx, dy, dw, dh;

      switch(item->type())
      {
        case ORGraphicsRectItem::Type:
        case ORGraphicsLabelItem::Type:
        case ORGraphicsFieldItem::Type:
        case ORGraphicsTextItem::Type:
        case ORGraphicsBarcodeItem::Type:
        case ORGraphicsImageItem::Type:
        case ORGraphicsGraphItem::Type:
        case ORGraphicsCrossTabItem::Type:
          rect = ((QGraphicsRectItem*)item)->rect();
          rect = QRectF(item->mapToParent(rect.topLeft()), rect.size());

          dx = (rect.x())/100.0;
          dy = (rect.y())/100.0;
          dw = (rect.width())/100.0;
          dh = (rect.height())/100.0;

          message = tr("Position [X: %1, Y: %2]    Size [W: %3, H: %4]")
                       .arg(QString::number(dx,'g',3))
                       .arg(QString::number(dy,'g',3))
                       .arg(QString::number(dw,'g',3))
                       .arg(QString::number(dh,'g',3));
          break;
        case ORGraphicsLineItem::Type:
          line = ((QGraphicsLineItem*)item)->line();
          line = QLineF(item->mapToParent(line.p1()), item->mapToParent(line.p2()));

          dx = ((line.p1().x()) / 100.0);
          dy = ((line.p1().y()) / 100.0);
          dw = ((line.p2().x()) / 100.0);
          dh = ((line.p2().y()) / 100.0);

          message = tr("Start Point [X: %1, Y: %2]    End Point [X: %3, Y: %4]")
                       .arg(QString::number(dx,'g',3))
                       .arg(QString::number(dy,'g',3))
                       .arg(QString::number(dw,'g',3))
                       .arg(QString::number(dh,'g',3));
          break;
        default:
          message = tr("Unknown Entity Type");
      }
    }
    else
      message = tr("Group Selection");
    emit messageChanged(message);
  }
  else
    emit messageCleared();
}

unsigned int ReportHandler::selectionCount()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    return gw->_scene->selectedItems().count();

  return 0;
}


//
// slot implementations
//
DocumentWindow* ReportHandler::fileNew()
{
  DocumentWindow* doc = createDocument(true);
//  doc->_scene->setPageOptions(doc->_scene->pageOptions());
  return doc;
}

DocumentWindow* ReportHandler::createDocument(bool newDoc)
{
  DocumentWindow * view = new DocumentWindow(newDoc, gridOptions, this, _parentWindow);
  connect(view->_scene, SIGNAL(selectionChanged()), this, SLOT(updateSelectedEntity()));

  if(_placeMenusOnWindows)
  {
    QMenuBar * mb = 0;
#ifdef Q_WS_MACX
    mb = new QMenuBar();
    view->setMenuBar(mb);
#else
    mb = view->menuBar();
#endif
    populateMenuBar(mb);
  }
  if(_placeToolbarsOnWindows)
    docToolBars(view);

  addDocumentWindow(view);
  view->showMaximized();
  return view;
}

void ReportHandler::fileOpen()
{
  // ok first we need to get a file to open.
  QString file = QFileDialog::getOpenFileName(
        0, tr("Open File"),
        QString::null, tr("XML (*.xml)") );
  fileOpen(file);
}

void ReportHandler::fileOpen(const QString &fileName)
{
  QString file = fileName;

  if(file.isNull())
    return; // user canceled

  // once we have a file lets go ahead and load it all up.
  QFile * f = new QFile(file);

  QDomDocument doc = QDomDocument();
  QString errMsg;
  int errLine, errCol;
  if(doc.setContent(f,&errMsg,&errLine,&errCol))
  {
    // ok lets read it
    DocumentWindow* rw = fileOpen(doc);
    if(rw)
      rw->_scene->filename = file;
  }
  else
  {
    // ERROR
    QMessageBox::critical(0, tr("Failed read on Open File"),
        QString().sprintf(tr("Encountered and error while parsing %s\n\n\t%s (Line %d Column %d)").toLatin1().constData(),file.toLatin1().data(),errMsg.toLatin1().data(),errLine,errCol));
  }
  delete f;
}


//
// File Actions
//
DocumentWindow * ReportHandler::fileOpen(const QDomDocument & doc, const QString & rptName, int rptGrade)
{
  DocumentWindow * rw = fileOpen(doc);
  if(rw) {
    // setup any variables to let this doc know how it was loaded
    // so when a save command is issued it knows what to do
    rw->_scene->lastSaveToDb = true;
    rw->_scene->dbRecordName = rptName;
    rw->_scene->dbRecordGrade = rptGrade;
  }
  return rw;
}

DocumentWindow * ReportHandler::fileOpen(const QDomDocument & doc)
{
    QDomElement root = doc.documentElement();
    if(root.tagName() != "report") {
        // arg we got an xml file but not one i know of
        qDebug("root element was not <report>");
        return 0;
    }

    DocumentWindow * dv = createDocument(false);
    DocumentScene * rw = dv->_scene;

    __dosnap = false;
    rw->loadDocument(root, _parentWindow);
    __dosnap = true;

    return dv;
}

void ReportHandler::fileSave()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
  {
    gw->_scene->checkValidity(gw);
    gw->_scene->save(gw);
  }
}

void ReportHandler::fileSaveAs()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
  {
    gw->_scene->checkValidity(gw);
    gw->_scene->saveAs(gw);
  }
}

void ReportHandler::fileClose()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->close();
}

//
// Edit Actions
//
void ReportHandler::editDelete()
{
    DocumentWindow * gw = activeDocumentWindow();
    if(gw)
    {
      gw->_scene->deleteSelected();
      return;
    }
}

void ReportHandler::editUndo()
{
    DocumentWindow * gw = activeDocumentWindow();
    bool ok = gw->_scene->undo();
    if(!ok) {
        QMessageBox::information(gw, tr("Undo"), tr("Nothing to undo"));
    }
}

void ReportHandler::editRedo()
{
    DocumentWindow * gw = activeDocumentWindow();
    if(gw) {        
        bool ok = gw->_scene->redo();
        if(!ok) {
            QMessageBox::information(gw, tr("Redo"), tr("Nothing to redo"));
        }
    }
}

void ReportHandler::editZoomIn()
{
    DocumentWindow * gw = activeDocumentWindow();
    if(gw) {        
        gw->_view->zoom(500);
    }
}

void ReportHandler::editZoomOut()
{
    DocumentWindow * gw = activeDocumentWindow();
    if(gw) {        
        gw->_view->zoom(-500);
    }
}

void ReportHandler::editCut()
{
  if(selectionCount() > 0) {
    editCopy();
    editDelete();
  }
}

void ReportHandler::editCopy() {
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
  {
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    QGraphicsItem * item = 0;
    if(list.count() > 0)
    {
      item = list.at(0);
      sectionData->copy_list.clear();
      if(item->type() == ORGraphicsLineItem::Type)
        sectionData->copy_pos = item->mapToScene(((ORGraphicsLineItem*)item)->line().p1());
      else
        sectionData->copy_pos = item->scenePos();
      ReportWriterCopyData cp;
      for(int i = 0; i < list.count(); i++)
      {
        item = list.at(i);
        int rtti = item->type();
        if(rtti == ORGraphicsLabelItem::Type)
        {
          ORGraphicsLabelItem * ent = (ORGraphicsLabelItem*)item;
          cp.copy_item = ReportWriterSectionData::LabelItem;
          cp.copy_str1 = ent->text();
          cp.copy_font = ent->font();
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_int1 = ent->textFlags();
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsFieldItem::Type)
        {
          ORGraphicsFieldItem * ent = (ORGraphicsFieldItem*)item;
          cp.copy_item = ReportWriterSectionData::FieldItem;
          cp.copy_str1 = ent->query();
          cp.copy_str2 = ent->column();
          cp.copy_str3 = ent->format();
          cp.copy_font = ent->font();
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_int1 = ent->textFlags();
          cp.copy_int2 = ent->lines();
          cp.copy_int3 = ent->columns();
          cp.copy_dbl1 = ent->xSpacing();
          cp.copy_dbl2 = ent->ySpacing();
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_bool1 = ent->trackTotal();
          cp.copy_bool2 = ent->trackBuiltinFormat();
          cp.copy_bool3 = ent->useSubTotal();
          cp.copy_bool4 = ent->triggerPageBreak();
          cp.copy_bool5 = ent->leftToRight();
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsTextItem::Type)
        {
          ORGraphicsTextItem * ent = (ORGraphicsTextItem*)item;
          cp.copy_item = ReportWriterSectionData::TextItem;
          cp.copy_str1 = ent->query();
          cp.copy_str2 = ent->column();
          cp.copy_font = ent->font();
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_int1 = ent->textFlags();
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_dbl1 = ent->bottomPadding();
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * ent = (ORGraphicsLineItem*)item;
          cp.copy_item = ReportWriterSectionData::LineItem;
          cp.copy_line_start = ent->mapToScene(ent->line().p1());
          cp.copy_line_end = ent->mapToScene(ent->line().p2());
          cp.copy_pen  = ent->pen();
           cp.copy_offset = cp.copy_line_start - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsRectItem::Type)
        {
          ORGraphicsRectItem * ent = (ORGraphicsRectItem*)item;
          cp.copy_item = ReportWriterSectionData::RectItem;
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsBarcodeItem::Type)
        {
          ORGraphicsBarcodeItem * ent = (ORGraphicsBarcodeItem*)item;
          cp.copy_item = ReportWriterSectionData::BarcodeItem;
          cp.copy_str1 = ent->query();
          cp.copy_str2 = ent->column();
          cp.copy_str3 = ent->format();
          cp.copy_int1 = ent->maxLength();
          cp.copy_int2 = ent->alignment();
          cp.copy_dbl1 = ent->narrowBarWidth();
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsImageItem::Type)
        {
          ORGraphicsImageItem * ent = (ORGraphicsImageItem*)item;
          cp.copy_item = ReportWriterSectionData::ImageItem;
          cp.copy_str1 = ent->query();
          cp.copy_str2 = ent->column();
          cp.copy_str3 = ent->inlineImageData();
          cp.copy_str4 = ent->mode();
          cp.copy_bool1 = ent->isInline();
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else if(rtti == ORGraphicsGraphItem::Type)
        {
          ORGraphicsGraphItem * ent = (ORGraphicsGraphItem*)item;
          cp.copy_item = ReportWriterSectionData::GraphItem;
          cp.copy_rect = QRectF(ent->mapToScene(ent->rect().topLeft()), ent->rect().size());
          cp.copy_pen  = ent->pen();
          cp.copy_border  = ent->border();
          cp.copy_brush = ent->brush();
          ent->copyData(cp.copy_graph);
          cp.copy_offset = cp.copy_rect.topLeft() - sectionData->copy_pos;
          sectionData->copy_list.append(cp);
        }
        else
        {
          qDebug("Tried to copy an item I know nothing about!");
        }
      }
    }
  }
}

void ReportHandler::editPaste()
{
  // call the editPaste function passing it a point
  //  that make sense as defaults (same canvas / slightly offset pos of orig copy)
  QPointF p;
  p = sectionData->copy_pos + QPointF(7.0, 7.0);
  editPaste(p);
}


void ReportHandler::editPaste(const QPointF & pos)
{
  DocumentWindow * gw = activeDocumentWindow();
  if(!gw)
    return;

  // paste a new item of the copy we have in the specified location
  if(sectionData->copy_list.count() < 1)
    return;

  QGraphicsItem * section = 0;
  QList<QGraphicsItem*> list = gw->_scene->items(pos);
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
    QMessageBox::critical(gw, tr("No Section Found"), tr("Items must be pasted into a section."));
    return;
  }

  QGraphicsItem * pasted_ent = 0;
  gw->_scene->clearSelection();
  sectionData->mouseAction = ReportWriterSectionData::MA_None;
  ReportWriterCopyData cp;
  for(int i = 0; i < sectionData->copy_list.count(); i++)
  {
    pasted_ent = 0;
    cp = sectionData->copy_list[i];
    if(cp.copy_item == ReportWriterSectionData::LabelItem)
    {
      ORGraphicsLabelItem * ent = new ORGraphicsLabelItem(section);
      ent->setText(cp.copy_str1);
      ent->setFont(cp.copy_font);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setTextFlags(cp.copy_int1);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::FieldItem)
    {
      ORGraphicsFieldItem * ent = new ORGraphicsFieldItem(section);
      ent->setQuery(cp.copy_str1);
      ent->setColumn(cp.copy_str2);
      ent->setFont(cp.copy_font);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setTextFlags(cp.copy_int1);
      ent->setArray(cp.copy_int2, cp.copy_int3, cp.copy_dbl1, cp.copy_dbl2, cp.copy_bool4, cp.copy_bool5);
      ent->setTrackTotal(cp.copy_bool1);
      ent->setFormat(cp.copy_str3, cp.copy_bool2);
      ent->setUseSubTotal(cp.copy_bool3);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::TextItem)
    {
      ORGraphicsTextItem * ent = new ORGraphicsTextItem(section);
      ent->setQuery(cp.copy_str1);
      ent->setColumn(cp.copy_str2);
      ent->setFont(cp.copy_font);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setTextFlags(cp.copy_int1);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      ent->setBottomPadding(cp.copy_dbl1);
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::LineItem)
    {
      ORGraphicsLineItem * ent = new ORGraphicsLineItem(section);
      ent->setPen(cp.copy_pen);
      QLineF line(section->mapFromScene(cp.copy_line_start + cp.copy_offset), section->mapFromScene(cp.copy_line_end + cp.copy_offset));
      ent->setLine(line);
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::RectItem)
    {
      ORGraphicsRectItem * ent = new ORGraphicsRectItem(section);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::BarcodeItem)
    {
      ORGraphicsBarcodeItem * ent = new ORGraphicsBarcodeItem(section);
      ent->setQuery(cp.copy_str1);
      ent->setColumn(cp.copy_str2);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setFormat(cp.copy_str3);
      ent->setMaxLength(cp.copy_int1);
      ent->setAlignment(cp.copy_int2);
      ent->setNarrowBarWidth(cp.copy_dbl1);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::ImageItem)
    {
      ORGraphicsImageItem * ent = new ORGraphicsImageItem(section);
      ent->setQuery(cp.copy_str1);
      ent->setColumn(cp.copy_str2);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setInlineImageData(cp.copy_str3);
      ent->setMode(cp.copy_str4);
      ent->setInline(cp.copy_bool1);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else if(cp.copy_item == ReportWriterSectionData::GraphItem)
    {
      ORGraphicsGraphItem * ent = new ORGraphicsGraphItem(section);
      ent->setData(cp.copy_graph);
      ent->setPen(cp.copy_pen);
      ent->setBorder(cp.copy_border);
      ent->setBrush(cp.copy_brush);
      ent->setPos(section->mapFromScene(pos + cp.copy_offset));
      ent->setRect(0, 0, cp.copy_rect.width(),cp.copy_rect.height());
      pasted_ent = ent;
    }
    else 
    {
      qDebug("tried to paste an item I don't understand.");
    }

    if(pasted_ent)
    {
      pasted_ent->setSelected(true);
      sectionData->mouseAction = ReportWriterSectionData::MA_Grab;
      gw->_scene->setModified(true);
    }
  }
}

void ReportHandler::editPreferences()
{
  EditPreferences * dlgPref = new EditPreferences(0);
  if(dlgPref)
  {
    dlgPref->setDefaultFont(QFont(ORGraphicsRectItem::getDefaultEntityFont()));
    dlgPref->setShowGrid(gridOptions->isVisible());
    dlgPref->setSnapGrid(gridOptions->isSnap());
    dlgPref->setGridSize(gridOptions->xInterval() ,gridOptions->yInterval());
    if(dlgPref->exec() == QDialog::Accepted)
    {
      ORGraphicsRectItem::setDefaultEntityFont(dlgPref->defaultFont());
      gridShowAction->setChecked(dlgPref->showGrid());
      gridSnapAction->setChecked(dlgPref->snapGrid());
      gridOptions->setXInterval(dlgPref->gridSizeX());
      gridOptions->setYInterval(dlgPref->gridSizeY());
      QString newLanguage = dlgPref->selectedLanguage();
      if(newLanguage != OpenRPT::languages.selectedTitle())
      {
        QMessageBox::information(dlgPref, QString(tr("Language: %1").arg(newLanguage)), tr("The language change will take effect the next time the report writer will be run."));
        OpenRPT::languages.selectFromTitle(newLanguage);
      }
    }
    delete dlgPref;
  }
}

void ReportHandler::editSelectAll()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
  {
    QList<QGraphicsItem*> list = gw->_scene->items();
    for(int i = 0; i < list.count(); i++)
    {
        list.at(i)->setSelected(true);
    }
  }
}

void ReportHandler::editProperties()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
  {
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    for(int i = 0; i < list.count(); i++)
    {
      switch(list.at(i)->type())
      {
        case ORGraphicsLineItem::Type:
          (static_cast<ORGraphicsLineItem*>(list.at(i)))->properties(gw);
          break;
        case ORGraphicsRectItem::Type:
        case ORGraphicsLabelItem::Type:
        case ORGraphicsFieldItem::Type:
        case ORGraphicsTextItem::Type:
        case ORGraphicsBarcodeItem::Type:
        case ORGraphicsImageItem::Type:
        case ORGraphicsGraphItem::Type:
          (static_cast<ORGraphicsRectItem*>(list.at(i)))->properties(gw);
          break;
      };
    }
  }  
}


// Doc Actions
//
void ReportHandler::docTitle()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->docInfoEditor(gw);
}

void ReportHandler::docPageSetup()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->docPageSetup(gw);
}

void ReportHandler::docQuerySourceList()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->querySourceList();
}

void ReportHandler::docSectionEditor()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->sectionEditor(gw);
}

void ReportHandler::docColorList()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->colorList(gw);
}

void ReportHandler::docDefinedParams()
{
  DocumentWindow * gw = activeDocumentWindow();
  if(gw)
    gw->_scene->editDefinedParameters(gw);
}

// Label Definitions Action
void ReportHandler::docLabelDefinitions()
{
  DocumentWindow * gw = activeDocumentWindow();
  if (LabelSizeInfo::areLabelsEditable())
  {
    if(gw)
      gw->_scene->editLabelDefinitions(gw);
  }
  else
  {
    QMessageBox::warning(gw, tr("Warning"), tr("You must connect to a database with a 'labeldef' table to edit label definitions."));
  }
}

//
// Item Actions
//
void ReportHandler::itemLabel() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::LabelItem;
}

void ReportHandler::itemField() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::FieldItem;
}

void ReportHandler::itemText() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::TextItem;
}

void ReportHandler::itemLine() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::LineItem;
}

void ReportHandler::itemCrossTab() {                                                                                         
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;                                                           
    sectionData->insertItem = ReportWriterSectionData::CrossTabItem;                                                         
}

void ReportHandler::itemRect() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::RectItem;
}

void ReportHandler::itemBarcode() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::BarcodeItem;
}

void ReportHandler::itemImage() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::ImageItem;
}

void ReportHandler::itemGraph() {
    sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    sectionData->insertItem = ReportWriterSectionData::GraphItem;
}

// TODO: add in support for a pointer/grabber/none option
//       and make the tool items toggled

//
// Db Actions
//
void ReportHandler::dbConnect() {
    if(!_allowDBConnect) return;

    bool dbConnected = (dbConnectAction->text() == _strMenuDisconnect);

    if(dbConnected) {
        // disconnect
        QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
        if(db.isValid()) db.close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    } else {
        // connect
        ParameterList params;
        params.append("name", name());
        params.append("copyright",   OpenRPT::copyright);
        params.append("version",     OpenRPT::version);
        params.append("build",       OpenRPT::build);
        params.append("databaseURL", OpenRPT::databaseURL);

        login newdlg(0, "", true);
        newdlg.set(params);

        if(newdlg.exec() == QDialog::Accepted) {
          OpenRPT::databaseURL = newdlg._databaseURL;

  // The following is code that works specifically with the xTuple ERP database
  // This should be expanded to be usefull when not connecting to an xTuple ERP
  // database as well. Command line option maybe?
  XSqlQuery langq("SELECT * "
                  "FROM usr, locale LEFT OUTER JOIN"
                  "     lang ON (locale_lang_id=lang_id) LEFT OUTER JOIN"
                  "     country ON (locale_country_id=country_id) "
                  "WHERE ( (usr_username=CURRENT_USER)"
                  " AND (usr_locale_id=locale_id) );" );
  if (langq.first())
  {
    QStringList paths;
    paths << "dict";
    paths << "";
    paths << "../dict";
    paths << qApp->applicationDirPath() + "/dict";
    paths << qApp->applicationDirPath();
    paths << qApp->applicationDirPath() + "/../dict";
#if defined Q_WS_MACX
    paths << qApp->applicationDirPath() + "/../../../dict";
    paths << qApp->applicationDirPath() + "/../../..";
#endif

    QStringList files;
    if (!langq.value("locale_lang_file").toString().isEmpty())
      files << langq.value("locale_lang_file").toString();

    QString langext;
    if (!langq.value("lang_abbr2").toString().isEmpty() &&
        !langq.value("country_abbr").toString().isEmpty())
    {
      langext = langq.value("lang_abbr2").toString() + "_" +
                langq.value("country_abbr").toString().toLower();
    }
    else if (!langq.value("lang_abbr2").toString().isEmpty())
    {
      langext = langq.value("lang_abbr2").toString();
    }

    if(!langext.isEmpty())
    {
      files << "reports." + langext;

      XSqlQuery pkglist("SELECT pkghead_name FROM pkghead WHERE packageIsEnabled(pkghead_name);");
      while(pkglist.next())
      {
        files << pkglist.value("pkghead_name").toString() + "." + langext;
      }
    }

    if (files.size() > 0)
    {
      QTranslator *translator = new QTranslator(qApp);
      for (QStringList::Iterator fit = files.begin(); fit != files.end(); ++fit)
      {
        for(QStringList::Iterator pit = paths.begin(); pit != paths.end(); ++pit)
        {
          qDebug("looking for %s in %s", (*fit).toLatin1().data(), (*pit).toLatin1().data());
          if (translator->load(*fit, *pit))
          {
            qApp->installTranslator(translator);
            qDebug("installed %s/%s", (*pit).toLatin1().data(), (*fit).toLatin1().data());
            translator = new QTranslator(qApp);
            break;
          }
        }
      }
    }
  }
  // END language loading code

        }
    }

    emit dbOpenClosed();
}

void ReportHandler::dbLoadDoc() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
    if(db.isValid()) {
        DBFileDialog rptDiag;
        rptDiag.setWindowTitle(tr("Load Report from Database"));
        rptDiag._name->setReadOnly(true);
        rptDiag._grade->setEnabled(false);
        if(rptDiag.exec() == QDialog::Accepted) {
            QDomDocument doc;
            QString errMsg;
            int errLine, errCol;
            if(doc.setContent(rptDiag.getSource(),&errMsg,&errLine,&errCol)) {
                DocumentWindow * rw = fileOpen(doc);
                if(rw)
                {
                  // setup any variables to let this doc know how it was loaded
                  // so when a save command is issued it knows what to do
                  rw->_scene->lastSaveToDb = true;
                  rw->_scene->dbRecordName = rptDiag.getNameById();
                  rw->_scene->dbRecordGrade = rptDiag.getGradeById();
                }
            } else {
                QMessageBox::warning(0, tr("Error Loading Report"),
                    QString().sprintf(tr("ReportWriterWindow::dbLoadDoc() : ERROR on setContent()\n\t%s (Line %d Column %d)").toLatin1().constData(),errMsg.toLatin1().data(),errLine,errCol));
            }
        }
    } else {
        QMessageBox::warning(0, tr("No Database Connection"),
            tr("There is no database connection that can be used to load a document."));
    }
}

void ReportHandler::dbSaveDoc()
{
  QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
  if(db.isValid())
  {
    DocumentWindow * gw = activeDocumentWindow();
    if(gw)
    {
      gw->_scene->checkValidity(gw);
      gw->_scene->saveToDb(gw);
    }
  }
  else
  {
    QMessageBox::warning(0, tr("No Database Connection"),
        tr("There is no database connection that can be used to save this document."));
  }
}


//
// ReportWindow tracking methods
//
DocumentWindow * ReportHandler::activeDocumentWindow()
{
  if(_mdiParentWindow)
  {
    // returns the topmost window, even if not active
    QList<QMdiSubWindow *> list = _mdiParentWindow->subWindowList(QMdiArea::StackingOrder);
    return list.isEmpty() ? 0 : qobject_cast<DocumentWindow*>(list.last()->widget());
  }
  else
  {
    for(int it = 0; it < gwList.count(); it++)
    {
      DocumentWindow * gw = gwList.at(it);
      if(gw && QApplication::activeWindow() == gw)
        return gw;
    }
  }
  return 0;
}

void ReportHandler::addDocumentWindow(DocumentWindow * gw)
{
  if(gw) {
    gwList.append(gw);
    connect(gw, SIGNAL(destroyed(QObject*)), this, SLOT(removeReportWindow(QObject*)));
    if(_mdiParentWindow)
      _mdiParentWindow->addSubWindow(gw);
  }
}

void ReportHandler::removeReportWindow(QObject * obj)
{
  DocumentWindow * gw = static_cast<DocumentWindow *>(obj);
  if(gw)
    gwList.removeAll(gw);
}

//
// dbMenuAboutToShow
//
void ReportHandler::dbMenuAboutToShow() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
    if(db.isValid() && db.isOpen() && _databaseElt.isNull()) {
        dbLoadAction->setEnabled(true);
        dbSaveAction->setEnabled(true);
        dbConnectAction->setText(_strMenuDisconnect);
    } else {
        dbLoadAction->setEnabled(false);
        dbSaveAction->setEnabled(false);
        dbConnectAction->setText(_strMenuConnect);
    }
}

void ReportHandler::filePreview()
{
  print(true);
}

void ReportHandler::filePrint()
{
  print(false);
}

void ReportHandler::print(bool showPreview)
{
  int rw = 0;
  DocumentWindow * gw = activeDocumentWindow();
  if(!rw && !gw) {
    return;
  }

  if(gw)
    gw->_scene->checkValidity(gw);

  QMainWindow * mw = 0;
  if(gw)
    mw = gw;

  QDomDocument ddoc;
  if(gw)
    ddoc = gw->_scene->document();

  ORPreRender pre;
  pre.setDom(ddoc);

  ParameterEdit paramEdit(0);
  QDialog *dlgEdit = ParameterEdit::ParameterEditDialog(&paramEdit, mw);
  if (paramEdit.setDocument(ddoc))
    if (dlgEdit->exec() != QDialog::Accepted)
      return;

  pre.setParamList(paramEdit.getParameterList());
  ORODocument * doc = pre.generate();

  if(doc)
  {
    ReportPrinter printer(QPrinter::HighResolution);

    ORPrintRender render;
    render.setupPrinter(doc, &printer);

    if(showPreview) 
    {
      if(printer.printerName().isEmpty()) { // no default printer
        QPrintDialog pd(&printer);
        if(pd.exec() != QDialog::Accepted)
        {
          return; // no printer, can't preview
        }
      }

      PreviewDialog preview (doc, &printer, mw);
      if (preview.exec() == QDialog::Rejected) 
        return;
    }

    QPrintDialog pd(&printer);
    pd.setMinMax(1, doc->pages());
    if(pd.exec() == QDialog::Accepted)
    {
      render.render(doc, &printer);
    }
    delete doc;
  }
}

void ReportHandler::filePrintToPDF()
{
  int rw = 0;
  DocumentWindow * gw = activeDocumentWindow();
  if(!rw && !gw)
    return;
  QMainWindow * mw = 0;
  if(gw)
    mw = gw;
  QString outfile = QFileDialog::getSaveFileName( mw, tr("Choose filename to save"), tr("print.pdf"), tr("Pdf (*.pdf)") );

  if(outfile.isEmpty()) // User canceled save dialog
    return;

  if(gw)
    filePrintToPDF(gw, gw->_scene->document(), outfile);
}


void ReportHandler::filePrintToPDF(QWidget *wnd, const QDomDocument & doc, QString & pdfFileName)
{
  if(pdfFileName.isEmpty()) 
    return;

  if ( QFileInfo( pdfFileName ).suffix().isEmpty() )
    pdfFileName.append(".pdf");

  ParameterEdit paramEdit(0);
  QDialog *dlgEdit = ParameterEdit::ParameterEditDialog(&paramEdit, wnd);
  if (paramEdit.setDocument(doc) && dlgEdit->exec() != QDialog::Accepted)
      return;

  ORPreRender pre;
  pre.setDom(doc);
  pre.setParamList(paramEdit.getParameterList());
  ORODocument *odoc = pre.generate();
  if (odoc) {
      ORPrintRender::exportToPDF(odoc, pdfFileName );
      delete odoc;
  }
}

void ReportHandler::sReportsChanged(int pReportid, bool pLocaleUpdate)
{
  emit reportsChanged(pReportid, pLocaleUpdate);
}

void ReportHandler::onWinChanged(QMdiSubWindow *w)
{
  bool show = w ? true : false;
  dbSaveAction->setEnabled(show & dbLoadAction->isEnabled());
  grpEdit->setEnabled(show);
  grpItem->setEnabled(show);
  grpGrid->setEnabled(show);
  grpDoc->setEnabled(show);
  grpFont->setEnabled(show);
}

QString ReportHandler::name()
{
#if defined Q_WS_WIN
  return QObject::tr("OpenRPT Report Writer for Windows");
#elif defined Q_WS_X11
  return QObject::tr("OpenRPT Report Writer for Linux");
#elif defined Q_WS_MAC
  return QObject::tr("OpenRPT Report Writer for OS X");
#else
  return QObject::tr("OpenRPT Report Writer");
#endif
}

QString ReportHandler::copyright()
{
  return OpenRPT::copyright;
}

QString ReportHandler::version()
{
  return OpenRPT::version;
}

int ReportHandler::insertItemCode() const
{
  return sectionData->insertItem;
}

void ReportHandler::resetInsertItemCode()
{
  sectionData->insertItem = ReportWriterSectionData::NoItem;

  foreach(QAction *action, grpItem->actions())
      action->setChecked(false);
}

void ReportHandler::createSectionActions()
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw)
        return;

    if(selectionCount()==0)
        return;

    // delete previous actions
    while (!m_SectionActions.isEmpty())
    {
        delete m_SectionActions.takeFirst();
    }

    QAction *selectedSection = NULL;
    bool differentSectionSelected = false;
    QList<ORGraphicsSectionItem *> sections = gw->_scene->sectionsList();

    foreach(ORGraphicsSectionItem *section, sections)
    {
        if(!section->isVisible())
            continue;

        QAction *sectionAction = new QAction(section->title(),this);
        sectionAction->setCheckable(true);
        QList<QGraphicsItem*> lstSel = gw->_scene->selectedItems();
        foreach(QGraphicsItem *item, lstSel)
        {
            if(item->parentItem() == section)
            {
                if(selectedSection && selectedSection != sectionAction)
                    differentSectionSelected = true;

                selectedSection = sectionAction;
                break;
            }
        }
        connect(sectionAction, SIGNAL(triggered(bool)), this, SLOT(sectionToggled(bool)));
        m_SectionActions.prepend(sectionAction);
    }

    if(selectedSection && !differentSectionSelected)
    {
        // only check the action if all the items selected belong to the same section
        selectedSection->setChecked(true);
    }
}

void ReportHandler::buildItemContextMenu(QMenu * menu)
{
  if(selectionCount() > 0)
  {
    menu->addAction(editCutAction);
    menu->addAction(editCopyAction);
  }
  if(sectionData->copy_list.count() > 0)
  {
    menu->addAction(editPasteAction);
  }
  if(selectionCount() == 1)
  {
    menu->addSeparator();
    menu->addAction(editPropertiesAction);
  }
  if(selectionCount() > 1)
  {
    menu->addSeparator();
    menu->addAction(alignTopAction);
    menu->addAction(alignVCenterAction);
    menu->addAction(alignBottomAction);
    menu->addAction(alignLeftAction);
    menu->addAction(alignHCenterAction);
    menu->addAction(alignRightAction);
  }
  if(selectionCount() > 2)
  {
    menu->addAction(evenHSpacingAction);
    menu->addAction(evenVSpacingAction);
  }
  if(selectionCount() >= 1)
  {
    menu->addSeparator();
    menu->addAction(colorAction);
    menu->addAction(fillAction);
    menu->addAction(borderAction);
    menu->addAction(rotationAction);

    createSectionActions();
    QMenu *sectionsMenu = menu->addMenu(tr("Section"));
    sectionsMenu->addActions(m_SectionActions);
  } 
}

QPointF ReportHandler::getCopyPoint() const
{
  return sectionData->copy_pos;
}

void ReportHandler::setCopyPoint(const QPointF & p)
{
  sectionData->copy_pos = p;
}

//////////////////////////////////////////////////////////////////////////////                                               
// Description                                                                                                               
//   Get icon name using the options for iconsize
//////////////////////////////////////////////////////////////////////////////
QString ReportHandler::getIconName(const QString& pDefaultName)
{
  QString defaultName = pDefaultName;
  QRegExp rx("_size");      // match ampersands but not &amp;

  // TODO: Get toolbar iconsize from options
  unsigned int iconSize(32);
  switch (iconSize)
  {
    case (16)  : defaultName.replace(rx, "_16x16");
                 break;
    case (24)  : defaultName.replace(rx, "_24x24");
                 break;
    case (32)  : defaultName.replace(rx, "_32x32");
                 break;
    case (48)  : defaultName.replace(rx, "_48x48");
                 break;
    case (64)  : defaultName.replace(rx, "_64x64");
                 break;
    default    : defaultName.replace(rx, "_32x32");
                 break;
  }
  return defaultName;
}


void ReportHandler::alignTop()
{
  qreal y = -1;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      y = list.at(0)->sceneBoundingRect().top();
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          y = qMin(y, list.at(i)->sceneBoundingRect().top());
        else
          y = qMin(y, list.at(i)->scenePos().y());
      }
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = y - li->sceneBoundingRect().top();
          QLineF l = li->line();
          l.translate(0, d);
          li->setLine(l);
        }
        else
        {
          QGraphicsItem * item = list.at(i);
          QPointF p(item->scenePos().x(), y);
          item->setPos(item->mapToParent(item->mapFromScene(p)));
        }
      }
    }
    __dosnap = true;
  }
}

void ReportHandler::alignVCenter()
{
  qreal y = 0;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          y += list.at(i)->sceneBoundingRect().center().y();
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
            y += ri->scenePos().y() + (ri->rect().height() / 2.0);
          else
            y += list.at(i)->sceneBoundingRect().center().y();
        }
      }
      y = y / (qreal)i;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = y - li->sceneBoundingRect().center().y();
          QLineF l = li->line();
          l.translate(0, d);
          li->setLine(l);
        }
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
          {
            QPointF p(ri->scenePos().x(), y - (ri->rect().height() / 2.0));
            ri->setPos(ri->mapToParent(ri->mapFromScene(p)));
          }
          else
          {
            QGraphicsItem * item = list.at(i);
            QPointF p(item->scenePos().x(), y - (item->sceneBoundingRect().height() / 2.0));
            item->setPos(item->mapToParent(item->mapFromScene(p)));
          }
        }
      }
    }
    __dosnap = true;
  }
}

void ReportHandler::alignBottom()
{
  qreal y = -1;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      y = list.at(0)->sceneBoundingRect().bottom();
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          y = qMax(y, list.at(i)->sceneBoundingRect().bottom());
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
            y = qMax(y, ri->scenePos().y() + ri->rect().height());
          else
            y = qMax(y, list.at(i)->sceneBoundingRect().bottom());
        }
      }
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = y - li->sceneBoundingRect().bottom();
          QLineF l = li->line();
          l.translate(0, d);
          li->setLine(l);
        }
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
          {
            QPointF p(ri->scenePos().x(), y - ri->rect().height());
            ri->setPos(ri->mapToParent(ri->mapFromScene(p)));
          }
          else
          {
            QGraphicsItem * item = list.at(i);
            QPointF p(item->scenePos().x(), y - item->sceneBoundingRect().height());
            item->setPos(item->mapToParent(item->mapFromScene(p)));
          }
        }
      }
    }
    __dosnap = true;
  }
}

void ReportHandler::alignLeft()
{
  qreal x = -1;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      x = list.at(0)->sceneBoundingRect().left();
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          x = qMin(x, list.at(i)->sceneBoundingRect().left());
        else
          x = qMin(x, list.at(i)->scenePos().x());
      }
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = x - li->sceneBoundingRect().left();
          QLineF l = li->line();
          l.translate(d, 0);
          li->setLine(l);
        }
        else
        {
          QGraphicsItem * item = list.at(i);
          QPointF p(x, item->scenePos().y());
          item->setPos(item->mapToParent(item->mapFromScene(p)));
        }
      }
    }
    __dosnap = true;
  }
}

void ReportHandler::alignHCenter()
{
  qreal x = 0;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          x += list.at(i)->sceneBoundingRect().center().x();
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
            x += ri->scenePos().x() + (ri->rect().width() / 2.0);
          else
            x += list.at(i)->sceneBoundingRect().center().x();
        }
      }
      x = x / (qreal)i;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = x - li->sceneBoundingRect().center().x();
          QLineF l = li->line();
          l.translate(d, 0);
          li->setLine(l);
        }
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
          {
            QPointF p(x - (ri->rect().width() / 2.0), ri->scenePos().y());
            ri->setPos(ri->mapToParent(ri->mapFromScene(p)));
          }
          else
          {
            QGraphicsItem * item = list.at(i);
            QPointF p(x - (item->sceneBoundingRect().width() / 2.0), item->scenePos().y());
            item->setPos(item->mapToParent(item->mapFromScene(p)));
          }
        }
      }
    }
    __dosnap = true;
  }
}

void ReportHandler::alignRight()
{
  qreal x = -1;
  DocumentWindow *gw = activeDocumentWindow();
  if(gw)
  {
    __dosnap = false;
    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() > 1)
    {
      x = list.at(0)->sceneBoundingRect().right();
      int i = 0;
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
          x = qMax(x, list.at(i)->sceneBoundingRect().right());
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
            x = qMax(x, ri->scenePos().x() + ri->rect().width());
          else
            x = qMax(x, list.at(i)->sceneBoundingRect().right());
        }
      }
      for(i = 0; i < list.count(); i++)
      {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          ORGraphicsLineItem * li = static_cast<ORGraphicsLineItem*>(list.at(i));
          qreal d = x - li->sceneBoundingRect().right();
          QLineF l = li->line();
          l.translate(d, 0);
          li->setLine(l);
        }
        else
        {
          QGraphicsRectItem * ri = static_cast<QGraphicsRectItem*>(list.at(i));
          if(ri)
          {
            QPointF p(x - ri->rect().width(), ri->scenePos().y());
            ri->setPos(ri->mapToParent(ri->mapFromScene(p)));
          }
          else
          {
            QGraphicsItem * item = list.at(i);
            QPointF p(x - item->sceneBoundingRect().width(), item->scenePos().y());
            item->setPos(item->mapToParent(item->mapFromScene(p)));
          }
        }
      }
    }
    __dosnap = true;
  }
}


void ReportHandler::evenHSpacing()
{
  evenSpacing(Qt::XAxis);
}

void ReportHandler::evenVSpacing()
{
  evenSpacing(Qt::YAxis);
}

void ReportHandler::evenSpacing(Qt::Axis axis)
{
  DocumentWindow *gw = activeDocumentWindow();
  if(!gw)
    return;

  __dosnap = false;

  QMap< qreal, QGraphicsItem* > sortedItems;
  foreach(QGraphicsItem* item, gw->_scene->selectedItems())
  {
    qreal refPosition = (axis == Qt::XAxis ? item->sceneBoundingRect().right() : item->sceneBoundingRect().bottom());
    sortedItems.insert(refPosition, item);
  }

  // We have to sort the items in ascending pos values
  QList<qreal> refPositions = sortedItems.keys();
  QList<QGraphicsItem*> values = sortedItems.values();
  qreal offset = 0;
  qreal firstSpacing = 0;

  for(int i = 1; i < sortedItems.count(); i++)
  {
    QGraphicsItem* item = values.at(i);
    qreal itemPos = (axis == Qt::XAxis ? item->sceneBoundingRect().left() : item->sceneBoundingRect().top());
    qreal spacing = itemPos - refPositions.at(i-1);
    if(i==1)
    { // the spacing between all items will be egal to the gap between the first two items
      firstSpacing = spacing;
    }

    qreal shift = firstSpacing - spacing;

    if(axis == Qt::XAxis)
      item->setPos(item->x() + shift + offset, item->y());
    else
      item->setPos(item->x(), item->y() + shift + offset);

    offset += shift;
  }

  __dosnap = true;
}


void ReportHandler::color()
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() == 0) 
        return; // no selected items

    QColor selectedColor;

    for(int i = 0; i < list.count(); i++)
    {
        if(list.at(i)->type() == ORGraphicsLineItem::Type)
        {
          QGraphicsLineItem * l = static_cast<QGraphicsLineItem*>(list.at(i));
          if(l) {
            QPen p = l->pen();
            if(i==0) {
              selectedColor = QColorDialog::getColor (p.color(), gw);
              if(!selectedColor.isValid())
                break;
            }
            p.setColor(selectedColor);
            l->setPen(p);
          }
        }
        else
        {
          QAbstractGraphicsShapeItem * gs = static_cast<QAbstractGraphicsShapeItem*>(list.at(i));
          if(gs)
          {
            QPen p = gs->pen();
            if(i==0) {
              selectedColor = QColorDialog::getColor (p.color(), gw);
              if(!selectedColor.isValid())
                break;
            }
            p.setColor(selectedColor);
            gs->setPen(p);
          }
        }
    }
}


void ReportHandler::fill()
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() == 0) 
        return; // no selected items

    QColor selectedColor;

    for(int i = 0; i < list.count(); i++)
    {
        QAbstractGraphicsShapeItem * item = static_cast<QAbstractGraphicsShapeItem*>(list.at(i));
        if(list.at(i)->type() != ORGraphicsLineItem::Type && item) {
            if(i==0)
            {
                selectedColor = QColorDialog::getColor (item->brush().color(), gw);
                if(!selectedColor.isValid())
                    break;
            }
            QBrush brush = item->brush();
            brush.setStyle(Qt::SolidPattern);
            brush.setColor(selectedColor);
            item->setBrush(brush);
        }
    }
}


void ReportHandler::border()
{
  DocumentWindow *gw = activeDocumentWindow();
  if(!gw)
    return;

  QList<QGraphicsItem*> list = gw->_scene->selectedItems();
  QPen newBorder, previousBorder;

  for(int i = 0; i < list.count(); i++)
  {
    if(list.at(i)->type() == ORGraphicsLineItem::Type)
    {
      ORGraphicsLineItem * l = static_cast<ORGraphicsLineItem*>(list.at(i));
      if(i==0)
      {
        previousBorder = l->pen();
        l->properties(gw);
        newBorder = l->pen();
      }
      else
      {
        QPen pen = l->pen();
        if(newBorder.color() != previousBorder.color()) pen.setColor(newBorder.color());
        if(newBorder.width() != previousBorder.width()) pen.setWidth(newBorder.width());
        if(newBorder.style() != previousBorder.style()) pen.setStyle(newBorder.style());
        l->setPen(pen);
      }
    }
    else
    {
      ORGraphicsRectItem * r = static_cast<ORGraphicsRectItem*>(list.at(i));
      if(i==0)
      {
        previousBorder = r->border();
        r->borderProperties(gw);
        newBorder = r->border();
      }
      else
      {
        QPen pen = r->border();
        if(newBorder.color() != previousBorder.color()) pen.setColor(newBorder.color());
        if(newBorder.width() != previousBorder.width()) pen.setWidth(newBorder.width());
        if(newBorder.style() != previousBorder.style()) pen.setStyle(newBorder.style());
        r->setBorder(pen);
      }
    }
  }
}


void ReportHandler::rotation()
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();
    if(list.count() == 0) 
        return; // no selected items

    bool ok = false;
    qreal angle = 0;

    for(int i = 0; i < list.count(); i++)
    {
        if(list.at(i)->type() != ORGraphicsLineItem::Type)
        {
            ORGraphicsRectItem * r = static_cast<ORGraphicsRectItem*>(list.at(i));
            if(r) 
            {
                if(!ok) 
                {
                    angle = QInputDialog::getDouble(gw, tr("Rotation"),
                        tr("Angle (0-360) :"), r->rotation(), 0, 360, 1, &ok);

                    if(!ok) 
                        return;
                }

                r->setRotation(angle); //temp
            }
        }
    }
}


void ReportHandler::fontChange(const QString &text)
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();

    for(int i = 0; i < list.count(); i++)
    {
        setItemFontFamily(list.at(i), text);
    }
}

void ReportHandler::fontSizeChange(const QString &text)
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();

    for(int i = 0; i < list.count(); i++)
    {
        setItemFontSize(list.at(i), text);
    }
}

void ReportHandler::fontWeightChange(bool checked)
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();

    for(int i = 0; i < list.count(); i++)
    {
        setItemFontWeight(list.at(i), checked);
    }
}

void ReportHandler::fontStyleChange(bool checked)
{
    DocumentWindow *gw = activeDocumentWindow();
    if(!gw) 
        return;

    QList<QGraphicsItem*> list = gw->_scene->selectedItems();

    for(int i = 0; i < list.count(); i++)
    {
        setItemFontStyle(list.at(i), checked);
    }
}


void ReportHandler::setFontFamily(const QString &text)
{
  QObject * w = parent();
  if(_placeToolbarsOnWindows)
    w = activeDocumentWindow();
  if(!w)
    return;
  QFontComboBox * o = w->findChild<QFontComboBox*>("font_family");
  
  if(o)
    o->setEditText(text);
}

void ReportHandler::setFontSize(const QString &text)
{
  QObject * w = parent();
  if(_placeToolbarsOnWindows)
    w = activeDocumentWindow();
  if(!w)
    return;
  QComboBox * o = w->findChild<QComboBox*>("font_size");
  
  if(o)
    o->setEditText(text);
}

void ReportHandler::setFontWeight(bool v)
{
  QObject * w = parent();
  if(_placeToolbarsOnWindows)
    w = activeDocumentWindow();
  if(!w)
    return;
  QPushButton * o = w->findChild<QPushButton*>("font_weight");
  
  if(o)
    o->setChecked(v);
}

void ReportHandler::setFontStyle(bool v)
{
  QObject * w = parent();
  if(_placeToolbarsOnWindows)
    w = activeDocumentWindow();
  if(!w)
    return;
  QPushButton * o = w->findChild<QPushButton*>("font_style");
  
  if(o)
    o->setChecked(v);
}

void ReportHandler::loadMemDB(const QString &filename, const QDomNode &it)
{
    _databaseElt = it.toElement();
    MemDbLoader mdb;
    bool ok = mdb.load(_databaseElt, QSqlDatabase::database());
    if(!ok)
        QMessageBox::warning(0, filename, mdb.lastError());
}

void ReportHandler::sectionToggled(bool v)
{
    if(!v)
        return;

    DocumentWindow * gw = activeDocumentWindow();
    if(!gw)
      return;

    bool exit = false;

    foreach(QAction *a, m_SectionActions) {
        if(a->isChecked())
        {
            // change selected items to new section

            ORGraphicsSectionItem * section = gw->_scene->getSection(a->text());
            if(section)
            {
                QList<QGraphicsItem*> lstSel = gw->_scene->selectedItems();
                foreach(QGraphicsItem *item, lstSel)
                {
                    if(item->parentItem() != section)
                    {
                        QPointF scenePos = item->scenePos();
                        item->setParentItem(section);
                        item->setPos(item->mapToParent(item->mapFromScene(scenePos)));
                        exit = true;
                        break;
                    }
                }
            }

            if(exit)
                break;
        }
    }
}
