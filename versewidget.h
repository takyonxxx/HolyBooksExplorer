#ifndef VERSEWIDGET_H
#define VERSEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include "databasemanager.h"
#include "searchhighlighter.h"

class VerseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VerseWidget(QWidget *parent = nullptr);
    
    void setVerse(const Verse &verse, bool showDetails = true);
    void setSearchHighlight(const QString &searchText);
    void setFont(const QFont &font);
    void setLanguage(const QString &lang);
    void clear();
    
    Verse currentVerse() const { return m_verse; }

signals:
    void verseClicked(const Verse &verse);
    void copyRequested(const Verse &verse);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupUi();
    void updateDisplay();
    
    Verse m_verse;
    bool m_showDetails;
    QString m_language;
    
    QVBoxLayout *m_layout;
    QLabel *m_headerLabel;
    QTextEdit *m_textEdit;
    QTextEdit *m_textEnEdit;  // İngilizce metin
    QTextEdit *m_arabicEdit;
    QTextEdit *m_latinEdit;
    
    SearchHighlighter *m_highlighter;
    SearchHighlighter *m_highlighterEn;
    QString m_searchText;
};

#endif // VERSEWIDGET_H
