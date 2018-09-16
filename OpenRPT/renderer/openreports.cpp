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
#include "openreports.h"

#include "orprerender.h"
#include "orprintrender.h"
#include "renderobjects.h"
#include "builtinSqlFunctions.h"
#include "previewdialog.h"

#include <QString>
#include <QVariant>
#include <QPrinter>
#include <QPainter>
#include <QPrintDialog>
#include <QApplication>
#include <QMessageBox>

#include <xsqlquery.h>
#include <parameter.h>

//
// Class orReportPrivate
//
class orReportPrivate {
  public:
    orReportPrivate();
    ~orReportPrivate();

    QString _reportName;

    bool _reportExists;

    ORPreRender _prerenderer;
    ORODocument * _genDoc;
};

orReportPrivate::orReportPrivate()
{
  _reportExists = false;  
  _genDoc = 0;
}

orReportPrivate::~orReportPrivate()
{
  if (_genDoc)
  {
    delete _genDoc;
    _genDoc = 0;
  }
}

//
// Class orReport
//
orReport::orReport(QSqlDatabase pDb)
  : _internal(0)
{
  _internal = new orReportPrivate();
  setDatabase(pDb);
}

orReport::orReport(const QString &qstrDomname, QSqlDatabase pDb)
  : _internal(0)
{
  _internal = new orReportPrivate();
  if(_internal != 0)
  {
    setDatabase(pDb);
    constructor(qstrDomname);
  }
}

orReport::orReport(const QString &qstrDomname, const QStringList &lstPParameters, QSqlDatabase pDb)
  : _internal(0)
{
  _internal = new orReportPrivate();

  if(_internal != 0) {
    setDatabase(pDb);
    setParamList(lstPParameters);

    constructor(qstrDomname);
  }
}

orReport::orReport(const QString &qstrDomname, const ParameterList &pParams, QSqlDatabase pDb)
  : _internal(0)
{
  _internal = new orReportPrivate();

  if(_internal != 0) {
    setDatabase(pDb);
    setParamList(pParams);

    constructor(qstrDomname);
  }
}

orReport::orReport(const char *pReportName, const ParameterList &pParams, QSqlDatabase pDb)
  : _internal(0)
{
  _internal = new orReportPrivate();

  if(_internal != 0) {
    setDatabase(pDb);
    setParamList(pParams);

    constructor(QString(pReportName));
  }
}

void orReport::constructor(const QString &pReportName)
{
  _internal->_reportName = pReportName;
  XSqlQuery report(_internal->_prerenderer.database());
  report.prepare(getSqlFromTag("fmt04", _internal->_prerenderer.database().driverName()));	// MANU
  report.bindValue(":report_name", pReportName);
  report.exec();
  if (report.first())
  {
    _internal->_reportExists = true;
    QString errorMessage;
    int     errorLine;

    QDomDocument docReport;
    if (docReport.setContent(report.value("report_source").toString(), &errorMessage, &errorLine))
      setDom(docReport);
  }
  else
    _internal->_reportExists = false;
}

orReport::~orReport()
{
  if(_internal != 0)
  {
    delete _internal;
    _internal = 0;
  }
}

QPrinter* orReport::multiPrinter = 0;
QPainter* orReport::multiPainter = 0;

bool orReport::beginMultiPrint(QPrinter *pPrinter)
{
  if (pPrinter == 0)
    return false;
  else if (multiPrinter != 0 && pPrinter != multiPrinter)
    return false;

  multiPrinter = pPrinter;
  multiPainter = new QPainter();

  return true;
}

bool orReport::beginMultiPrint(QPrinter *pPrinter, bool & userCanceled)
{
  userCanceled = false;
  if (pPrinter == 0)
    return false;
  else if (multiPrinter != 0 && pPrinter != multiPrinter)
    return false;

  QPrintDialog printerSetup(pPrinter);
  if (printerSetup.exec() != QDialog::Accepted)
  {
    userCanceled = true;
    return false;
  }

  multiPrinter = pPrinter;
  multiPainter = new QPainter();

  return true;
}

bool orReport::print(QPrinter *prtThis, bool boolSetupPrinter, bool showPreview, QWidget *parent)
{
// TODO: Figure out how all this is supposed to be with the new engine
  bool retval = false;
  bool localPrinter = false;

  if (multiPainter)
  {
    if(_internal->_prerenderer.isValid())
    {
      _internal->_genDoc = _internal->_prerenderer.generate();
      if(_internal->_genDoc)
      {
        ORPrintRender prender;
        prender.setupPrinter(_internal->_genDoc, prtThis);
        if (boolSetupPrinter)     // 1st call
        {
          retval = multiPainter->begin(multiPrinter);
          if (retval == false)
          {
            delete multiPainter;
            multiPrinter = 0;
            multiPainter = 0;
          }
        }
        else                      // 2nd or later print call
          retval = multiPrinter->newPage();
        if (retval)
          retval = render(multiPainter, multiPrinter);

        delete _internal->_genDoc;
        _internal->_genDoc = 0;
      }
    }
  }
  else
  {
    if(_internal != 0)
    {
      if (prtThis == 0)
      {
        prtThis = new ReportPrinter(QPrinter::HighResolution);
        localPrinter = true;
      }

      if(_internal->_prerenderer.isValid())
      {
        _internal->_genDoc = _internal->_prerenderer.generate();
        if(_internal->_genDoc)
        {
          retval = true;
          ORPrintRender prender;
          prender.setupPrinter(_internal->_genDoc, prtThis);

          if (showPreview)
          {
            PreviewDialog preview(_internal->_genDoc, prtThis, parent);
            if (preview.exec() == QDialog::Rejected)
              return false;
          }

          QPrintDialog pd(prtThis);
          if (boolSetupPrinter)
          {
            pd.setMinMax(1, _internal->_genDoc->pages());
            retval = (pd.exec() == QDialog::Accepted);
          }
  
          if(retval == true)
            retval = render(0, prtThis);

          delete _internal->_genDoc;
          _internal->_genDoc = 0;
        }
      }

      if (localPrinter && prtThis != 0)
        delete prtThis;
    }
  }

  return retval;
}

bool orReport::endMultiPrint(QPrinter *pPrinter)
{
  if (pPrinter != multiPrinter || pPrinter == 0)
    return false;

  if (multiPainter)
  {
    if (multiPainter->isActive())
      multiPainter->end();
    delete multiPainter;
  }

  multiPainter = 0;
  multiPrinter =  0;

  return true;
}


bool orReport::render(QPainter *pPainter, QPrinter *pPrinter)
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
  bool retval = false;

  if(_internal != 0 && pPrinter != 0)
  {
    bool localAlloc = false;
    if(_internal->_genDoc == 0)
    {
      _internal->_genDoc = _internal->_prerenderer.generate();
      localAlloc = true;
    }

    if(_internal->_genDoc)
    {
      ORPrintRender render;
  
      render.setPrinter(pPrinter);
      render.setPainter(pPainter);
      retval = render.render(_internal->_genDoc);

      if (localAlloc)
      {
        delete _internal->_genDoc;
        _internal->_genDoc = 0;
      }
    }
  }
  else
  {
    // rendering to a non print device isn't supported at this time
    qDebug("Trying to render to a non print device");
  }

  QApplication::restoreOverrideCursor();

  return retval;
}

bool orReport::exportToPDF( const QString& fileName )
{
  if ( !isValid() )
    return false;

  ORODocument * doc = _internal->_prerenderer.generate();

  if(doc)
  {
    QString localFileName = fileName;

    QPrinter printer( QPrinter::HighResolution );
    if (! localFileName.endsWith(".pdf", Qt::CaseInsensitive))
      localFileName.append(".pdf");
    printer.setOutputFileName( localFileName );
#ifdef Q_WS_MAC
    printer.setOutputFormat( QPrinter::NativeFormat );
#else
    printer.setOutputFormat( QPrinter::PdfFormat );
#endif

    ORPrintRender render;
    render.setupPrinter(doc, &printer);
    render.setPrinter(&printer);
    bool res = render.render(doc);
    delete doc;
    return res;
  }
  return false;
}

QString orReport::watermarkText()
{
  return ( _internal != 0 ? _internal->_prerenderer.watermarkText() : QString::null );
}

void orReport::setWatermarkText(const QString & txt)
{
  if(_internal != 0)
    _internal->_prerenderer.setWatermarkText(txt);
}

QFont orReport::watermarkFont()
{
  return (_internal != 0 ? _internal->_prerenderer.watermarkFont() : QFont() );
}

void orReport::setWatermarkFont(const QFont & fnt)
{
  if(_internal != 0)
    _internal->_prerenderer.setWatermarkFont(fnt);
}

unsigned char orReport::watermarkOpacity()
{
  return ( _internal != 0 ? _internal->_prerenderer.watermarkOpacity() : 0 );
}

void orReport::setWatermarkOpacity(unsigned char o)
{
  if(_internal != 0)
    _internal->_prerenderer.setWatermarkOpacity(o);
}

QImage orReport::backgroundImage()
{
  return ( _internal != 0 ? _internal->_prerenderer.backgroundImage() : QImage() );
}

void orReport::setBackgroundImage(const QImage & img)
{
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundImage(img);
}

QRect orReport::backgroundRect()
{
  if(_internal != 0)
  {
    QRectF rf = _internal->_prerenderer.backgroundRect();
    if(rf.isValid())
      return QRect((int)(rf.x() * 100), (int)(rf.y() * 100), (int)(rf.width() * 100), (int)(rf.height() * 100));
  }
  return QRect();
}

void orReport::setBackgroundRect(const QRect & r)
{
  setBackgroundRect(r.x(), r.y(), r.width(), r.height());
}

void orReport::setBackgroundRect(int x, int y, int w, int h)
{
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundRect(x/100.0, y/100.0, w/100.0, h/100.0);
}

unsigned char orReport::backgroundOpacity()
{
  return ( _internal != 0 ? _internal->_prerenderer.backgroundOpacity() : 0 );
}

void orReport::setBackgroundOpacity(unsigned char o)
{
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundOpacity(o);
}

int orReport::backgroundAlignment()
{
  return ( _internal != 0 ? _internal->_prerenderer.backgroundAlignment() : 0 );
}

void orReport::setBackgroundAlignment(int a) {
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundAlignment(a);
}

bool orReport::backgroundScale()
{
 return ( _internal != 0 ? _internal->_prerenderer.backgroundScale() : false );
}

void orReport::setBackgroundScale(bool scale)
{
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundScale(scale);
}

Qt::AspectRatioMode orReport::backgroundScaleMode()
{
  return ( _internal != 0 ? _internal->_prerenderer.backgroundScaleMode() : Qt::IgnoreAspectRatio );
}

void orReport::setBackgroundScaleMode(Qt::AspectRatioMode mode)
{
  if(_internal != 0)
    _internal->_prerenderer.setBackgroundScaleMode(mode);
}

bool orReport::isValid()
{
  if(_internal != 0)
    return _internal->_prerenderer.isValid();
  return false;
}

void orReport::setDatabase(QSqlDatabase pDb)
{
  if(_internal != 0)
    _internal->_prerenderer.setDatabase(pDb);
}

bool orReport::setDom(const QDomDocument & docPReport)
{
  if(_internal != 0)
  {
    _internal->_reportExists = true;
    return _internal->_prerenderer.setDom(docPReport);
  }
  return false;
}

void orReport::setParamList(const QStringList & lstPParameters)
{
  ParameterList plist;
  for(int t = 0; t < lstPParameters.count(); t++)
    plist.append(Parameter(QString(), lstPParameters[t]));

  setParamList(plist);
}

void orReport::setParamList(const ParameterList & pParamList)
{
  if(_internal != 0)
    _internal->_prerenderer.setParamList(pParamList);
}

ParameterList orReport::getParamList()
{
  ParameterList plist;
  if(_internal != 0)
    plist = _internal->_prerenderer.paramList();
  return plist;
}

bool orReport::doParamsSatisfy()
{
  if(_internal != 0)
    return _internal->_prerenderer.doParamsSatisfy();
  return true;
}

bool orReport::doesReportExist()
{
  if(_internal != 0)
    return _internal->_reportExists;
  return false;
}

bool orReport::satisfyParams(QWidget * /*widget*/)
{
// TODO: implement this somehow
qDebug("orReport[_wrapper]::satisfyParams() not implemented");
/*
  // get list of missing parameters
  QStringList mlist;
  QStringList::iterator it;
  QuerySource * qs = 0;
  for(unsigned int i = 0; i < _internal->_reportData->queries.size(); i++) {
      qs = _internal->_reportData->queries.get(i);
      orQuery qry(qs->name(), qs->query(_internal->_database), _internal->_lstParameters, true, _internal->_database);
      it = qry.missingParamList.begin();
      for(it = qry.missingParamList.begin(); it != qry.missingParamList.end(); it++) {
          if((*it)[0] != '%' && !mlist.contains(*it)) mlist.append(*it);
      }
  }

  if(mlist.count() == 0) return true; // if the params satisfy then no reason to go further

  bool ok = false;
  QString ret = QString::null;
  for(it = mlist.begin(); it != mlist.end(); it++) {
      ret = QInputDialog::getText(QObject::tr("Missing Report Parameter"), QString(QObject::tr("Enter in a value for the parameter \"%1\":")).arg(*it),
                                   QLineEdit::Normal, QString::null, &ok, widget);
      if(!ok || ret.isEmpty()) return false;
      _internal->_lstParameters.append(Parameter(*it, ret));
  }
*/

  // now that we have gathered up the information we needed we will check
  // one more time and return the value to indicate if everything went as planned
  return doParamsSatisfy();
}

int orReport::reportError(QWidget *pParent)
{
  if (!_internal->_reportExists)
    QMessageBox::critical( pParent, QObject::tr("Report Definition Not Found"),
                           QObject::tr( "The report definition for this report, \"%1\" cannot be found.\n"
                                        "Please contact your Systems Administrator and report this issue." )
                           .arg(_internal->_reportName) );
  else
    QMessageBox::critical( pParent, QObject::tr("Unknown Error"),
                           QObject::tr( "An unknown error was encountered while processing your request.\n"
                                        "Please contact your Systems Administrator and report this issue." ) );
//  ToDo  Add support for unsatisfied parameters list and return something meaningful
  return -1;
}

