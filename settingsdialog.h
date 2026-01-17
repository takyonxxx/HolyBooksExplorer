#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFont>

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

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    
    Ui::SettingsDialog *ui;
    QFont m_font;
    QString m_language;
    bool m_showArabic;
    bool m_showLatin;
    QColor m_highlightColor;
};

#endif // SETTINGSDIALOG_H
