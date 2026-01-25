#ifndef WORDANALYSISWIDGET_H
#define WORDANALYSISWIDGET_H

#include <QWidget>
#include <QList>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include "databasemanager.h"

class WordAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WordAnalysisWidget(QWidget *parent = nullptr);
    
    void setWordMeanings(const QList<WordMeaning> &meanings);
    void setLanguage(const QString &language);
    void clear();
    void setFont(const QFont &font);

protected:
    void changeEvent(QEvent *event) override;
    
private:
    void updateDisplay();
    void retranslateUi();
    
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    
    QList<WordMeaning> m_meanings;
    QString m_language;
    QFont m_currentFont;
};

#endif // WORDANALYSISWIDGET_H
