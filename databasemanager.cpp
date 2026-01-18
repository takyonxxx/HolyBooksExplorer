#include "databasemanager.h"
#include <QSqlError>
#include <QDebug>
#include <QUuid>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    m_connectionName = QUuid::createUuid().toString();
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::openDatabase(const QString &path)
{
    if (m_db.isOpen()) {
        closeDatabase();
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

QStringList DatabaseManager::getBookNames() const
{
    return QStringList() << "Kuran" << "İncil" << "Tevrat" << "Zebur";
}

DatabaseManager::BookType DatabaseManager::getBookType(const QString &bookName) const
{
    if (bookName == "Kuran" || bookName == "Quran")
        return Quran;
    else if (bookName == "İncil" || bookName == "Injil" || bookName == "Gospel")
        return Injil;
    else if (bookName == "Tevrat" || bookName == "Torah")
        return Tevrat;
    else if (bookName == "Zebur" || bookName == "Psalms")
        return Zebur;
    return Quran;
}

QString DatabaseManager::bookTypeToTablePrefix(BookType type) const
{
    switch (type) {
    case Quran: return "kuran";
    case Injil: return "incil";
    case Tevrat: return "tevrat";
    case Zebur: return "zebur";
    }
    return "kuran";
}

QList<Chapter> DatabaseManager::getChapters(BookType bookType) const
{
    QList<Chapter> chapters;
    if (!m_db.isOpen()) return chapters;

    QSqlQuery query(m_db);
    QString tableName = QString("tbl_%1_sureler").arg(bookTypeToTablePrefix(bookType));

    if (bookType == Quran) {
        query.prepare(QString("SELECT sureno, sure, sureadi, sureinisno, sureinis, sureinisadi FROM %1 ORDER BY sureno").arg(tableName));
    } else if (bookType == Injil) {
        query.prepare(QString("SELECT sureno, kitap, sureadi FROM %1 ORDER BY kitap, sureno").arg(tableName));
    } else {
        query.prepare(QString("SELECT sureno, sureadi FROM %1 ORDER BY sureno").arg(tableName));
    }

    if (query.exec()) {
        while (query.next()) {
            Chapter ch;
            ch.no = query.value(0).toInt();
            
            if (bookType == Quran) {
                ch.name = query.value(1).toString().trimmed();
                ch.displayName = query.value(2).toString().trimmed();
                ch.revelationOrder = query.value(3).toInt();
                ch.revelationName = query.value(5).toString().trimmed();
            } else if (bookType == Injil) {
                ch.bookName = query.value(1).toString().trimmed();
                ch.displayName = query.value(2).toString().trimmed();
                ch.name = ch.displayName;
            } else {
                ch.displayName = query.value(1).toString().trimmed();
                ch.name = ch.displayName;
            }
            
            chapters.append(ch);
        }
    } else {
        qWarning() << "Query failed:" << query.lastError().text();
    }

    return chapters;
}

QList<Chapter> DatabaseManager::getChaptersByRevelationOrder(BookType bookType) const
{
    QList<Chapter> chapters;
    if (!m_db.isOpen() || bookType != Quran) return getChapters(bookType);

    QSqlQuery query(m_db);
    // s1: İniş sırası indeksi, s2: Gerçek sure bilgileri
    query.prepare(
        "SELECT s2.sureno, s2.sure, s2.sureadi, s1.sureno as revelation_order "
        "FROM tbl_kuran_sureler s1 "
        "JOIN tbl_kuran_sureler s2 ON s1.sureinisno = s2.sureno "
        "ORDER BY s1.sureno"
    );

    if (query.exec()) {
        while (query.next()) {
            Chapter ch;
            ch.no = query.value(0).toInt();
            ch.name = query.value(1).toString().trimmed();
            ch.displayName = query.value(2).toString().trimmed();
            ch.revelationOrder = query.value(3).toInt(); // İniş sırası
            chapters.append(ch);
        }
    }

    return chapters;
}

QList<Chapter> DatabaseManager::getChaptersByScientificContent(BookType bookType) const
{
    QList<Chapter> chapters;
    if (!m_db.isOpen() || bookType != Quran) return getChapters(bookType);

    QSqlQuery query(m_db);
    query.prepare("SELECT sureno, sure, sureadi FROM tbl_kuran_sureler WHERE bilimsel=1 ORDER BY sureno");

    if (query.exec()) {
        while (query.next()) {
            Chapter ch;
            ch.no = query.value(0).toInt();
            ch.name = query.value(1).toString().trimmed();
            ch.displayName = query.value(2).toString().trimmed();
            chapters.append(ch);
        }
    }

    return chapters;
}

QList<Verse> DatabaseManager::getVerses(BookType bookType, int chapterNo, const QString &bookName) const
{
    QList<Verse> verses;
    if (!m_db.isOpen()) return verses;

    QSqlQuery query(m_db);

    switch (bookType) {
    case Quran:
        // meal_saf ve meal_en sütunlarını kullan (saf mealler)
        query.prepare("SELECT sureno, ayet, meal_saf, meal_en, arapca, latince, sureadi FROM tbl_kuran_meal WHERE sureno = ? ORDER BY ayet");
        query.addBindValue(chapterNo);
        if (query.exec()) {
            verses = parseQuranResults(query);
        }
        break;

    case Injil:
        if (!bookName.isEmpty()) {
            query.prepare("SELECT sureno, kitap, ayet FROM tbl_incil WHERE sureno = ? AND kitap = ? ORDER BY rowid");
            query.addBindValue(chapterNo);
            query.addBindValue(bookName);
        } else {
            query.prepare("SELECT sureno, kitap, ayet FROM tbl_incil WHERE sureno = ? ORDER BY kitap, rowid");
            query.addBindValue(chapterNo);
        }
        if (query.exec()) {
            verses = parseInjilResults(query);
        }
        break;

    case Tevrat:
        query.prepare("SELECT sureno, ayet FROM tbl_tevrat WHERE sureno = ? ORDER BY rowid");
        query.addBindValue(chapterNo);
        if (query.exec()) {
            verses = parseTevratZeburResults(query, Tevrat);
        }
        break;

    case Zebur:
        query.prepare("SELECT sureno, ayet FROM tbl_zebur WHERE sureno = ? ORDER BY rowid");
        query.addBindValue(chapterNo);
        if (query.exec()) {
            verses = parseTevratZeburResults(query, Zebur);
        }
        break;
    }

    if (query.lastError().isValid()) {
        qWarning() << "Query failed:" << query.lastError().text();
    }

    return verses;
}

QList<Verse> DatabaseManager::searchVerses(BookType bookType, const QString &searchText) const
{
    QList<Verse> verses;
    if (!m_db.isOpen() || searchText.trimmed().isEmpty()) return verses;

    QSqlQuery query(m_db);
    QString searchPattern = QString("%%%1%%").arg(searchText);

    switch (bookType) {
    case Quran:
        // meal_saf ve meal_en'de ara
        query.prepare("SELECT sureno, ayet, meal_saf, meal_en, arapca, latince, sureadi FROM tbl_kuran_meal WHERE meal_saf LIKE ? OR meal_en LIKE ? OR latince LIKE ? ORDER BY sureno, ayet");
        query.addBindValue(searchPattern);
        query.addBindValue(searchPattern);
        query.addBindValue(searchPattern);
        if (query.exec()) {
            verses = parseQuranResults(query);
        }
        break;

    case Injil:
        query.prepare("SELECT sureno, kitap, ayet FROM tbl_incil WHERE ayet LIKE ? ORDER BY kitap, sureno, rowid");
        query.addBindValue(searchPattern);
        if (query.exec()) {
            verses = parseInjilResults(query);
        }
        break;

    case Tevrat:
        query.prepare("SELECT sureno, ayet FROM tbl_tevrat WHERE ayet LIKE ? ORDER BY sureno, rowid");
        query.addBindValue(searchPattern);
        if (query.exec()) {
            verses = parseTevratZeburResults(query, Tevrat);
        }
        break;

    case Zebur:
        query.prepare("SELECT sureno, ayet FROM tbl_zebur WHERE ayet LIKE ? ORDER BY sureno, rowid");
        query.addBindValue(searchPattern);
        if (query.exec()) {
            verses = parseTevratZeburResults(query, Zebur);
        }
        break;
    }

    return verses;
}

QList<Verse> DatabaseManager::parseQuranResults(QSqlQuery &query) const
{
    QList<Verse> verses;
    while (query.next()) {
        Verse v;
        v.suraNo = query.value(0).toInt();
        v.verseNo = query.value(1).toInt();
        v.text = query.value(2).toString();      // meal_saf (Türkçe saf meal)
        v.textEn = query.value(3).toString();    // meal_en (İngilizce çeviri)
        v.arabic = query.value(4).toString();
        v.latin = query.value(5).toString();
        v.suraName = query.value(6).toString().trimmed();
        verses.append(v);
    }
    return verses;
}

QList<Verse> DatabaseManager::parseInjilResults(QSqlQuery &query) const
{
    QList<Verse> verses;
    int verseNo = 1;
    int lastSuraNo = -1;
    QString lastBook;
    
    while (query.next()) {
        Verse v;
        v.suraNo = query.value(0).toInt();
        v.bookName = query.value(1).toString().trimmed();
        v.text = query.value(2).toString();
        
        // Reset verse number when chapter or book changes
        if (v.suraNo != lastSuraNo || v.bookName != lastBook) {
            verseNo = 1;
            lastSuraNo = v.suraNo;
            lastBook = v.bookName;
        }
        v.verseNo = verseNo++;
        v.suraName = QString("%1 - %2").arg(v.bookName).arg(v.suraNo);
        verses.append(v);
    }
    return verses;
}

QList<Verse> DatabaseManager::parseTevratZeburResults(QSqlQuery &query, BookType type) const
{
    QList<Verse> verses;
    int verseNo = 1;
    int lastSuraNo = -1;
    
    while (query.next()) {
        Verse v;
        v.suraNo = query.value(0).toInt();
        v.text = query.value(1).toString();
        
        if (v.suraNo != lastSuraNo) {
            verseNo = 1;
            lastSuraNo = v.suraNo;
        }
        v.verseNo = verseNo++;
        v.suraName = (type == Tevrat) ? QString("Bölüm %1").arg(v.suraNo) : QString("Mezmur %1").arg(v.suraNo);
        verses.append(v);
    }
    return verses;
}


QList<WordMeaning> DatabaseManager::getWordMeanings(int suraNo, int verseNo) const
{
    QList<WordMeaning> meanings;
    if (!m_db.isOpen()) return meanings;

    QSqlQuery query(m_db);
    query.prepare("SELECT sureno, ayet, latince, turkce, english FROM tbl_kuran_kelime WHERE sureno = ? AND ayet = ? ORDER BY latince");
    query.addBindValue(suraNo);
    query.addBindValue(verseNo);

    if (query.exec()) {
        while (query.next()) {
            WordMeaning wm;
            wm.suraNo = query.value(0).toInt();
            wm.verseNo = query.value(1).toInt();
            wm.latin = query.value(2).toString();
            wm.turkish = query.value(3).toString();            
            wm.english = query.value(4).toString();
            
            meanings.append(wm);
        }
    }

    return meanings;
}

int DatabaseManager::getVerseCount(BookType bookType, int chapterNo) const
{
    if (!m_db.isOpen()) return 0;

    QSqlQuery query(m_db);
    QString tableName;
    
    switch (bookType) {
    case Quran: tableName = "tbl_kuran_meal"; break;
    case Injil: tableName = "tbl_incil"; break;
    case Tevrat: tableName = "tbl_tevrat"; break;
    case Zebur: tableName = "tbl_zebur"; break;
    }

    if (chapterNo > 0) {
        query.prepare(QString("SELECT COUNT(*) FROM %1 WHERE sureno = ?").arg(tableName));
        query.addBindValue(chapterNo);
    } else {
        query.prepare(QString("SELECT COUNT(*) FROM %1").arg(tableName));
    }

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getChapterCount(BookType bookType) const
{
    if (!m_db.isOpen()) return 0;

    QSqlQuery query(m_db);
    QString tableName = QString("tbl_%1_sureler").arg(bookTypeToTablePrefix(bookType));
    
    query.prepare(QString("SELECT COUNT(DISTINCT sureno) FROM %1").arg(tableName));

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
