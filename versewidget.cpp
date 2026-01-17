#include "versewidget.h"
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QContextMenuEvent>

VerseWidget::VerseWidget(QWidget *parent)
    : QWidget(parent)
    , m_showDetails(true)
    , m_language("tr")
    , m_highlighter(nullptr)
    , m_highlighterEn(nullptr)
{
    setupUi();
}

void VerseWidget::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 5, 8, 5);
    m_layout->setSpacing(2);
    
    // Header label
    m_headerLabel = new QLabel(this);
    m_headerLabel->setStyleSheet("font-weight: bold; color: #90CAF9; padding: 1px 0px;");
    m_layout->addWidget(m_headerLabel);
    
    // Main text (Turkish) - koyu tema
    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setFrameStyle(QFrame::NoFrame);
    m_textEdit->setStyleSheet("background-color: transparent; color: #E0E0E0; padding: 1px;");
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_textEdit);
    
    // English text - koyu tema
    m_textEnEdit = new QTextEdit(this);
    m_textEnEdit->setReadOnly(true);
    m_textEnEdit->setFrameStyle(QFrame::NoFrame);
    m_textEnEdit->setStyleSheet("background-color: #1E3A5F; color: #81D4FA; padding: 3px; border-radius: 2px;");
    m_textEnEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEnEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEnEdit->hide();
    m_layout->addWidget(m_textEnEdit);
    
    // Arabic text (for Quran) - koyu tema
    m_arabicEdit = new QTextEdit(this);
    m_arabicEdit->setReadOnly(true);
    m_arabicEdit->setFrameStyle(QFrame::NoFrame);
    m_arabicEdit->setStyleSheet("background-color: #1B5E20; color: #A5D6A7; padding: 3px; border-radius: 2px;");
    m_arabicEdit->setAlignment(Qt::AlignRight);
    m_arabicEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_arabicEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_arabicEdit->setLayoutDirection(Qt::RightToLeft);
    m_arabicEdit->hide();
    m_layout->addWidget(m_arabicEdit);
    
    // Latin transliteration (for Quran) - koyu tema
    m_latinEdit = new QTextEdit(this);
    m_latinEdit->setReadOnly(true);
    m_latinEdit->setFrameStyle(QFrame::NoFrame);
    m_latinEdit->setStyleSheet("background-color: #4A4A00; color: #FFEB3B; padding: 3px; border-radius: 2px;");
    m_latinEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_latinEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_latinEdit->hide();
    m_layout->addWidget(m_latinEdit);
    
    // Create highlighters
    m_highlighter = new SearchHighlighter(m_textEdit->document());
    m_highlighterEn = new SearchHighlighter(m_textEnEdit->document());
    
    // Style the widget - koyu tema
    setStyleSheet("VerseWidget { background-color: #2D2D2D; border: 1px solid #424242; border-radius: 3px; margin: 1px; }");
}

void VerseWidget::setVerse(const Verse &verse, bool showDetails)
{
    m_verse = verse;
    m_showDetails = showDetails;
    updateDisplay();
}

void VerseWidget::setLanguage(const QString &lang)
{
    m_language = lang;
    updateDisplay();
}

void VerseWidget::updateDisplay()
{
    // Set header
    QString header;
    if (!m_verse.bookName.isEmpty()) {
        header = QString("%1 - %2:%3").arg(m_verse.bookName).arg(m_verse.suraNo).arg(m_verse.verseNo);
    } else {
        header = QString("%1 %2:%3").arg(m_verse.suraName).arg(m_verse.suraNo).arg(m_verse.verseNo);
    }
    m_headerLabel->setText(header);
    
    // Set main text based on language
    QString displayText = m_verse.text;
    if (m_language == "en" && !m_verse.textEn.isEmpty()) {
        displayText = m_verse.textEn;
    }
    m_textEdit->setPlainText(displayText);
    
    // Calculate compact height
    QFontMetrics fm(m_textEdit->font());
    int lineHeight = fm.lineSpacing();
    int textWidth = m_textEdit->width() - 8;
    if (textWidth < 100) textWidth = 400;
    
    QRect textRect = fm.boundingRect(QRect(0, 0, textWidth, 1000), Qt::TextWordWrap, displayText);
    int textHeight = textRect.height() + 4;
    m_textEdit->setFixedHeight(qMax(textHeight, lineHeight + 4));
    
    // Show English translation if available and language is Turkish
    if (m_showDetails && !m_verse.textEn.isEmpty() && m_language == "tr") {
        m_textEnEdit->setPlainText(m_verse.textEn);
        m_textEnEdit->show();
        
        QFontMetrics fmEn(m_textEnEdit->font());
        QRect enRect = fmEn.boundingRect(QRect(0, 0, textWidth, 1000), Qt::TextWordWrap, m_verse.textEn);
        int enHeight = enRect.height() + 4;
        m_textEnEdit->setFixedHeight(qMax(enHeight, fmEn.lineSpacing() + 4));
    } else {
        m_textEnEdit->hide();
    }
    
    // Show Arabic for Quran
    if (m_showDetails && !m_verse.arabic.isEmpty()) {
        m_arabicEdit->setPlainText(m_verse.arabic);
        m_arabicEdit->show();
        
        QFontMetrics fmArabic(m_arabicEdit->font());
        QRect arabicRect = fmArabic.boundingRect(QRect(0, 0, textWidth, 1000), Qt::TextWordWrap, m_verse.arabic);
        int arabicHeight = arabicRect.height() + 4;
        m_arabicEdit->setFixedHeight(qMax(arabicHeight, fmArabic.lineSpacing() + 4));
    } else {
        m_arabicEdit->hide();
    }
    
    // Show Latin for Quran
    if (m_showDetails && !m_verse.latin.isEmpty()) {
        m_latinEdit->setPlainText(m_verse.latin);
        m_latinEdit->show();
        
        QFontMetrics fmLatin(m_latinEdit->font());
        QRect latinRect = fmLatin.boundingRect(QRect(0, 0, textWidth, 1000), Qt::TextWordWrap, m_verse.latin);
        int latinHeight = latinRect.height() + 4;
        m_latinEdit->setFixedHeight(qMax(latinHeight, fmLatin.lineSpacing() + 4));
    } else {
        m_latinEdit->hide();
    }
    
    // Re-apply highlight if set
    if (!m_searchText.isEmpty()) {
        m_highlighter->setSearchText(m_searchText);
        m_highlighterEn->setSearchText(m_searchText);
    }
}

void VerseWidget::setSearchHighlight(const QString &searchText)
{
    m_searchText = searchText;
    m_highlighter->setSearchText(searchText);
    m_highlighterEn->setSearchText(searchText);
}

void VerseWidget::setFont(const QFont &font)
{
    // Aynı fontu tüm alanlara uygula
    m_textEdit->setFont(font);
    m_textEnEdit->setFont(font);
    m_latinEdit->setFont(font);
    
    // Arapça için biraz daha büyük font
    QFont arabicFont = font;
    arabicFont.setPointSize(font.pointSize() + 2);
    m_arabicEdit->setFont(arabicFont);
    
    // Header için bold
    QFont headerFont = font;
    headerFont.setBold(true);
    m_headerLabel->setFont(headerFont);
    
    if (m_verse.suraNo > 0) {
        updateDisplay();
    }
}

void VerseWidget::clear()
{
    m_verse = Verse();
    m_headerLabel->clear();
    m_textEdit->clear();
    m_textEnEdit->clear();
    m_textEnEdit->hide();
    m_arabicEdit->clear();
    m_arabicEdit->hide();
    m_latinEdit->clear();
    m_latinEdit->hide();
}

void VerseWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit verseClicked(m_verse);
    }
    QWidget::mousePressEvent(event);
}

void VerseWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #3D3D3D; color: #E0E0E0; border: 1px solid #555; }"
                       "QMenu::item:selected { background-color: #505050; }");
    
    QAction *copyTextAction = menu.addAction(tr("Copy Text"));
    QAction *copyArabicAction = nullptr;
    QAction *copyLatinAction = nullptr;
    QAction *copyAllAction = menu.addAction(tr("Copy All"));
    
    if (!m_verse.arabic.isEmpty()) {
        copyArabicAction = menu.addAction(tr("Copy Arabic"));
    }
    if (!m_verse.latin.isEmpty()) {
        copyLatinAction = menu.addAction(tr("Copy Latin"));
    }
    
    QAction *selected = menu.exec(event->globalPos());
    
    if (selected == copyTextAction) {
        QApplication::clipboard()->setText(m_verse.text);
    } else if (selected == copyArabicAction && copyArabicAction) {
        QApplication::clipboard()->setText(m_verse.arabic);
    } else if (selected == copyLatinAction && copyLatinAction) {
        QApplication::clipboard()->setText(m_verse.latin);
    } else if (selected == copyAllAction) {
        QString all = QString("%1:%2\n%3").arg(m_verse.suraNo).arg(m_verse.verseNo).arg(m_verse.text);
        if (!m_verse.arabic.isEmpty()) {
            all += "\n" + m_verse.arabic;
        }
        if (!m_verse.latin.isEmpty()) {
            all += "\n" + m_verse.latin;
        }
        QApplication::clipboard()->setText(all);
    }
}
