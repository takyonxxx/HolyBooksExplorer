#include "wordanalysiswidget.h"
#include <QHeaderView>

WordAnalysisWidget::WordAnalysisWidget(QWidget *parent)
    : QWidget(parent)
    , m_language("tr")
{
    setupUi();
}

void WordAnalysisWidget::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    
    // Title
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #90CAF9; padding: 5px;");
    m_layout->addWidget(m_titleLabel);
    
    // Table - koyu tema
    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(
        "QTableWidget { "
        "   background-color: #2D2D2D; "
        "   alternate-background-color: #353535; "
        "   gridline-color: #424242; "
        "   border: 1px solid #424242; "
        "   border-radius: 4px; "
        "   color: #E0E0E0; "
        "} "
        "QTableWidget::item { "
        "   padding: 4px; "
        "} "
        "QTableWidget::item:selected { "
        "   background-color: #1565C0; "
        "   color: white; "
        "} "
        "QHeaderView::section { "
        "   background-color: #3D3D3D; "
        "   color: #90CAF9; "
        "   padding: 6px; "
        "   border: none; "
        "   border-bottom: 1px solid #424242; "
        "   font-weight: bold; "
        "}"
        "QScrollBar:vertical { "
        "   background-color: #2D2D2D; "
        "   width: 10px; "
        "} "
        "QScrollBar::handle:vertical { "
        "   background-color: #4D4D4D; "
        "   border-radius: 4px; "
        "   min-height: 20px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "   background-color: #5D5D5D; "
        "} "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "   height: 0px; "
        "}"
    );
    
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    
    m_layout->addWidget(m_table);
    
    updateHeaders();
    
    // Connect click signal
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int col) {
        Q_UNUSED(col);
        QTableWidgetItem *item = m_table->item(row, 0);
        if (item) {
            emit wordClicked(item->text());
        }
    });
}

void WordAnalysisWidget::updateHeaders()
{
    if (m_language == "tr") {
        m_titleLabel->setText(tr("📚 Kelime Anlamları"));
        m_table->setHorizontalHeaderLabels({tr("Latince"), tr("Türkçe"), tr("İngilizce")});
    } else {
        m_titleLabel->setText(tr("📚 Word Meanings"));
        m_table->setHorizontalHeaderLabels({tr("Latin"), tr("Turkish"), tr("English")});
    }
}

void WordAnalysisWidget::setWordMeanings(const QList<WordMeaning> &meanings)
{
    m_table->setRowCount(0);
    m_table->setRowCount(meanings.size());
    
    for (int i = 0; i < meanings.size(); ++i) {
        const WordMeaning &wm = meanings[i];
        
        // Latin
        QTableWidgetItem *latinItem = new QTableWidgetItem(wm.latin);
        latinItem->setFlags(latinItem->flags() & ~Qt::ItemIsEditable);
        latinItem->setForeground(QColor("#B0BEC5"));
        m_table->setItem(i, 0, latinItem);
        
        // Turkish
        QTableWidgetItem *turkishItem = new QTableWidgetItem(wm.turkish);
        turkishItem->setFlags(turkishItem->flags() & ~Qt::ItemIsEditable);
        turkishItem->setForeground(QColor("#81C784"));
        m_table->setItem(i, 1, turkishItem);
        
        // English
        QTableWidgetItem *englishItem = new QTableWidgetItem(wm.english);
        englishItem->setFlags(englishItem->flags() & ~Qt::ItemIsEditable);
        englishItem->setForeground(QColor("#64B5F6"));
        m_table->setItem(i, 2, englishItem);
    }
    
    m_table->resizeRowsToContents();
}

void WordAnalysisWidget::setLanguage(const QString &lang)
{
    m_language = lang;
    updateHeaders();
}

void WordAnalysisWidget::setFont(const QFont &font)
{
    m_table->setFont(font);
    m_titleLabel->setFont(font);
}

void WordAnalysisWidget::clear()
{
    m_table->setRowCount(0);
}
