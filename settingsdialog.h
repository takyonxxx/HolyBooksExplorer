#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFont>
#include <QComboBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QRadioButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QButtonGroup>

class TranslationWorker;
class GoogleTranslateWorker;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    
    void setCurrentFont(const QFont &font);
    QFont selectedFont() const;
    
    void setCurrentLanguage(const QString &lang);
    QString selectedLanguage() const;
    
    void setShowArabic(bool show);
    bool showArabic() const;
    
    void setShowLatin(bool show);
    bool showLatin() const;
    
    void setHighlightColor(const QColor &color);
    QColor highlightColor() const;

private slots:
    void onFontSizeChanged(int size);
    void onFontFamilyChanged(const QString &family);
    void onHighlightColorClicked();
    
    // Translation slots
    void onStartTranslation();
    void onCancelTranslation();
    void onTranslationProgress(int current, int total);
    void onTranslationComplete();
    void onTranslationError(const QString &error);
    void onTranslationLog(const QString &message);
    void onTranslationServiceChanged();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupTranslationUi();
    void loadSettings();
    void saveSettings();
    void loadSurahList();
    
    Ui::SettingsDialog *ui;
    QFont m_font;
    QString m_language;
    bool m_showArabic;
    bool m_showLatin;
    QColor m_highlightColor;
    
    // Translation UI elements
    QGroupBox *m_translationGroup;
    QComboBox *m_serviceCombo;     // Yeni: Servis seçimi (Claude API / Google Translate)
    QLineEdit *m_apiKeyEdit;
    QComboBox *m_surahCombo;
    QRadioButton *m_mealRadio;
    QRadioButton *m_wordRadio;
    QPushButton *m_startButton;
    QPushButton *m_cancelButton;
    QProgressBar *m_progressBar;
    QTextEdit *m_logEdit;
    
    TranslationWorker *m_translationWorker;
    GoogleTranslateWorker *m_googleTranslateWorker;  // Yeni: Google Translate worker
};

#endif // SETTINGSDIALOG_H
