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
{
    setupUi();
}

void VerseWidget::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(8, 5, 8, 5);
    m_layout->setSpacing(0);

    // Header label
    m_headerLabel = new QLabel(this);
    m_headerLabel->setStyleSheet("font-weight: bold; color: #90CAF9; background-color: #1A2332; padding: 4px 2px; margin: 0px; border-radius: 2px;");
    m_layout->addWidget(m_headerLabel);

    // Main text (Turkish) - koyu tema
    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextFormat(Qt::RichText);
    m_textLabel->setStyleSheet("background-color: #333333; color: #E0E0E0; padding: 2px; margin: 0px; border-radius: 2px;");
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_layout->addWidget(m_textLabel);

    // English text - koyu tema
    m_textEnLabel = new QLabel(this);
    m_textEnLabel->setWordWrap(true);
    m_textEnLabel->setTextFormat(Qt::RichText);
    m_textEnLabel->setStyleSheet("background-color: #1E3A5F; color: #81D4FA; padding: 2px; margin: 0px; border-radius: 2px;");
    m_textEnLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_textEnLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_textEnLabel->hide();
    m_layout->addWidget(m_textEnLabel);

    // Arabic text (for Quran) - koyu tema
    m_arabicLabel = new QLabel(this);
    m_arabicLabel->setWordWrap(true);
    m_arabicLabel->setTextFormat(Qt::PlainText);
    m_arabicLabel->setStyleSheet("background-color: #1B5E20; color: #A5D6A7; padding: 2px; margin: 0px; border-radius: 2px;");
    m_arabicLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    m_arabicLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_arabicLabel->hide();
    m_layout->addWidget(m_arabicLabel);

    // Latin transliteration (for Quran) - koyu tema
    m_latinLabel = new QLabel(this);
    m_latinLabel->setWordWrap(true);
    m_latinLabel->setTextFormat(Qt::PlainText);
    m_latinLabel->setStyleSheet("background-color: #4A4A00; color: #FFEB3B; padding: 2px; margin: 0px; border-radius: 2px;");
    m_latinLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_latinLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_latinLabel->hide();
    m_layout->addWidget(m_latinLabel);

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

    // Set main text - always show Turkish text regardless of UI language
    m_textLabel->setText(highlightText(m_verse.text, m_searchText));

    // Show English translation if available - always show regardless of UI language
    if (m_showDetails && !m_verse.textEn.isEmpty()) {
        m_textEnLabel->setText(highlightText(m_verse.textEn, m_searchText));
        m_textEnLabel->show();
    } else {
        m_textEnLabel->hide();
    }

    // Show Arabic for Quran
    if (m_showDetails && !m_verse.arabic.isEmpty()) {
        m_arabicLabel->setText(m_verse.arabic);
        m_arabicLabel->show();
    } else {
        m_arabicLabel->hide();
    }

    // Show Latin for Quran
    if (m_showDetails && !m_verse.latin.isEmpty()) {
        m_latinLabel->setText(m_verse.latin);
        m_latinLabel->show();
    } else {
        m_latinLabel->hide();
    }
}

void VerseWidget::setSearchHighlight(const QString &searchText)
{
    m_searchText = searchText;
    updateDisplay();
}

QString VerseWidget::highlightText(const QString &text, const QString &searchText)
{
    if (searchText.isEmpty()) {
        return text.toHtmlEscaped();
    }

    QString result = text;
    int pos = 0;
    QString highlighted = result;
    QString searchLower = searchText.toLower();

    // HTML escape first
    result = result.toHtmlEscaped();
    highlighted = "";

    pos = 0;
    while (pos < text.length()) {
        int foundPos = text.toLower().indexOf(searchLower, pos);
        if (foundPos == -1) {
            highlighted += text.mid(pos).toHtmlEscaped();
            break;
        }

        highlighted += text.mid(pos, foundPos - pos).toHtmlEscaped();
        highlighted += "<span style='background-color: #FFFF00; color: #000000;'>";
        highlighted += text.mid(foundPos, searchText.length()).toHtmlEscaped();
        highlighted += "</span>";
        pos = foundPos + searchText.length();
    }

    return highlighted;
}

void VerseWidget::setFont(const QFont &font)
{
    // Aynı fontu tüm alanlara uygula
    m_textLabel->setFont(font);
    m_textEnLabel->setFont(font);
    m_latinLabel->setFont(font);

    // Arapça için biraz daha büyük font
    QFont arabicFont = font;
    arabicFont.setPointSize(font.pointSize() + 2);
    m_arabicLabel->setFont(arabicFont);

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
    m_textLabel->clear();
    m_textEnLabel->clear();
    m_textEnLabel->hide();
    m_arabicLabel->clear();
    m_arabicLabel->hide();
    m_latinLabel->clear();
    m_latinLabel->hide();
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
