#include "searchhighlighter.h"

SearchHighlighter::SearchHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Default yellow highlight
    m_highlightFormat.setBackground(QColor(255, 255, 0)); // Yellow
    m_highlightFormat.setForeground(QColor(0, 0, 0));     // Black text
    m_highlightFormat.setFontWeight(QFont::Bold);
}

void SearchHighlighter::setSearchText(const QString &text)
{
    m_searchText = text;
    rehighlight();
}

void SearchHighlighter::setHighlightColor(const QColor &color)
{
    m_highlightFormat.setBackground(color);
    rehighlight();
}

void SearchHighlighter::highlightBlock(const QString &text)
{
    if (m_searchText.isEmpty()) {
        return;
    }

    // Case insensitive search
    QString pattern = QRegularExpression::escape(m_searchText);
    QRegularExpression regex(pattern, QRegularExpression::CaseInsensitiveOption);
    
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_highlightFormat);
    }
}
