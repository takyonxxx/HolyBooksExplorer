#include "wordanalysiswidget.h"
#include <QLabel>
#include <QFrame>

WordAnalysisWidget::WordAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , m_language("tr")
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Title
    QLabel *titleLabel = new QLabel(tr("Word Analysis"), this);
    titleLabel->setStyleSheet("QLabel { background-color: #192841; color: #DCE6F5; padding: 8px; font-weight: bold; }");
    mainLayout->addWidget(titleLabel);
    
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
    
    // Display word meanings
    for (const WordMeaning &meaning : m_meanings) {
        QFrame *wordFrame = new QFrame(this);
        wordFrame->setStyleSheet("QFrame { background-color: #192841; border-radius: 4px; padding: 8px; }");
        
        QVBoxLayout *wordLayout = new QVBoxLayout(wordFrame);
        wordLayout->setContentsMargins(8, 8, 8, 8);
        wordLayout->setSpacing(4);
        
        // Latin text (Arabic pronunciation)
        if (!meaning.latin.isEmpty()) {
            QLabel *latinLabel = new QLabel(meaning.latin, wordFrame);
            latinLabel->setStyleSheet("QLabel { color: #FFA500; font-weight: bold; }");
            latinLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                QFont labelFont = m_currentFont;
                labelFont.setBold(true);
                latinLabel->setFont(labelFont);
            }
            wordLayout->addWidget(latinLabel);
        }
        
        // Turkish meaning
        if (!meaning.turkish.isEmpty()) {
            QLabel *turkishLabel = new QLabel(tr("Turkish: %1").arg(meaning.turkish), wordFrame);
            turkishLabel->setStyleSheet("QLabel { color: #DCE6F5; }");
            turkishLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                turkishLabel->setFont(m_currentFont);
            }
            wordLayout->addWidget(turkishLabel);
        }
        
        // English meaning
        if (!meaning.english.isEmpty()) {
            QLabel *englishLabel = new QLabel(tr("English: %1").arg(meaning.english), wordFrame);
            englishLabel->setStyleSheet("QLabel { color: #A0C0FF; }");
            englishLabel->setWordWrap(true);
            if (m_currentFont.pointSize() > 0) {
                englishLabel->setFont(m_currentFont);
            }
            wordLayout->addWidget(englishLabel);
        }
        
        m_layout->insertWidget(m_layout->count() - 1, wordFrame);
    }
}
