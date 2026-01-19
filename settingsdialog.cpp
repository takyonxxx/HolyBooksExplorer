#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "translationworker.h"
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
#include <QDebug>
#include <QShowEvent>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_showArabic(true)
    , m_showLatin(true)
    , m_highlightColor(Qt::yellow)
    , m_translationWorker(nullptr)
{
    ui->setupUi(this);
    setupUi();
    setupTranslationUi();
    loadSettings();
    // loadSurahList will be called in showEvent when database is ready
}

SettingsDialog::~SettingsDialog()
{
    if (m_translationWorker) {
        m_translationWorker->cancelTranslation();
        delete m_translationWorker;
    }
    delete ui;
}

void SettingsDialog::setupUi()
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(500, 600);
    resize(520, 680);
    
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
    m_translationGroup = new QGroupBox(tr("Translation Tool (Claude API)"), this);
    
    QVBoxLayout *groupLayout = new QVBoxLayout(m_translationGroup);
    groupLayout->setSpacing(8);
    
    // API Key
    QHBoxLayout *apiLayout = new QHBoxLayout();
    QLabel *apiLabel = new QLabel(tr("API Key:"), this);
    apiLabel->setFixedWidth(100);
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("Enter your Anthropic API Key"));
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
    m_logEdit->setFixedHeight(100);
    m_logEdit->setStyleSheet("QTextEdit { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Consolas', 'Courier New', monospace; font-size: 9px; }");
    groupLayout->addWidget(m_logEdit);
    
    // Get the main layout and insert translation group before spacer
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        // Remove the vertical spacer temporarily
        QLayoutItem *spacerItem = nullptr;
        QLayoutItem *buttonBoxItem = nullptr;
        
        // Find and remove spacer and button box
        for (int i = mainLayout->count() - 1; i >= 0; --i) {
            QLayoutItem *item = mainLayout->itemAt(i);
            if (item->spacerItem() && !spacerItem) {
                spacerItem = mainLayout->takeAt(i);
            } else if (item->widget() && qobject_cast<QDialogButtonBox*>(item->widget())) {
                buttonBoxItem = mainLayout->takeAt(i);
            }
        }
        
        // Add translation group
        mainLayout->addWidget(m_translationGroup);
        
        // Re-add spacer and button box
        if (spacerItem) {
            mainLayout->addItem(spacerItem);
        }
        if (buttonBoxItem) {
            mainLayout->addItem(buttonBoxItem);
        }
    }
    
    // Connections
    connect(m_startButton, &QPushButton::clicked, this, &SettingsDialog::onStartTranslation);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelTranslation);
    
    // Create translation worker
    m_translationWorker = new TranslationWorker(this);
    connect(m_translationWorker, &TranslationWorker::progressChanged, 
            this, &SettingsDialog::onTranslationProgress);
    connect(m_translationWorker, &TranslationWorker::translationComplete, 
            this, &SettingsDialog::onTranslationComplete);
    connect(m_translationWorker, &TranslationWorker::translationError, 
            this, &SettingsDialog::onTranslationError);
    connect(m_translationWorker, &TranslationWorker::logMessage, 
            this, &SettingsDialog::onTranslationLog);
}

void SettingsDialog::loadSurahList()
{
    m_surahCombo->clear();
    
    // Find an open database connection
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
        qWarning() << "No open database connection found. Connections:" << connections;
        return;
    }
    
    QSqlQuery query(db);
    if (!query.exec("SELECT sureno, sure FROM tbl_kuran_sureler ORDER BY sureno")) {
        qWarning() << "Failed to load surah list:" << query.lastError().text();
        return;
    }
    
    while (query.next()) {
        int sureNo = query.value(0).toInt();
        QString sureName = query.value(1).toString().trimmed();
        m_surahCombo->addItem(QString("%1 - %2").arg(sureNo).arg(sureName), sureNo);
    }
    
    qDebug() << "Loaded" << m_surahCombo->count() << "surahs";
}

void SettingsDialog::loadSettings()
{
    QSettings settings;
    
    m_font.setFamily(settings.value("font/family", "Segoe UI").toString());
    m_font.setPointSize(settings.value("font/size", 12).toInt());
    m_language = settings.value("language", "tr").toString();
    m_showArabic = settings.value("display/showArabic", true).toBool();
    m_showLatin = settings.value("display/showLatin", true).toBool();
    m_highlightColor = QColor(settings.value("display/highlightColor", "#FFFF00").toString());
    
    // Load API key (stored securely)
    QString apiKey = settings.value("translation/apiKey", "").toString();
    m_apiKeyEdit->setText(apiKey);
    
    // Update UI
    ui->fontComboBox->setCurrentFont(m_font);
    ui->fontSizeSpinBox->setValue(m_font.pointSize());
    
    int langIndex = ui->languageCombo->findData(m_language);
    if (langIndex >= 0) {
        ui->languageCombo->setCurrentIndex(langIndex);
    }
    
    ui->showArabicCheckBox->setChecked(m_showArabic);
    ui->showLatinCheckBox->setChecked(m_showLatin);
    
    // Update highlight color button
    QString colorStyle = QString("background-color: %1; border: 1px solid #ccc; border-radius: 3px;").arg(m_highlightColor.name());
    ui->highlightColorButton->setStyleSheet(colorStyle);
}

void SettingsDialog::saveSettings()
{
    QSettings settings;
    
    settings.setValue("font/family", m_font.family());
    settings.setValue("font/size", m_font.pointSize());
    settings.setValue("language", ui->languageCombo->currentData().toString());
    settings.setValue("display/showArabic", ui->showArabicCheckBox->isChecked());
    settings.setValue("display/showLatin", ui->showLatinCheckBox->isChecked());
    settings.setValue("display/highlightColor", m_highlightColor.name());
    
    // Save API key
    settings.setValue("translation/apiKey", m_apiKeyEdit->text());
}

void SettingsDialog::setCurrentFont(const QFont &font)
{
    m_font = font;
    ui->fontComboBox->setCurrentFont(font);
    ui->fontSizeSpinBox->setValue(font.pointSize());
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
    if (index >= 0) {
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
    ui->showArabicCheckBox->setChecked(show);
}

bool SettingsDialog::showArabic() const
{
    return ui->showArabicCheckBox->isChecked();
}

void SettingsDialog::setShowLatin(bool show)
{
    m_showLatin = show;
    ui->showLatinCheckBox->setChecked(show);
}

bool SettingsDialog::showLatin() const
{
    return ui->showLatinCheckBox->isChecked();
}

void SettingsDialog::setHighlightColor(const QColor &color)
{
    m_highlightColor = color;
    QString colorStyle = QString("background-color: %1; border: 1px solid #ccc; border-radius: 3px;").arg(color.name());
    ui->highlightColorButton->setStyleSheet(colorStyle);
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
        QString colorStyle = QString("background-color: %1; border: 1px solid #ccc; border-radius: 3px;").arg(color.name());
        ui->highlightColorButton->setStyleSheet(colorStyle);
    }
}

void SettingsDialog::onStartTranslation()
{
    QString apiKey = m_apiKeyEdit->text().trimmed();
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter your API Key"));
        return;
    }
    
    if (m_surahCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a Surah"));
        return;
    }
    
    int sureNo = m_surahCombo->currentData().toInt();
    TranslationWorker::TranslationType type = m_mealRadio->isChecked() 
        ? TranslationWorker::TranslateMeal 
        : TranslationWorker::TranslateWord;
    
    // Save API key
    QSettings settings;
    settings.setValue("translation/apiKey", apiKey);
    
    // Update UI
    m_startButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    m_surahCombo->setEnabled(false);
    m_mealRadio->setEnabled(false);
    m_wordRadio->setEnabled(false);
    m_apiKeyEdit->setEnabled(false);
    m_progressBar->setValue(0);
    m_logEdit->clear();
    
    // Start translation
    m_translationWorker->setApiKey(apiKey);
    m_translationWorker->startTranslation(sureNo, type);
}

void SettingsDialog::onCancelTranslation()
{
    m_translationWorker->cancelTranslation();
    
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_surahCombo->setEnabled(true);
    m_mealRadio->setEnabled(true);
    m_wordRadio->setEnabled(true);
    m_apiKeyEdit->setEnabled(true);
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
    m_surahCombo->setEnabled(true);
    m_mealRadio->setEnabled(true);
    m_wordRadio->setEnabled(true);
    m_apiKeyEdit->setEnabled(true);
    
    QMessageBox::information(this, tr("Complete"), tr("Translation completed successfully!"));
}

void SettingsDialog::onTranslationError(const QString &error)
{
    m_logEdit->append(QString("<span style='color: #f38ba8;'>ERROR: %1</span>").arg(error));
    
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_surahCombo->setEnabled(true);
    m_mealRadio->setEnabled(true);
    m_wordRadio->setEnabled(true);
    m_apiKeyEdit->setEnabled(true);
}

void SettingsDialog::onTranslationLog(const QString &message)
{
    m_logEdit->append(message);
    // Scroll to bottom
    QTextCursor cursor = m_logEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(cursor);
}

void SettingsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    
    // Load surah list when dialog is shown (database should be open by now)
    if (m_surahCombo->count() == 0) {
        loadSurahList();
    }
}
