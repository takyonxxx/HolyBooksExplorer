#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantList>
#include <QMap>

// Verse structure for all books
struct Verse {
    int suraNo;
    int verseNo;
    QString text;
    QString textEn;      // English translation
    QString arabic;      // Only for Quran
    QString latin;       // Only for Quran
    QString suraName;
    QString bookName;    // For Injil
    int revelationOrder; // Revelation order for Quran
};

// Word meaning structure for Quran
struct WordMeaning {
    int suraNo;
    int verseNo;
    QString latin;
    QString turkish;
    QString english;
};

// Sura/Chapter structure
struct Chapter {
    int no;
    QString name;
    QString displayName;
    QString bookName;    // For Injil which has multiple books
    int revelationOrder;
    QString revelationName;
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    enum BookType {
        Quran,
        Injil,
        Tevrat,
        Zebur
    };
    Q_ENUM(BookType)

    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool openDatabase(const QString &path);
    void closeDatabase();
    bool isOpen() const;

    // Book operations
    QStringList getBookNames() const;
    BookType getBookType(const QString &bookName) const;

    // Chapter operations
    QList<Chapter> getChapters(BookType bookType) const;
    QList<Chapter> getChaptersByRevelationOrder(BookType bookType) const;
    
    // Verse operations
    QList<Verse> getVerses(BookType bookType, int chapterNo, const QString &bookName = QString()) const;
    QList<Verse> searchVerses(BookType bookType, const QString &searchText) const;
    
    // Word meanings (Quran only)
    QList<WordMeaning> getWordMeanings(int suraNo, int verseNo) const;
    
    // Statistics
    int getVerseCount(BookType bookType, int chapterNo = -1) const;
    int getChapterCount(BookType bookType) const;

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    
    QString bookTypeToTablePrefix(BookType type) const;
    QList<Verse> parseQuranResults(QSqlQuery &query) const;
    QList<Verse> parseInjilResults(QSqlQuery &query) const;
    QList<Verse> parseTevratZeburResults(QSqlQuery &query, BookType type) const;
};

#endif // DATABASEMANAGER_H
