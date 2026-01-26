#include "wordanalysiswidget.h"
#include <QLabel>
#include <QFrame>
#include <QEvent>

WordAnalysisWidget::WordAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , m_language("tr")
    , m_titleLabel(nullptr)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Title
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("QLabel { background-color: #192841; color: #DCE6F5; padding: 8px; font-weight: bold; font-size: 16px; }");
    mainLayout->addWidget(m_titleLabel);

    // Scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #0F192D; }");

    // Content widget
    m_contentWidget = new QWidget();
    m_layout = new QVBoxLayout(m_contentWidget);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(8);
    m_layout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    setStyleSheet("WordAnalysisWidget { background-color: #0F192D; }");

    retranslateUi();
}

void WordAnalysisWidget::setWordMeanings(const QList<WordMeaning> &meanings)
{
    m_meanings = meanings;
    updateDisplay();
}

void WordAnalysisWidget::setLanguage(const QString &language)
{
    m_language = language;
    updateDisplay();
}

void WordAnalysisWidget::clear()
{
    m_meanings.clear();
    updateDisplay();
}

void WordAnalysisWidget::setFont(const QFont &font)
{
    m_currentFont = font;
    updateDisplay();
}

void WordAnalysisWidget::updateDisplay()
{
    // Clear existing widgets
    while (m_layout->count() > 1) {
        QLayoutItem *item = m_layout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (m_meanings.isEmpty()) {
        QLabel *emptyLabel = new QLabel(tr("No word analysis available"), this);
        emptyLabel->setStyleSheet("QLabel { color: #7F8C9F; font-style: italic; }");
        emptyLabel->setWordWrap(true);
        m_layout->insertWidget(0, emptyLabel);
        return;
    }

    // Display word meanings - always show both Turkish and English
    for (const WordMeaning &meaning : m_meanings) {
        QFrame *wordFrame = new QFrame(this);
        wordFrame->setStyleSheet("QFrame { background-color: #192841; border-radius: 2px; padding: 3px; }");

        QVBoxLayout *wordLayout = new QVBoxLayout(wordFrame);
        wordLayout->setContentsMargins(0, 0, 0, 0);
        wordLayout->setSpacing(0);

        // Latin text (Arabic pronunciation)
        if (!meaning.latin.isEmpty()) {
            QLabel *latinLabel = new QLabel(meaning.latin, wordFrame);
            latinLabel->setStyleSheet("QLabel { color: #FFA500; font-weight: bold; }");
            latinLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                QFont labelFont = m_currentFont;
                labelFont.setBold(true);
                labelFont.setPointSize(m_currentFont.pointSize() - 1);  // 1 punto küçük
                latinLabel->setFont(labelFont);
            }
            wordLayout->addWidget(latinLabel);
        }

        // Turkish meaning - always show with "Türkçe:" label
        if (!meaning.turkish.isEmpty()) {
            QLabel *turkishLabel = new QLabel(QString("Türkçe: %1").arg(meaning.turkish), wordFrame);
            turkishLabel->setStyleSheet("QLabel { color: #DCE6F5; }");
            turkishLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                QFont labelFont = m_currentFont;
                labelFont.setPointSize(m_currentFont.pointSize() - 1);  // 1 punto küçük
                turkishLabel->setFont(labelFont);
            }
            wordLayout->addWidget(turkishLabel);
        }

        // English meaning - always show with "English:" label
        if (!meaning.english.isEmpty()) {
            QLabel *englishLabel = new QLabel(QString("English: %1").arg(meaning.english), wordFrame);
            englishLabel->setStyleSheet("QLabel { color: #A0C0FF; }");
            englishLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                QFont labelFont = m_currentFont;
                labelFont.setPointSize(m_currentFont.pointSize() - 1);  // 1 punto küçük
                englishLabel->setFont(labelFont);
            }
            wordLayout->addWidget(englishLabel);
        }

        m_layout->insertWidget(m_layout->count() - 1, wordFrame);
    }
}

void WordAnalysisWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void WordAnalysisWidget::retranslateUi()
{
    if (m_titleLabel) {
        m_titleLabel->setText(tr("Word Meanings"));
    }
}
