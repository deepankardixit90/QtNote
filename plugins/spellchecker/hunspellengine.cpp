#include "hunspellengine.h"

#include <QString>
#include <QSet>
#include <QDir>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QCoreApplication>
#include <QDataStream>
#include <hunspell/hunspell.hxx>

#include "utils.h"

namespace QtNote {

static QStringList dictPaths()
{
    static QStringList dictPaths;
    if (dictPaths.isEmpty()) {
        QSet<QString> dictPathSet;
        QString pathFromEnv = QString::fromLocal8Bit(qgetenv("MYSPELL_DICT_DIR"));
        if (!pathFromEnv.isEmpty())
            dictPathSet << pathFromEnv;
#if defined(Q_OS_WIN)
        dictPathSet << QCoreApplication::applicationDirPath() + QLatin1String(stringify(DICTSUBDIR));
        dictPathSet << Utils::qtnoteDataDir() + QLatin1String(stringify(DICTSUBDIR));
#elif defined(Q_OS_MAC)
        dictPathSet << QLatin1String("/opt/local/share/myspell"); // MacPorts standard paths
#else
        dictPathSet << QLatin1String("/usr/share/myspell")
                  << QLatin1String("/usr/share/hunspell")
                  << QLatin1String("/usr/local/share/myspell")
                  << QLatin1String("/usr/local/share/hunspell");
#endif
        dictPaths = dictPathSet.toList();
    }
    return dictPaths;
}

static bool scanDictPaths(const QString &language, QFileInfo &aff , QFileInfo &dic)
{
    foreach (const QString &dictPath, dictPaths()) {
        QDir dir(dictPath);
        if (dir.exists()) {
            QFileInfo affInfo(dir.filePath(language + QLatin1String(".aff")));
            QFileInfo dicInfo(dir.filePath(language + QLatin1String(".dic")));
            if (affInfo.isReadable() && dicInfo.isReadable()) {
                aff = affInfo;
                dic = dicInfo;
                return true;
            }
        }
    }

    return false;
}

HunspellEngine::HunspellEngine()
{
    QFile f(Utils::qtnoteDataDir() + QLatin1String("/spellcheck-custom.words"));
    if (f.open(QIODevice::ReadOnly)) {
        QDataStream in(&f);
        QString w;
        while (!in.atEnd()) {
            in >> w;
            runtimeDict << w;
        }
    }
}

HunspellEngine::~HunspellEngine()
{
    foreach (const LangItem &li, languages) {
        delete li.hunspell;
    }
    QFile f(Utils::qtnoteDataDir() + QLatin1String("/spellcheck-custom.words"));
    if (f.open(QIODevice::WriteOnly)) {
        QDataStream out(&f);
        for(auto w : runtimeDict.values()) {
            out << w;
        }
    } else {
        qDebug("Failed to write runtime spellcheck dictionary");
    }
}

QList<QLocale> HunspellEngine::supportedLanguages() const
{
    QMap<QString,QLocale> retHash;
    foreach (const QString &dictPath, dictPaths()) {
        QDir dir(dictPath);
        if (!dir.exists()) {
            continue;
        }
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList() << "*.dic", QDir::Files)) {
            QLocale locale(fi.baseName());
            if (locale != QLocale::c())  {
                retHash.insert(locale.nativeLanguageName()+locale.nativeCountryName(), locale);
            }
        }
    }
    return retHash.values();
}

bool HunspellEngine::addLanguage(const QLocale &locale)
{
    QString language = locale.name();
    QFileInfo aff, dic;
    if (scanDictPaths(language, aff, dic)) {
        LangItem li;
        //qDebug() << "Add hunspell:" << aff.absoluteFilePath() << dic.absoluteFilePath();
        QByteArray codecName(li.hunspell_->get_dic_encoding());
		if (codecName.startsWith("microsoft-cp125")) {
			codecName.replace(0, sizeof("microsoft-cp") - 1, "Windows-");
		} else if (codecName.startsWith("TIS620-2533")) {
			codecName.resize(sizeof("TIS620") - 1);
		}
		li.codec = QTextCodec::codecForName(codecName);
		if (li.codec) {
			li.info.language = locale.language();
			li.info.country = locale.country();
			li.info.filename = dic.filePath();
			languages_.append(li);
		} else {
			qDebug("Unsupported myspell dict encoding: \"%s\" for %s", codecName.data(), qPrintable(dic.fileName()));
		}
        return true;
    }
    return false;
}

bool HunspellEngine::spell(const QString &word) const
{
    if (runtimeDict.size() && runtimeDict.contains(word)) {
        return true;
    }
    foreach (const LangItem &li, languages) {
        if (li.hunspell->spell(li.codec->fromUnicode(word)) != 0) {
            return true;
        }
    }
    return false;
}

void HunspellEngine::addToDictionary(const QString &word)
{
    runtimeDict.insert(word);
}

QList<QString> HunspellEngine::suggestions(const QString& word)
{
    QStringList qtResult;
    foreach (const LangItem &li, languages) {
        char **result;
        int sugNum = li.hunspell->suggest(&result, li.codec->fromUnicode(word));
        for (int i=0; i < sugNum; i++) {
            qtResult << li.codec->toUnicode(result[i]);
        }
        li.hunspell->free_list(&result, sugNum);
    }
    return qtResult;
}

QList<SpellEngineInterface::DictInfo> HunspellEngine::loadedDicts() const
{
    QList<DictInfo> ret;
    foreach (const LangItem &li, languages) {
        ret.append(li.info);
    }
    return ret;
}

}
