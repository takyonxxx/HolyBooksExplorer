#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QSettings>

#include "databasemanager.h"
#include "wordanalysiswidget.h"
#include "versewidget.h"
#include "searchhighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Book/Chapter selection
    void onBookChanged(int index);
    void onChapterChanged(int index);
    void onSortOrderChanged(int index);
    
    // Search
    void onSearchTextChanged(const QString &text);
    void onSearchButtonClicked();
    void onClearSearchClicked();
    
    // Verse selection
    void onVerseClicked(const Verse &verse);
    
    // Menu actions
    void onOpenDatabase();
    void onSettings();
    void onAbout();
    void onLanguageChanged(const QString &lang);
    
    // Export
    void onExportChapter();
    void onExportSearchResults();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void retranslateUi();
    void applyDarkTheme();
    
    void loadChapters();
    void loadVerses();
    void displayVerses(const QList<Verse> &verses);
    void clearVerses();
    void updateStatistics();
    void applyFont(const QFont &font);
    
    Ui::MainWindow *ui;
    
    // Database
    DatabaseManager *m_dbManager;
    QString m_databasePath;
    
    // Current state
    DatabaseManager::BookType m_currentBook;
    int m_currentChapter;
    QString m_currentBookName; // For Injil
    QString m_currentSearchText;
    QList<Verse> m_currentVerses;
    QList<Verse> m_searchResults;
    
    // UI Components
    QComboBox *m_bookCombo;
    QComboBox *m_chapterCombo;
    QComboBox *m_sortCombo;
    QLineEdit *m_searchEdit;
    QPushButton *m_searchButton;
    QPushButton *m_clearSearchButton;
    
    QScrollArea *m_verseScrollArea;
    QWidget *m_verseContainer;
    QVBoxLayout *m_verseLayout;
    QList<VerseWidget*> m_verseWidgets;
    
    WordAnalysisWidget *m_wordAnalysisWidget;
    
    QLabel *m_statusLabel;
    QLabel *m_statsLabel;
    
    // Settings
    QFont m_appFont;
    QString m_language;
    bool m_showArabic;
    bool m_showLatin;
    QColor m_highlightColor;
};

#endif // MAINWINDOW_H
