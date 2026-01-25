#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "translationworker.h"
#include "googletranslateworker.h"
#include <QSettings>
#include <QColorDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>
#include <QShowEvent>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_showArabic(true)
    , m_showLatin(true)
    , m_highlightColor(Qt::yellow)
    , m_translationWorker(nullptr)
    , m_googleTranslateWorker(nullptr)
{
    ui->setupUi(this);
    setupUi();
    setupTranslationUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    if (m_translationWorker) {
        m_translationWorker->cancelTranslation();
        delete m_translationWorker;
    }
    if (m_googleTranslateWorker) {
        m_googleTranslateWorker->cancelTranslation();
        delete m_googleTranslateWorker;
    }
    delete ui;
}

void SettingsDialog::setupUi()
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(500, 700);
    resize(520, 780);
    
    // Font size range
    ui->fontSizeSpinBox->setRange(8, 32);
    ui->fontSizeSpinBox->setValue(12);
    
    // Language combo
    ui->languageCombo->addItem("Türkçe", "tr");
    ui->languageCombo->addItem("English", "en");
    
    // Connections
    connect(ui->fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsDialog::onFontSizeChanged);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged,
            this, [this](const QFont &font) { onFontFamilyChanged(font.family()); });
    connect(ui->highlightColorButton, &QPushButton::clicked,
            this, &SettingsDialog::onHighlightColorClicked);
    
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::setupTranslationUi()
{
    // Create translation group box
    m_translationGroup = new QGroupBox(tr("Translation Tool"), this);
    
    QVBoxLayout *groupLayout = new QVBoxLayout(m_translationGroup);
    groupLayout->setSpacing(8);
    
    // Translation service selection
    QHBoxLayout *serviceLayout = new QHBoxLayout();
    QLabel *serviceLabel = new QLabel(tr("Service:"), this);
    serviceLabel->setFixedWidth(100);
    m_serviceCombo = new QComboBox(this);
    m_serviceCombo->addItem(tr("Google Translate (Free)"), "google");
    m_serviceCombo->addItem(tr("Claude API (Requires Key)"), "claude");
    m_serviceCombo->setToolTip(tr("Google Translate: Free but limited rate\nClaude API: Higher quality but requires API key"));
    serviceLayout->addWidget(serviceLabel);
    serviceLayout->addWidget(m_serviceCombo);
    groupLayout->addLayout(serviceLayout);
    
    // API Key (sadece Claude için görünür)
    QHBoxLayout *apiLayout = new QHBoxLayout();
    QLabel *apiLabel = new QLabel(tr("API Key:"), this);
    apiLabel->setFixedWidth(100);
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("Required for Claude API"));
    m_apiKeyEdit->setEnabled(false); // Başlangıçta disabled
    apiLayout->addWidget(apiLabel);
    apiLayout->addWidget(m_apiKeyEdit);
    groupLayout->addLayout(apiLayout);
    
    // Surah selection
    QHBoxLayout *surahLayout = new QHBoxLayout();
    QLabel *surahLabel = new QLabel(tr("Select Surah:"), this);
    surahLabel->setFixedWidth(100);
    m_surahCombo = new QComboBox(this);
    m_surahCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    surahLayout->addWidget(surahLabel);
    surahLayout->addWidget(m_surahCombo);
    groupLayout->addLayout(surahLayout);
    
    // Translation type
    QHBoxLayout *typeLayout = new QHBoxLayout();
    QLabel *typeLabel = new QLabel(tr("Type:"), this);
    typeLabel->setFixedWidth(100);
    m_mealRadio = new QRadioButton(tr("Verses (meal)"), this);
    m_wordRadio = new QRadioButton(tr("Words (kelime)"), this);
    m_mealRadio->setChecked(true);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_mealRadio);
    typeLayout->addWidget(m_wordRadio);
    typeLayout->addStretch();
    groupLayout->addLayout(typeLayout);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(tr("Start Translation"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setEnabled(false);
    
    // Style buttons
    m_startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 6px 12px; border-radius: 4px; } QPushButton:hover { background-color: #45a049; } QPushButton:disabled { background-color: #cccccc; }");
    m_cancelButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 6px 12px; border-radius: 4px; } QPushButton:hover { background-color: #da190b; } QPushButton:disabled { background-color: #cccccc; }");
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    groupLayout->addLayout(buttonLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%v / %m (%p%)");
    m_progressBar->setFixedHeight(20);
    groupLayout->addWidget(m_progressBar);
    
    // Log area
    QLabel *logLabel = new QLabel(tr("Log:"), this);
    groupLayout->addWidget(logLabel);
    
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setFixedHeight(120);
    m_logEdit->setStyleSheet("QTextEdit { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Consolas', 'Courier New', monospace; font-size: 9px; }");
    groupLayout->addWidget(m_logEdit);
    
    // Info label for Google Translate
    QLabel *infoLabel = new QLabel(tr("<i>Google Translate: ~15 requests/min, free but may be blocked by Google<br>"
                                     "Claude API: Higher quality, requires API key from anthropic.com</i>"), this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { color: #666; font-size: 8pt; }");
    groupLayout->addWidget(infoLabel);
    
    // Get the main layout and insert translation group before spacer
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        QLayoutItem *spacerItem = nullptr;
        QLayoutItem *buttonBoxItem = nullptr;
        
        for (int i = mainLayout->count() - 1; i >= 0; --i) {
            QLayoutItem *item = mainLayout->itemAt(i);
            if (item->spacerItem() && !spacerItem) {
                spacerItem = mainLayout->takeAt(i);
            } else if (item->widget() && qobject_cast<QDialogButtonBox*>(item->widget())) {
                buttonBoxItem = mainLayout->takeAt(i);
            }
        }
        
        mainLayout->addWidget(m_translationGroup);
        
        if (spacerItem) {
            mainLayout->addItem(spacerItem);
        }
        if (buttonBoxItem) {
            mainLayout->addItem(buttonBoxItem);
        }
    }
    
    // Connections
    connect(m_serviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onTranslationServiceChanged);
    connect(m_startButton, &QPushButton::clicked, this, &SettingsDialog::onStartTranslation);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelTranslation);
    
    // Create workers
    m_translationWorker = new TranslationWorker(this);
    connect(m_translationWorker, &TranslationWorker::progressChanged, 
            this, &SettingsDialog::onTranslationProgress);
    connect(m_translationWorker, &TranslationWorker::translationComplete, 
            this, &SettingsDialog::onTranslationComplete);
    connect(m_translationWorker, &TranslationWorker::translationError, 
            this, &SettingsDialog::onTranslationError);
    connect(m_translationWorker, &TranslationWorker::logMessage, 
            this, &SettingsDialog::onTranslationLog);
    
    m_googleTranslateWorker = new GoogleTranslateWorker(this);
    connect(m_googleTranslateWorker, &GoogleTranslateWorker::progressChanged, 
            this, &SettingsDialog::onTranslationProgress);
    connect(m_googleTranslateWorker, &GoogleTranslateWorker::translationComplete, 
            this, &SettingsDialog::onTranslationComplete);
    connect(m_googleTranslateWorker, &GoogleTranslateWorker::translationError, 
            this, &SettingsDialog::onTranslationError);
    connect(m_googleTranslateWorker, &GoogleTranslateWorker::logMessage, 
            this, &SettingsDialog::onTranslationLog);
}

void SettingsDialog::onTranslationServiceChanged()
{
    QString service = m_serviceCombo->currentData().toString();
    
    // API key sadece Claude için gerekli
    m_apiKeyEdit->setEnabled(service == "claude");
    
    if (service == "google") {
        m_logEdit->append(tr("[INFO] Using Google Translate (free, ~15 requests/min)"));
    } else {
        m_logEdit->append(tr("[INFO] Using Claude API (requires API key)"));
    }
}

void SettingsDialog::onStartTranslation()
{
    QString service = m_serviceCombo->currentData().toString();
    
    if (service == "claude") {
        QString apiKey = m_apiKeyEdit->text().trimmed();
        if (apiKey.isEmpty()) {
            QMessageBox::warning(this, tr("API Key Required"), 
                               tr("Please enter your Anthropic API Key for Claude API.\n\n"
                                  "Or switch to Google Translate (free)."));
            return;
        }
        m_translationWorker->setApiKey(apiKey);
    }
    
    int sureNo = m_surahCombo->currentData().toInt();
    
    m_startButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_progressBar->setValue(0);
    m_logEdit->clear();
    
    if (service == "google") {
        m_logEdit->append(tr("=== Starting Google Translate ==="));
        m_logEdit->append(tr("Note: Rate limited to ~15 requests/min"));
        
        if (m_mealRadio->isChecked()) {
            m_googleTranslateWorker->startTranslation(sureNo, GoogleTranslateWorker::TranslateMeal);
        } else {
            m_googleTranslateWorker->startTranslation(sureNo, GoogleTranslateWorker::TranslateWord);
        }
    } else {
        m_logEdit->append(tr("=== Starting Claude API Translation ==="));
        
        if (m_mealRadio->isChecked()) {
            m_translationWorker->startTranslation(sureNo, TranslationWorker::TranslateMeal);
        } else {
            m_translationWorker->startTranslation(sureNo, TranslationWorker::TranslateWord);
        }
    }
}

void SettingsDialog::onCancelTranslation()
{
    QString service = m_serviceCombo->currentData().toString();
    
    if (service == "google") {
        m_googleTranslateWorker->cancelTranslation();
    } else {
        m_translationWorker->cancelTranslation();
    }
    
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
}

void SettingsDialog::onTranslationProgress(int current, int total)
{
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
}

void SettingsDialog::onTranslationComplete()
{
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_logEdit->append(tr("\n=== Translation Completed Successfully! ==="));
    
    QMessageBox::information(this, tr("Translation Complete"), 
                           tr("Translation process completed successfully!"));
}

void SettingsDialog::onTranslationError(const QString &error)
{
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_logEdit->append(tr("\n[ERROR] %1").arg(error));
    
    QMessageBox::critical(this, tr("Translation Error"), error);
}

void SettingsDialog::onTranslationLog(const QString &message)
{
    m_logEdit->append(message);
    m_logEdit->verticalScrollBar()->setValue(m_logEdit->verticalScrollBar()->maximum());
}

// Diğer mevcut fonksiyonlar (değişiklik yok)...
void SettingsDialog::loadSurahList()
{
    m_surahCombo->clear();
    m_surahCombo->addItem(tr("All Surahs (1-114)"), -1);
    
    QSqlDatabase db;
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &connName : connections) {
        QSqlDatabase tempDb = QSqlDatabase::database(connName);
        if (tempDb.isOpen()) {
            db = tempDb;
            break;
        }
    }
    
    if (!db.isValid() || !db.isOpen()) {
        return;
    }
    
    QSqlQuery query(db);
    // Sure numarası ve ismini al
    query.prepare("SELECT sureno, sureadi FROM tbl_kuran_sureler ORDER BY sureno");
    
    if (query.exec()) {
        while (query.next()) {
            int sureNo = query.value(0).toInt();
            QString sureAdi = query.value(1).toString().trimmed();
            
            // "Sure No - Sure Adı" formatında göster
            QString displayText = QString("%1 - %2").arg(sureNo).arg(sureAdi);
            m_surahCombo->addItem(displayText, sureNo);
        }
    }
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    loadSurahList();
}

void SettingsDialog::loadSettings()
{
    QSettings settings("Anthropic", "HolyBooksExplorer");
    
    m_showArabic = settings.value("Display/ShowArabic", true).toBool();
    m_showLatin = settings.value("Display/ShowLatin", true).toBool();
    
    QString colorName = settings.value("Display/HighlightColor", "yellow").toString();
    m_highlightColor = QColor(colorName);
    
    int fontSize = settings.value("Display/FontSize", 12).toInt();
    ui->fontSizeSpinBox->setValue(fontSize);
    
    QString fontFamily = settings.value("Display/FontFamily", "Arial").toString();
    ui->fontComboBox->setCurrentFont(QFont(fontFamily));
    
    m_language = settings.value("Language", "tr").toString();
    int langIndex = ui->languageCombo->findData(m_language);
    if (langIndex != -1) {
        ui->languageCombo->setCurrentIndex(langIndex);
    }
    
    // Load API key
    QString apiKey = settings.value("Translation/ClaudeAPIKey", "").toString();
    m_apiKeyEdit->setText(apiKey);
    
    // Load service preference
    QString service = settings.value("Translation/Service", "google").toString();
    int serviceIndex = m_serviceCombo->findData(service);
    if (serviceIndex != -1) {
        m_serviceCombo->setCurrentIndex(serviceIndex);
    }
}

void SettingsDialog::saveSettings()
{
    QSettings settings("Anthropic", "HolyBooksExplorer");
    
    settings.setValue("Display/ShowArabic", m_showArabic);
    settings.setValue("Display/ShowLatin", m_showLatin);
    settings.setValue("Display/HighlightColor", m_highlightColor.name());
    
    settings.setValue("Display/FontSize", ui->fontSizeSpinBox->value());
    settings.setValue("Display/FontFamily", ui->fontComboBox->currentFont().family());
    
    settings.setValue("Language", ui->languageCombo->currentData().toString());
    
    // Save API key
    settings.setValue("Translation/ClaudeAPIKey", m_apiKeyEdit->text());
    
    // Save service preference
    settings.setValue("Translation/Service", m_serviceCombo->currentData().toString());
}

// Getter/setter methods...
void SettingsDialog::setCurrentFont(const QFont &font)
{
    m_font = font;
    ui->fontSizeSpinBox->setValue(font.pointSize());
    ui->fontComboBox->setCurrentFont(font);
}

QFont SettingsDialog::selectedFont() const
{
    QFont font = ui->fontComboBox->currentFont();
    font.setPointSize(ui->fontSizeSpinBox->value());
    return font;
}

void SettingsDialog::setCurrentLanguage(const QString &lang)
{
    m_language = lang;
    int index = ui->languageCombo->findData(lang);
    if (index != -1) {
        ui->languageCombo->setCurrentIndex(index);
    }
}

QString SettingsDialog::selectedLanguage() const
{
    return ui->languageCombo->currentData().toString();
}

void SettingsDialog::setShowArabic(bool show)
{
    m_showArabic = show;
}

bool SettingsDialog::showArabic() const
{
    return m_showArabic;
}

void SettingsDialog::setShowLatin(bool show)
{
    m_showLatin = show;
}

bool SettingsDialog::showLatin() const
{
    return m_showLatin;
}

void SettingsDialog::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
}

QColor SettingsDialog::highlightColor() const
{
    return m_highlightColor;
}

void SettingsDialog::onFontSizeChanged(int size)
{
    m_font.setPointSize(size);
}

void SettingsDialog::onFontFamilyChanged(const QString &family)
{
    m_font.setFamily(family);
}

void SettingsDialog::onHighlightColorClicked()
{
    QColor color = QColorDialog::getColor(m_highlightColor, this, tr("Select Highlight Color"));
    if (color.isValid()) {
        m_highlightColor = color;
    }
}
