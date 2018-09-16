#include <QSettings>
#include <QLocale>
#include <QApplication>
#include <QStringList>
#include <QFile>

#include "languageoptions.h"


LanguageOptions::LanguageOptions(QObject *parent)
    : QObject(parent)
{
    _defaultLanguage = "default";
    addLanguage(_defaultLanguage, _defaultLanguage);

    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenRPT");
    _selectedLanguage = settings.value("/OpenRPT/_selectedLanguage", _defaultLanguage).toString();

    addLanguage("en", "English");
}



LanguageOptions::~LanguageOptions()
{
}


void LanguageOptions::addLanguage(QString languageID, QString languageTitle)
{
    if(_languages.contains(languageID)) 
    {
        return;
    }

    QStringList content;
    content << languageTitle;
    _languages.insert(languageID,content);
}



void LanguageOptions::addTranslation(QString languageID, QString translationFile)
{
    if(_languages.contains(languageID)) 
    {
        _languages[languageID] << translationFile;
    }    
}


void LanguageOptions::select(QString languageID)
{
    _selectedLanguage = languageID;

    QSettings settings(QSettings::UserScope, "OpenMFG.com", "OpenRPT");
    settings.setValue("/OpenRPT/_selectedLanguage", _selectedLanguage);
}



void LanguageOptions::installLanguage(QString languageID)
{
    QStringList content = _languages[languageID];

    for (int i=1; i<content.size(); i++) 
    {
        QTranslator *ptranslator = new QTranslator;
        ptranslator->load(content[i]);
        qApp->installTranslator(ptranslator);
    }

    // translation for Default title (can't do it before because the translations are not loaded)
    _languages[_defaultLanguage][0] = tr("Default");
}



void LanguageOptions::installSelected(void)

{
    installLanguage(_selectedLanguage);
}



void LanguageOptions::addTranslationToDefault(QString file)

{
  /* check if we want a resource language translation */
  if (file.startsWith(":")) {
    /* check if have a lg_CO/file.qm file in the locales which would override resource */
    QString base = QApplication::applicationDirPath() + "/locales/";
    QString fname = file.right(file.length()-1);
    QString locale = QLocale::system().name();
    if (QFile::exists(base + locale + fname)) {
      addTranslation(_defaultLanguage, base + locale + fname);
    } else {
      /* check if have a lg/file.qm file in the locales which would override resource */
      locale = locale.left(2);
      if (QFile::exists(base + locale +fname)) {
        addTranslation(_defaultLanguage, base + locale + fname);
      } else {
        /* load default resource file */
        addTranslation(_defaultLanguage, file);
      }
    }
  } else {
    /* load requested file */
    addTranslation(_defaultLanguage, file);
  }
}



QStringList LanguageOptions::languageTitlesList(void)
{
    QStringList res;

    foreach (QStringList value, _languages) 
    {
        res << value[0];
    }

    return res;
}


QString LanguageOptions::selectedTitle()
{
    return _languages.value(_selectedLanguage)[0];

}


void LanguageOptions::selectFromTitle(QString title)
{
    QString id = _defaultLanguage;

    QMapIterator<QString,QStringList> i(_languages);

     while (i.hasNext()) 
    {
         i.next();
        QString currentTitle = i.value()[0];
        if(currentTitle==title) {
            id = i.key();
            break;
        }
    }

    select(id);
}

