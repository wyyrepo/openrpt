#include <QFile>
#include <QDomElement>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlField>
#include <QVariant>

#include "memdbloader.h"

const QString memDbName = ":memory:";


MemDbLoader::MemDbLoader()
{
}

MemDbLoader::~MemDbLoader()
{
}

QString MemDbLoader::createMemoryDB(QString id)
{
	if(!QSqlDatabase::isDriverAvailable("QSQLITE")) {
		return "SQLITE plugin not available - data can't be loaded";
	}
	if(id.isEmpty() || !QSqlDatabase::contains(id)) {
		QSqlDatabase db = id.isEmpty() ? QSqlDatabase::addDatabase("QSQLITE") : QSqlDatabase::addDatabase("QSQLITE", id);
		db.setDatabaseName(memDbName); 
	}
	return "";
}

// Load the DB from a file
bool MemDbLoader::load(const QString &filename, QSqlDatabase db)
{
    QFile * f = new QFile(filename);

    QDomDocument doc = QDomDocument();
    QString errMsg;
    int errLine, errCol;

    if(!doc.setContent(f,&errMsg,&errLine,&errCol)) {
        _lastError = QString().sprintf("Encountered and error while parsing %s\n\n\t%s (Line %d Column %d)",filename.toLatin1().data(),errMsg.toLatin1().data(),errLine,errCol);
        return false;
    }

    QDomElement root = doc.documentElement();

    return load(root, db);
}

// Load the DB from a DomElement
bool MemDbLoader::load(const QDomElement & elemSource, QSqlDatabase db)
{
    _lastError = "";
    
    // Login to the SQLITE memory DB

	if(db.isOpen()) {
		db.close();
		db.open();
	}

	if(db.databaseName() != memDbName) {
		_lastError = createMemoryDB();
		if(!_lastError.isEmpty()) {
			return false;
		}
		db = QSqlDatabase::database();
		if(!db.open()) {
			_lastError = "Error opening QSQLITE memory database";
			return false;
		}  
	}
	_db = db;

    QDomNodeList nlist = elemSource.childNodes();
    for(int i = 0; i < nlist.count(); i++ ) {
        QDomElement it = nlist.item(i).toElement();
        if(it.tagName()=="table") {
            parseTable(it.toElement());
        }
    }
    return _lastError.isEmpty();
}

void MemDbLoader::parseTable(const QDomElement & elemSource)
{
    _tableName = elemSource.attribute("name");
    _columnNames.clear();

    QDomNodeList nlist = elemSource.childNodes();
    for(int i = 0; i < nlist.count(); i++ ) {
        QDomElement it = nlist.item(i).toElement();
        if(it.tagName()=="columns") {
            parseColumns(it);
        }
        else if(it.tagName()=="record") {
            parseRecord(it);
        }
    }
}

void MemDbLoader::parseColumns(const QDomElement & elemSource)
{
    if(_tableName.isEmpty()) {
        _lastError = "Error: missing table name";
        return;
    }
    
    QString	myQuery = "CREATE TABLE " + _tableName + " (";

    QDomNodeList nlist = elemSource.childNodes();
    bool firstCol = true;
    for(int i = 0; i < nlist.count(); i++ ) {
        QDomElement it = nlist.item(i).toElement();
        if(it.tagName()=="column") {
            QString colName = it.text();
            QString colType = it.attribute("type");
            if(colType.isEmpty()) {
                colType = "TEXT";
            }
            if(!firstCol) {
                myQuery += ", ";
            }
            firstCol = false;

            myQuery += colName + " " + colType;
            _columnNames << colName;
        }
    }
    
    myQuery += ")";
    
    QSqlQuery	query(_db);

    if(!query.exec(myQuery)) {
        _lastError = "Failed Query: " + myQuery;
        return;
    }    
}

void MemDbLoader::parseRecord(const QDomElement & elemSource)
{
    QString	myQuery = "INSERT INTO " + _tableName + " (";
    
    bool firstCol = true;
    foreach (QString val, _columnNames) {
        if(!firstCol) {
            myQuery += ", ";
        }
        firstCol = false;
        myQuery += val;
    }

    myQuery += ") VALUES (";

    QSqlQuery	query(_db);

    QDomNodeList nlist = elemSource.childNodes();
    firstCol = true;
    for(int i = 0; i < nlist.count(); i++ ) {
        QDomElement it = nlist.item(i).toElement();
        if(it.tagName()=="val") {
            if(!firstCol) {
                myQuery += ", ";
            }
            firstCol = false;

            QSqlField field(_columnNames.at(i), QVariant::String);
            field.setValue(it.text());
            myQuery += query.driver()->formatValue (field); 
        }
    }
    
    myQuery += ")";
    
    if(!query.exec(myQuery)) {
        _lastError = "Failed Query: " + myQuery;
        return;
    }
}
