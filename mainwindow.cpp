#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QCloseEvent>
#include <QScrollBar>
#include <QTextStream>
#include <QApplication>
#include <QStyle>
#include <QTranslator>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_dbManager(new DatabaseManager(this))
    , m_currentBook(DatabaseManager::Quran)
    , m_currentChapter(1)
    , m_showArabic(true)
    , m_showLatin(true)
    , m_highlightColor(Qt::yellow)
    , m_bookLabel(nullptr)
    , m_chapterLabel(nullptr)
    , m_sortLabel(nullptr)
    , m_searchLabel(nullptr)
{
    ui->setupUi(this);
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupConnections();
    applyDarkTheme();
    loadSettings();
    
    // Database path
    QString defaultPath = QCoreApplication::applicationDirPath() + "/Kutsal_Kitaplar.db";
    QString resourcePath = ":/data/Kutsal_Kitaplar.db";
    
    // Eğer exe yanında database yoksa, resource'dan kopyala
    if (!QFile::exists(defaultPath)) {
        // Copy from resources
        if (QFile::copy(resourcePath, defaultPath)) {
            // Set file permissions to read/write
            QFile::setPermissions(defaultPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
            m_statusLabel->setText(tr("Database copied from resources"));
        } else {
            m_statusLabel->setText(tr("Failed to copy database from resources"));
        }
    }
    
    // Database'i aç
    m_databasePath = defaultPath;
    if (m_dbManager->openDatabase(defaultPath)) {
        loadChapters();
        m_statusLabel->setText(tr("Database loaded successfully"));
    } else {
        m_statusLabel->setText(tr("Failed to open database"));
    }
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::applyDarkTheme()
{
    // Koyu mavi tema
    QString darkBlueStyle = R"(
        QMainWindow {
            background-color: #0F192D;
        }
        QWidget {
            background-color: #0F192D;
            color: #DCE6F5;
        }
        QMenuBar {
            background-color: #192841;
            color: #DCE6F5;
            border-bottom: 1px solid #2D558C;
        }
        QMenuBar::item:selected {
            background-color: #2D558C;
        }
        QMenu {
            background-color: #192841;
            color: #DCE6F5;
            border: 1px solid #2D558C;
        }
        QMenu::item:selected {
            background-color: #2D558C;
        }
        QToolBar {
            background-color: #192841;
            border: none;
            spacing: 5px;
            padding: 3px;
        }
        QToolButton {
            background-color: #2D558C;
            color: #DCE6F5;
            border: 1px solid #4575A0;
            border-radius: 3px;
            padding: 5px 10px;
        }
        QToolButton:hover {
            background-color: #3D6DB0;
        }
        QComboBox {
            background-color: #192841;
            color: #DCE6F5;
            border: 1px solid #2D558C;
            border-radius: 3px;
            padding: 5px 8px;
            min-height: 25px;
        }
        QComboBox:hover {
            border-color: #4575A0;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #192841;
            color: #DCE6F5;
            selection-background-color: #2D558C;
            border: 1px solid #2D558C;
        }
        QLineEdit {
            background-color: #192841;
            color: #DCE6F5;
            border: 1px solid #2D558C;
            border-radius: 3px;
            padding: 5px 8px;
        }
        QLineEdit:focus {
            border-color: #6496DC;
        }
        QPushButton {
            background-color: #2D558C;
            color: #DCE6F5;
            border: 1px solid #4575A0;
            border-radius: 3px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #3D6DB0;
        }
        QPushButton:pressed {
            background-color: #1D4570;
        }
        QScrollArea {
            background-color: #0F192D;
            border: none;
        }
        QScrollBar:vertical {
            background-color: #192841;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: #2D558C;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4575A0;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #192841;
            height: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal {
            background-color: #2D558C;
            border-radius: 5px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4575A0;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QLabel {
            color: #DCE6F5;
        }
        QStatusBar {
            background-color: #192841;
            color: #90CAF9;
            border-top: 1px solid #2D558C;
        }
        QSplitter::handle {
            background-color: #2D558C;
        }
    )";

    setStyleSheet(darkBlueStyle);
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("Holy Books Explorer"));
    setMinimumSize(1200, 800);
    
    // Central widget with splitter
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Left panel - Controls
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(8);
    
    // Book selection
    m_bookLabel = new QLabel(tr("📖 Book:"), leftPanel);
    m_bookLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #90CAF9;");
    leftLayout->addWidget(m_bookLabel);
    
    m_bookCombo = new QComboBox(leftPanel);
    m_bookCombo->addItem(tr("📗 Quran"), "Kuran");
    m_bookCombo->addItem(tr("📕 Gospel"), "İncil");
    m_bookCombo->addItem(tr("📘 Torah"), "Tevrat");
    m_bookCombo->addItem(tr("📙 Psalms"), "Zebur");
    leftLayout->addWidget(m_bookCombo);
    
    // Chapter selection
    m_chapterLabel = new QLabel(tr("📑 Chapter/Sure:"), leftPanel);
    m_chapterLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #90CAF9;");
    leftLayout->addWidget(m_chapterLabel);
    
    m_chapterCombo = new QComboBox(leftPanel);
    m_chapterCombo->setMaxVisibleItems(20);
    leftLayout->addWidget(m_chapterCombo);
    
    // Sort order (for Quran)
    m_sortLabel = new QLabel(tr("🔄 Sort Order:"), leftPanel);
    m_sortLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #90CAF9;");
    leftLayout->addWidget(m_sortLabel);
    
    m_sortCombo = new QComboBox(leftPanel);
    m_sortCombo->addItem(tr("Default Order"), "number");
    m_sortCombo->addItem(tr("Revelation Order"), "revelation");
    m_sortCombo->addItem(tr("Scientific Verses"), "scientific");
    m_sortCombo->addItem(tr("Verse Length (Shortest First)"), "short");
    m_sortCombo->addItem(tr("Verse Length (Longest First)"), "long");
    leftLayout->addWidget(m_sortCombo);
    
    // Search
    m_searchLabel = new QLabel(tr("🔍 Search:"), leftPanel);
    m_searchLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #90CAF9;");
    leftLayout->addWidget(m_searchLabel);
    
    m_searchEdit = new QLineEdit(leftPanel);
    m_searchEdit->setPlaceholderText(tr("Enter keyword to search..."));
    leftLayout->addWidget(m_searchEdit);
    
    QHBoxLayout *searchButtonLayout = new QHBoxLayout();
    searchButtonLayout->setSpacing(5);
    
    m_searchButton = new QPushButton(tr("🔍 Search"), leftPanel);
    m_searchButton->setStyleSheet("background-color: #1565C0; color: white; font-weight: bold;");
    searchButtonLayout->addWidget(m_searchButton);
    
    m_clearSearchButton = new QPushButton(tr("✖ Clear"), leftPanel);
    m_clearSearchButton->setStyleSheet("background-color: #616161; color: white;");
    searchButtonLayout->addWidget(m_clearSearchButton);
    leftLayout->addLayout(searchButtonLayout);
    
    // Statistics
    m_statsLabel = new QLabel(leftPanel);
    m_statsLabel->setStyleSheet("color: #A0A0A0; font-size: 11px; padding: 8px; background-color: #2D2D2D; border-radius: 4px;");
    m_statsLabel->setWordWrap(true);
    leftLayout->addWidget(m_statsLabel);
    
    leftLayout->addStretch();
    
    leftPanel->setMinimumWidth(260);
    leftPanel->setMaximumWidth(320);
    mainSplitter->addWidget(leftPanel);
    
    // Center panel - Verses
    QWidget *centerPanel = new QWidget(this);
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setContentsMargins(3, 3, 3, 3);
    
    m_verseScrollArea = new QScrollArea(centerPanel);
    m_verseScrollArea->setWidgetResizable(true);
    m_verseScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_verseContainer = new QWidget();
    m_verseContainer->setStyleSheet("background-color: #252525;");
    m_verseLayout = new QVBoxLayout(m_verseContainer);
    m_verseLayout->setContentsMargins(3, 3, 3, 3);
    m_verseLayout->setSpacing(3);
    m_verseLayout->addStretch();
    
    m_verseScrollArea->setWidget(m_verseContainer);
    centerLayout->addWidget(m_verseScrollArea);
    
    mainSplitter->addWidget(centerPanel);
    
    // Right panel - Word analysis (for Quran)
    m_wordAnalysisWidget = new WordAnalysisWidget(this);
    m_wordAnalysisWidget->setMinimumWidth(280);
    m_wordAnalysisWidget->setMaximumWidth(380);
    mainSplitter->addWidget(m_wordAnalysisWidget);
    
    // Status bar
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, 1);
    
    // Set splitter sizes
    mainSplitter->setSizes({280, 600, 320});
}

void MainWindow::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *openAction = fileMenu->addAction(tr("&Open Database..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenDatabase);
    
    fileMenu->addSeparator();
    
    QAction *exportChapterAction = fileMenu->addAction(tr("Export &Chapter..."));
    connect(exportChapterAction, &QAction::triggered, this, &MainWindow::onExportChapter);
    
    QAction *exportSearchAction = fileMenu->addAction(tr("Export &Search Results..."));
    connect(exportSearchAction, &QAction::triggered, this, &MainWindow::onExportSearchResults);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Edit menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    
    QAction *settingsAction = editMenu->addAction(tr("&Settings..."));
    settingsAction->setShortcut(QKeySequence::Preferences);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    // Language menu
    QMenu *langMenu = menuBar()->addMenu(tr("&Language"));
    
    QAction *turkishAction = langMenu->addAction("🇹🇷 Türkçe");
    turkishAction->setData("tr");
    connect(turkishAction, &QAction::triggered, this, [this]() { onLanguageChanged("tr"); });
    
    QAction *englishAction = langMenu->addAction("🇬🇧 English");
    englishAction->setData("en");
    connect(englishAction, &QAction::triggered, this, [this]() { onLanguageChanged("en"); });
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About..."));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    toolBar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Open"), this, &MainWindow::onOpenDatabase);
    toolBar->addSeparator();
    toolBar->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Settings"), this, &MainWindow::onSettings);
}

void MainWindow::setupConnections()
{
    connect(m_bookCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onBookChanged);
    connect(m_chapterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onChapterChanged);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSortOrderChanged);
    
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(m_clearSearchButton, &QPushButton::clicked, this, &MainWindow::onClearSearchClicked);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchButtonClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Window geometry
    restoreGeometry(settings.value("window/geometry").toByteArray());
    restoreState(settings.value("window/state").toByteArray());
    
    // Font
    m_appFont.setFamily(settings.value("font/family", "Segoe UI").toString());
    m_appFont.setPointSize(settings.value("font/size", 11).toInt());
    applyFont(m_appFont);
    
    // Language - Default Türkçe
    m_language = settings.value("language", "tr").toString();
    if (m_language.isEmpty() || (m_language != "tr" && m_language != "en")) {
        m_language = "tr";
        settings.setValue("language", "tr");
    }
    m_wordAnalysisWidget->setLanguage(m_language);
    
    // Display options
    m_showArabic = settings.value("display/showArabic", true).toBool();
    m_showLatin = settings.value("display/showLatin", true).toBool();
    m_highlightColor = QColor(settings.value("display/highlightColor", "#FFFF00").toString());
    
    // Database path
    m_databasePath = settings.value("database/path", "").toString();
    if (!m_databasePath.isEmpty() && QFile::exists(m_databasePath)) {
        m_dbManager->openDatabase(m_databasePath);
        loadChapters();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    settings.setValue("database/path", m_databasePath);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

void MainWindow::onBookChanged(int index)
{
    Q_UNUSED(index);
    QString bookName = m_bookCombo->currentData().toString();
    m_currentBook = m_dbManager->getBookType(bookName);
    
    // Show/hide sort combo based on book type
    m_sortCombo->setVisible(m_currentBook == DatabaseManager::Quran);
    
    // Show/hide word analysis for Quran only
    m_wordAnalysisWidget->setVisible(m_currentBook == DatabaseManager::Quran);
    
    loadChapters();
    m_wordAnalysisWidget->clear();
}

void MainWindow::onChapterChanged(int index)
{
    if (index < 0) return;
    
    m_currentChapter = m_chapterCombo->currentData().toInt();
    
    // For Injil, also get book name
    if (m_currentBook == DatabaseManager::Injil) {
        QString displayText = m_chapterCombo->currentText();
        QStringList parts = displayText.split(" - ");
        if (parts.size() >= 2) {
            m_currentBookName = parts[0].trimmed();
        }
    }
    
    loadVerses();
    m_wordAnalysisWidget->clear();
}

void MainWindow::onSortOrderChanged(int index)
{
    Q_UNUSED(index);
    loadChapters();
}

void MainWindow::loadChapters()
{
    if (!m_dbManager->isOpen()) return;
    
    m_chapterCombo->blockSignals(true);
    m_chapterCombo->clear();
    
    QList<Chapter> chapters;
    QString sortType = m_sortCombo->currentData().toString();
    
    if (m_currentBook == DatabaseManager::Quran && sortType == "revelation") {
        chapters = m_dbManager->getChaptersByRevelationOrder(m_currentBook);
    } else if (m_currentBook == DatabaseManager::Quran && sortType == "scientific") {
        chapters = m_dbManager->getChaptersByScientificContent(m_currentBook);
    } else {
        chapters = m_dbManager->getChapters(m_currentBook);
    }
    
    for (const Chapter &ch : chapters) {
        QString displayText;
        if (m_currentBook == DatabaseManager::Injil) {
            displayText = QString("%1 - %2").arg(ch.bookName).arg(ch.displayName);
        } else if (m_currentBook == DatabaseManager::Quran && sortType == "revelation") {
            displayText = QString("%1. %2 (Sıra: %3)").arg(ch.revelationOrder).arg(ch.name).arg(ch.no);
        } else {
            displayText = ch.displayName;
        }
        m_chapterCombo->addItem(displayText, ch.no);
    }
    
    m_chapterCombo->blockSignals(false);
    
    if (m_chapterCombo->count() > 0) {
        m_chapterCombo->setCurrentIndex(0);
        onChapterChanged(0);
    }
    
    updateStatistics();
}

void MainWindow::loadVerses()
{
    if (!m_dbManager->isOpen()) return;
    
    m_currentVerses = m_dbManager->getVerses(m_currentBook, m_currentChapter, m_currentBookName);
    displayVerses(m_currentVerses);
    updateStatistics();
}

void MainWindow::displayVerses(const QList<Verse> &verses)
{
    clearVerses();
    
    bool showDetails = m_showArabic || m_showLatin;
    
    for (const Verse &verse : verses) {
        VerseWidget *vw = new VerseWidget(m_verseContainer);
        vw->setFont(m_appFont);
        vw->setLanguage(m_language);
        vw->setVerse(verse, showDetails && m_currentBook == DatabaseManager::Quran);
        
        if (!m_currentSearchText.isEmpty()) {
            vw->setSearchHighlight(m_currentSearchText);
        }
        
        connect(vw, &VerseWidget::verseClicked, this, &MainWindow::onVerseClicked);
        
        m_verseLayout->insertWidget(m_verseLayout->count() - 1, vw);
        m_verseWidgets.append(vw);
    }
    
    // Scroll to top
    m_verseScrollArea->verticalScrollBar()->setValue(0);
}

void MainWindow::clearVerses()
{
    for (VerseWidget *vw : m_verseWidgets) {
        m_verseLayout->removeWidget(vw);
        vw->deleteLater();
    }
    m_verseWidgets.clear();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    // Live highlight as user types
    for (VerseWidget *vw : m_verseWidgets) {
        vw->setSearchHighlight(text);
    }
}

void MainWindow::onSearchButtonClicked()
{
    QString searchText = m_searchEdit->text().trimmed();
    if (searchText.isEmpty()) {
        loadVerses();
        return;
    }
    
    m_currentSearchText = searchText;
    m_searchResults = m_dbManager->searchVerses(m_currentBook, searchText);
    
    displayVerses(m_searchResults);
    
    m_statusLabel->setText(tr("Found %1 verses containing '%2'")
                          .arg(m_searchResults.size())
                          .arg(searchText));
}

void MainWindow::onClearSearchClicked()
{
    m_searchEdit->clear();
    m_currentSearchText.clear();
    m_searchResults.clear();
    loadVerses();
    m_statusLabel->setText(tr("Search cleared"));
}

void MainWindow::onVerseClicked(const Verse &verse)
{
    // Load word meanings for Quran
    if (m_currentBook == DatabaseManager::Quran) {
        QList<WordMeaning> meanings = m_dbManager->getWordMeanings(verse.suraNo, verse.verseNo);
        m_wordAnalysisWidget->setWordMeanings(meanings);
    }
    
    m_statusLabel->setText(tr("Selected: %1:%2").arg(verse.suraNo).arg(verse.verseNo));
}

void MainWindow::onOpenDatabase()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Database"),
        QDir::homePath(),
        tr("SQLite Database (*.db *.sqlite);;All Files (*)")
    );
    
    if (fileName.isEmpty()) return;
    
    if (m_dbManager->openDatabase(fileName)) {
        m_databasePath = fileName;
        loadChapters();
        m_statusLabel->setText(tr("Database loaded: %1").arg(QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open database file."));
    }
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(this);
    dialog.setCurrentFont(m_appFont);
    dialog.setCurrentLanguage(m_language);
    dialog.setShowArabic(m_showArabic);
    dialog.setShowLatin(m_showLatin);
    dialog.setHighlightColor(m_highlightColor);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString oldLanguage = m_language;
        
        m_appFont = dialog.selectedFont();
        m_language = dialog.selectedLanguage();
        m_showArabic = dialog.showArabic();
        m_showLatin = dialog.showLatin();
        m_highlightColor = dialog.highlightColor();
        
        applyFont(m_appFont);
        
        // Dil değiştiyse translator'ı güncelle
        if (oldLanguage != m_language) {
            onLanguageChanged(m_language);
        } else {
            m_wordAnalysisWidget->setLanguage(m_language);
        }
        
        // Reload verses with new settings
        if (!m_searchResults.isEmpty()) {
            displayVerses(m_searchResults);
        } else {
            displayVerses(m_currentVerses);
        }
    }
}

void MainWindow::onAbout()
{
    QString aboutText;
    if (m_language == "tr") {
        aboutText = QString(
            "<h2>Kutsal Kitaplar Gezgini</h2>"
            "<p>Versiyon: %1</p>"
            "<p>Bu uygulama, Kuran-ı Kerim, İncil, Tevrat ve Zebur'u "
            "incelemenizi sağlayan kapsamlı bir araçtır.</p>"
            "<h3>Özellikler:</h3>"
            "<ul>"
            "<li>Dört kutsal kitabı görüntüleme</li>"
            "<li>Sure/Bölüm bazında ayet okuma</li>"
            "<li>Kelime arama ve vurgulama</li>"
            "<li>Kuran için kelime anlamları</li>"
            "<li>İniş sırasına göre sıralama</li>"
            "<li>Türkçe ve İngilizce dil desteği</li>"
            "</ul>"
            "<p>© 2024 Türkay Biliyor</p>"
        ).arg(APP_VERSION);
    } else {
        aboutText = QString(
            "<h2>Holy Books Explorer</h2>"
            "<p>Version: %1</p>"
            "<p>This application is a comprehensive tool for exploring "
            "the Quran, Gospel, Torah, and Psalms.</p>"
            "<h3>Features:</h3>"
            "<ul>"
            "<li>View four holy books</li>"
            "<li>Read verses by chapter/sure</li>"
            "<li>Word search and highlighting</li>"
            "<li>Word meanings for Quran</li>"
            "<li>Sort by revelation order</li>"
            "<li>Turkish and English language support</li>"
            "</ul>"
            "<p>© 2024 Türkay Biliyor</p>"
        ).arg(APP_VERSION);
    }
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("About"));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(aboutText);
    msgBox.setStyleSheet("QMessageBox { background-color: #2D2D2D; } QLabel { color: #E0E0E0; }");
    msgBox.exec();
}

void MainWindow::onLanguageChanged(const QString &lang)
{
    m_language = lang;
    m_wordAnalysisWidget->setLanguage(lang);
    
    for (VerseWidget *vw : m_verseWidgets) {
        vw->setLanguage(lang);
    }
    
    QSettings settings;
    settings.setValue("language", lang);
    
    // Yeni translator yükle
    static QTranslator *appTranslator = nullptr;
    
    if (appTranslator) {
        qApp->removeTranslator(appTranslator);
        delete appTranslator;
    }
    
    appTranslator = new QTranslator(qApp);
    QString translationFile = QString(":/translations/holybooksexplorer_%1.qm").arg(lang);
    
    if (appTranslator->load(translationFile)) {
        qApp->installTranslator(appTranslator);
        retranslateUi();
    }
}

void MainWindow::onExportChapter()
{
    if (m_currentVerses.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No verses to export."));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Chapter"),
        QDir::homePath() + "/" + m_chapterCombo->currentText() + ".txt",
        tr("Text Files (*.txt);;All Files (*)")
    );
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        
        stream << m_chapterCombo->currentText() << "\n";
        stream << QString("=").repeated(50) << "\n\n";
        
        for (const Verse &verse : m_currentVerses) {
            stream << QString("%1:%2 - %3\n").arg(verse.suraNo).arg(verse.verseNo).arg(verse.text);
            if (!verse.arabic.isEmpty()) {
                stream << verse.arabic << "\n";
            }
            if (!verse.latin.isEmpty()) {
                stream << verse.latin << "\n";
            }
            stream << "\n";
        }
        
        file.close();
        m_statusLabel->setText(tr("Chapter exported to: %1").arg(fileName));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save file."));
    }
}

void MainWindow::onExportSearchResults()
{
    if (m_searchResults.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No search results to export."));
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Search Results"),
        QDir::homePath() + "/search_results.txt",
        tr("Text Files (*.txt);;All Files (*)")
    );
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        
        stream << tr("Search Results for: %1").arg(m_currentSearchText) << "\n";
        stream << tr("Found: %1 verses").arg(m_searchResults.size()) << "\n";
        stream << QString("=").repeated(50) << "\n\n";
        
        for (const Verse &verse : m_searchResults) {
            stream << QString("%1 %2:%3\n").arg(verse.suraName).arg(verse.suraNo).arg(verse.verseNo);
            stream << verse.text << "\n\n";
        }
        
        file.close();
        m_statusLabel->setText(tr("Search results exported to: %1").arg(fileName));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save file."));
    }
}

void MainWindow::updateStatistics()
{
    if (!m_dbManager->isOpen()) {
        m_statsLabel->clear();
        return;
    }
    
    int totalChapters = m_dbManager->getChapterCount(m_currentBook);
    int totalVerses = m_dbManager->getVerseCount(m_currentBook);
    int chapterVerses = m_dbManager->getVerseCount(m_currentBook, m_currentChapter);
    
    QString stats;
    if (m_language == "tr") {
        stats = QString(
            "📊 İstatistikler:\n"
            "• Toplam Bölüm: %1\n"
            "• Toplam Ayet: %2\n"
            "• Bu Bölümde: %3 ayet"
        ).arg(totalChapters).arg(totalVerses).arg(chapterVerses);
    } else {
        stats = QString(
            "📊 Statistics:\n"
            "• Total Chapters: %1\n"
            "• Total Verses: %2\n"
            "• This Chapter: %3 verses"
        ).arg(totalChapters).arg(totalVerses).arg(chapterVerses);
    }
    
    m_statsLabel->setText(stats);
}

void MainWindow::applyFont(const QFont &font)
{
    m_appFont = font;
    
    for (VerseWidget *vw : m_verseWidgets) {
        vw->setFont(font);
    }
    
    m_wordAnalysisWidget->setFont(font);
    m_searchEdit->setFont(font);
}


void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("Holy Books Explorer"));
    
    menuBar()->clear();
    setupMenuBar();
    
    QList<QToolBar*> toolbars = findChildren<QToolBar*>();
    for (QToolBar *toolbar : toolbars) {
        removeToolBar(toolbar);
        delete toolbar;
    }
    setupToolBar();
    
    if (m_bookLabel) m_bookLabel->setText(tr("📖 Book:"));
    if (m_chapterLabel) m_chapterLabel->setText(tr("📑 Chapter/Sure:"));
    if (m_sortLabel) m_sortLabel->setText(tr("🔄 Sort Order:"));
    if (m_searchLabel) m_searchLabel->setText(tr("🔍 Search:"));
    
    if (m_bookCombo) {
        int currentIndex = m_bookCombo->currentIndex();
        m_bookCombo->setItemText(0, tr("📗 Quran"));
        m_bookCombo->setItemText(1, tr("📕 Gospel"));
        m_bookCombo->setItemText(2, tr("📘 Torah"));
        m_bookCombo->setItemText(3, tr("📙 Psalms"));
        m_bookCombo->setCurrentIndex(currentIndex);
    }
    
    if (m_sortCombo) {
        int currentIndex = m_sortCombo->currentIndex();
        m_sortCombo->setItemText(0, tr("Default Order"));
        m_sortCombo->setItemText(1, tr("Revelation Order"));
        m_sortCombo->setItemText(2, tr("Scientific Verses"));
        m_sortCombo->setItemText(3, tr("Verse Length (Shortest First)"));
        m_sortCombo->setItemText(4, tr("Verse Length (Longest First)"));
        m_sortCombo->setCurrentIndex(currentIndex);
    }
    
    if (m_searchButton) m_searchButton->setText(tr("🔍 Search"));
    if (m_clearSearchButton) m_clearSearchButton->setText(tr("✖ Clear"));
    if (m_searchEdit) m_searchEdit->setPlaceholderText(tr("Enter keyword to search..."));
}
