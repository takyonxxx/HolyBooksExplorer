#ifndef SEARCHHIGHLIGHTER_H
#define SEARCHHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class SearchHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SearchHighlighter(QTextDocument *parent = nullptr);

    void setSearchText(const QString &text);
    QString searchText() const { return m_searchText; }
    
    void setHighlightColor(const QColor &color);
    QColor highlightColor() const { return m_highlightFormat.background().color(); }

protected:
    void highlightBlock(const QString &text) override;

private:
    QString m_searchText;
    QTextCharFormat m_highlightFormat;
};

#endif // SEARCHHIGHLIGHTER_H
