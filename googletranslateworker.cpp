#include "googletranslateworker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

GoogleTranslateWorker::GoogleTranslateWorker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_running(false)
    , m_cancelled(false)
    , m_currentIndex(0)
    , m_totalItems(0)
    , m_currentSureIndex(0)
    , m_processTimer(new QTimer(this))
    , m_requestCount(0)
    , m_rateLimitTimer(new QTimer(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &GoogleTranslateWorker::onNetworkReply);
    
    m_processTimer->setSingleShot(true);
    connect(m_processTimer, &QTimer::timeout, this, &GoogleTranslateWorker::processNextItem);
    
    // Rate limit reset timer (her dakika resetle)
    m_rateLimitTimer->setInterval(60000); // 1 dakika
    connect(m_rateLimitTimer, &QTimer::timeout, this, [this]() {
        m_requestCount = 0;
        emit logMessage(tr("Rate limit counter reset"));
    });
}

GoogleTranslateWorker::~GoogleTranslateWorker()
{
}

void GoogleTranslateWorker::setDatabase(const QString &dbPath)
{
    m_dbPath = dbPath;
}

void GoogleTranslateWorker::startTranslation(int sureNo, TranslationType type)
{
    if (m_running) {
        emit translationError(tr("Translation already in progress"));
        return;
    }
    
    m_running = true;
    m_cancelled = false;
    m_currentType = type;
    m_currentIndex = 0;
    m_queue.clear();
    m_sureList.clear();
    m_currentSureIndex = 0;
    m_requestCount = 0;
    
    // Rate limit timer'ı başlat
    m_rateLimitTimer->start();
    
    // If sureNo is -1, process all surahs (1-114)
    if (sureNo == -1) {
        for (int i = 1; i <= 114; i++) {
            m_sureList.append(i);
        }
        emit logMessage(tr("Starting translation for all surahs (1-114) using Google Translate"));
        processNextSure();
    } else {
        // Process single surah
        m_currentSureNo = sureNo;
        m_sureList.append(sureNo);
        
        if (type == TranslateMeal) {
            translateMeal(sureNo);
        } else {
            translateWord(sureNo);
        }
    }
}

void GoogleTranslateWorker::cancelTranslation()
{
    m_cancelled = true;
    m_running = false;
    m_processTimer->stop();
    m_rateLimitTimer->stop();
    emit logMessage(tr("Translation cancelled"));
}

void GoogleTranslateWorker::processNextSure()
{
    if (m_cancelled || m_currentSureIndex >= m_sureList.size()) {
        m_running = false;
        m_rateLimitTimer->stop();
        if (!m_cancelled) {
            emit logMessage(tr("All surahs translation completed!"));
            emit translationComplete();
        }
        return;
    }
    
    m_currentSureNo = m_sureList[m_currentSureIndex];
    m_queue.clear();
    m_currentIndex = 0;
    
    emit logMessage(tr("Processing Surah %1 of %2 (Surah #%3)")
                    .arg(m_currentSureIndex + 1)
                    .arg(m_sureList.size())
                    .arg(m_currentSureNo));
    
    if (m_currentType == TranslateMeal) {
        translateMeal(m_currentSureNo);
    } else {
        translateWord(m_currentSureNo);
    }
}

void GoogleTranslateWorker::translateMeal(int sureNo)
{
    // Find an open database connection
    QSqlDatabase db;
    QStringList connections = QSqlDatabase::connectionNames();
    
    for (const QString &connName : connections) {
        QSqlDatabase tempDb = QSqlDatabase::database(connName);
        if (tempDb.isOpen()) {
            db = tempDb;
            break;
        }
    }
    
    if (!db.isValid() || !db.isOpen()) {
        emit translationError(tr("Database not open"));
        m_running = false;
        return;
    }
    
    // Get all verses for this surah that need translation (meal_en is empty or NULL)
    QSqlQuery query(db);
    query.prepare("SELECT ayet, meal_saf FROM tbl_kuran_meal WHERE sureno = ? AND (meal_en IS NULL OR meal_en = '') AND meal_saf IS NOT NULL AND meal_saf != '' ORDER BY ayet");
    query.addBindValue(sureNo);
    
    if (!query.exec()) {
        emit translationError(tr("Query error: %1").arg(query.lastError().text()));
        m_running = false;
        return;
    }
    
    while (query.next()) {
        TranslationItem item;
        item.sureno = sureNo;
        item.ayet = query.value(0).toInt();
        item.id = item.ayet;
        item.text = query.value(1).toString().trimmed();
        
        if (!item.text.isEmpty()) {
            m_queue.append(item);
        }
    }
    
    m_totalItems = m_queue.size();
    
    if (m_totalItems == 0) {
        emit logMessage(tr("No items to translate for Surah %1 (already translated)").arg(sureNo));
        
        // Move to next surah if processing all
        if (m_sureList.size() > 1) {
            m_currentSureIndex++;
            QTimer::singleShot(100, this, &GoogleTranslateWorker::processNextSure);
        } else {
            m_running = false;
            m_rateLimitTimer->stop();
            emit translationComplete();
        }
        return;
    }
    
    emit logMessage(tr("Starting meal translation for Surah %1: %2 verses").arg(sureNo).arg(m_totalItems));
    emit progressChanged(0, m_totalItems);
    
    // Start processing
    processNextItem();
}

void GoogleTranslateWorker::translateWord(int sureNo)
{
    // Find an open database connection
    QSqlDatabase db;
    QStringList connections = QSqlDatabase::connectionNames();
    
    for (const QString &connName : connections) {
        QSqlDatabase tempDb = QSqlDatabase::database(connName);
        if (tempDb.isOpen()) {
            db = tempDb;
            break;
        }
    }
    
    if (!db.isValid() || !db.isOpen()) {
        emit translationError(tr("Database not open"));
        m_running = false;
        return;
    }
    
    // Get all words for this surah that need translation (english is empty or NULL)
    QSqlQuery query(db);
    query.prepare("SELECT rowid, sureno, ayet, turkce FROM tbl_kuran_kelime WHERE sureno = ? AND (english IS NULL OR english = '') AND turkce IS NOT NULL AND turkce != '' ORDER BY ayet");
    query.addBindValue(sureNo);
    
    if (!query.exec()) {
        emit translationError(tr("Query error: %1").arg(query.lastError().text()));
        m_running = false;
        return;
    }
    
    while (query.next()) {
        TranslationItem item;
        item.id = query.value(0).toInt();  // rowid
        item.sureno = query.value(1).toInt();
        item.ayet = query.value(2).toInt();
        item.text = query.value(3).toString().trimmed();
        
        if (!item.text.isEmpty()) {
            m_queue.append(item);
        }
    }
    
    m_totalItems = m_queue.size();
    
    if (m_totalItems == 0) {
        emit logMessage(tr("No items to translate for Surah %1 (already translated)").arg(sureNo));
        
        // Move to next surah if processing all
        if (m_sureList.size() > 1) {
            m_currentSureIndex++;
            QTimer::singleShot(100, this, &GoogleTranslateWorker::processNextSure);
        } else {
            m_running = false;
            m_rateLimitTimer->stop();
            emit translationComplete();
        }
        return;
    }
    
    emit logMessage(tr("Starting word translation for Surah %1: %2 words").arg(sureNo).arg(m_totalItems));
    emit progressChanged(0, m_totalItems);
    
    // Start processing
    processNextItem();
}

void GoogleTranslateWorker::processNextItem()
{
    if (m_cancelled || m_currentIndex >= m_queue.size()) {
        // Current surah is complete
        if (!m_cancelled) {
            emit logMessage(tr("Surah %1 translation completed").arg(m_currentSureNo));
            
            // Move to next surah if processing multiple surahs
            if (m_sureList.size() > 1) {
                m_currentSureIndex++;
                QTimer::singleShot(1000, this, &GoogleTranslateWorker::processNextSure);
            } else {
                m_running = false;
                m_rateLimitTimer->stop();
                emit translationComplete();
            }
        } else {
            m_running = false;
            m_rateLimitTimer->stop();
        }
        return;
    }
    
    // Rate limiting kontrolü
    if (m_requestCount >= MAX_REQUESTS_PER_MINUTE) {
        emit logMessage(tr("Rate limit reached (%1 requests/min), waiting 60 seconds...")
                       .arg(MAX_REQUESTS_PER_MINUTE));
        // 60 saniye bekle
        m_processTimer->start(60000);
        m_requestCount = 0;
        return;
    }
    
    const TranslationItem &item = m_queue[m_currentIndex];
    sendTranslationRequest(item.text, m_currentIndex);
}

void GoogleTranslateWorker::sendTranslationRequest(const QString &text, int recordId)
{
    // Google Translate API endpoint (unofficial)
    QUrl url("https://translate.googleapis.com/translate_a/single");
    
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "tr");  // Source: Turkish
    query.addQueryItem("tl", "en");  // Target: English
    query.addQueryItem("dt", "t");   // Return translation
    query.addQueryItem("q", text);
    
    url.setQuery(query);
    
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    // Store index in request for later retrieval
    request.setAttribute(QNetworkRequest::User, recordId);
    
    emit logMessage(tr("Sending request for: %1...").arg(text.left(40)));
    
    m_networkManager->get(request);
    m_requestCount++;
}

QString GoogleTranslateWorker::extractTranslation(const QString &responseData)
{
    // Google Translate API response format:
    // [[["translated text","original text",null,null,3]],null,"tr",null,null,null,null,[]]
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(responseData.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit logMessage(tr("JSON parse error: %1").arg(error.errorString()));
        return QString();
    }
    
    if (!doc.isArray()) {
        emit logMessage(tr("Unexpected response format"));
        return QString();
    }
    
    QJsonArray mainArray = doc.array();
    if (mainArray.isEmpty() || !mainArray[0].isArray()) {
        emit logMessage(tr("Empty or invalid translation array"));
        return QString();
    }
    
    QJsonArray translationArray = mainArray[0].toArray();
    QString result;
    
    // Her satırı birleştir
    for (const QJsonValue &val : translationArray) {
        if (val.isArray()) {
            QJsonArray lineArray = val.toArray();
            if (!lineArray.isEmpty() && lineArray[0].isString()) {
                result += lineArray[0].toString();
            }
        }
    }
    
    return result.trimmed();
}

void GoogleTranslateWorker::onNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();
    
    if (m_cancelled) {
        return;
    }
    
    int itemIndex = reply->request().attribute(QNetworkRequest::User).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = tr("Network error: %1").arg(reply->errorString());
        emit logMessage(errorMsg);
        
        // Skip this item and continue
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(4000);  // 4 saniye bekle
        return;
    }
    
    QByteArray responseData = reply->readAll();
    QString translation = extractTranslation(QString::fromUtf8(responseData));
    
    if (translation.isEmpty()) {
        emit logMessage(tr("Empty translation received for item %1").arg(itemIndex));
        emit logMessage(tr("Raw response: %1").arg(QString::fromUtf8(responseData).left(200)));
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(2000);
        return;
    }
    
    // Save translation to database
    const TranslationItem &item = m_queue[itemIndex];
    
    // Find an open database connection
    QSqlDatabase db;
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString &connName : connections) {
        QSqlDatabase tempDb = QSqlDatabase::database(connName);
        if (tempDb.isOpen()) {
            db = tempDb;
            break;
        }
    }
    
    if (!db.isValid() || !db.isOpen()) {
        emit logMessage(tr("Database connection lost"));
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(1000);
        return;
    }
    
    QSqlQuery updateQuery(db);
    
    if (m_currentType == TranslateMeal) {
        updateQuery.prepare("UPDATE tbl_kuran_meal SET meal_en = ? WHERE sureno = ? AND ayet = ?");
        updateQuery.addBindValue(translation);
        updateQuery.addBindValue(item.sureno);
        updateQuery.addBindValue(item.ayet);
    } else {
        updateQuery.prepare("UPDATE tbl_kuran_kelime SET english = ? WHERE rowid = ?");
        updateQuery.addBindValue(translation);
        updateQuery.addBindValue(item.id);
    }
    
    if (!updateQuery.exec()) {
        emit logMessage(tr("Database update error: %1").arg(updateQuery.lastError().text()));
    } else {
        emit logMessage(tr("Translated [%1:%2]: %3 -> %4")
                       .arg(item.sureno)
                       .arg(item.ayet)
                       .arg(item.text.left(50))
                       .arg(translation.left(50)));
    }
    
    m_currentIndex++;
    emit progressChanged(m_currentIndex, m_totalItems);
    
    // Google'ın rate limit'ine takılmamak için bekleme süresi
    // Her istek arasında 4 saniye bekle (dakikada ~15 istek)
    m_processTimer->start(4000);
}
