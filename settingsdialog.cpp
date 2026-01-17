#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QColorDialog>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , m_showArabic(true)
    , m_showLatin(true)
    , m_highlightColor(Qt::yellow)
{
    ui->setupUi(this);
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setupUi()
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(400, 350);
    
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

void SettingsDialog::loadSettings()
{
    QSettings settings;
    
    m_font.setFamily(settings.value("font/family", "Segoe UI").toString());
    m_font.setPointSize(settings.value("font/size", 12).toInt());
    m_language = settings.value("language", "tr").toString();
    m_showArabic = settings.value("display/showArabic", true).toBool();
    m_showLatin = settings.value("display/showLatin", true).toBool();
    m_highlightColor = QColor(settings.value("display/highlightColor", "#FFFF00").toString());
    
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
