#ifndef WORDANALYSISWIDGET_H
#define WORDANALYSISWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "databasemanager.h"

class WordAnalysisWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WordAnalysisWidget(QWidget *parent = nullptr);
    
    void setWordMeanings(const QList<WordMeaning> &meanings);
    void setLanguage(const QString &lang);
    void setFont(const QFont &font);
    void clear();

signals:
    void wordClicked(const QString &word);

private:
    void setupUi();
    void updateHeaders();
    
    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    QTableWidget *m_table;
    QString m_language;
};

#endif // WORDANALYSISWIDGET_H
