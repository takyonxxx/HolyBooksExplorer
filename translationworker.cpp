#include "translationworker.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>
#include <QDebug>

TranslationWorker::TranslationWorker(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_running(false)
    , m_cancelled(false)
    , m_currentIndex(0)
    , m_totalItems(0)
    , m_currentSureIndex(0)
    , m_processTimer(new QTimer(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TranslationWorker::onNetworkReply);
    
    // Handle SSL errors
    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
            this, [this](QNetworkReply *reply, const QList<QSslError> &errors) {
        QStringList errorStrings;
        for (const QSslError &error : errors) {
            errorStrings << error.errorString();
        }
        emit logMessage(tr("SSL Errors: %1").arg(errorStrings.join(", ")));
        // Ignore SSL errors for now (not recommended for production)
        reply->ignoreSslErrors();
    });
    
    m_processTimer->setSingleShot(true);
    connect(m_processTimer, &QTimer::timeout, this, &TranslationWorker::processNextItem);
}

TranslationWorker::~TranslationWorker()
{
}

void TranslationWorker::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void TranslationWorker::setDatabase(const QString &dbPath)
{
    m_dbPath = dbPath;
}

void TranslationWorker::startTranslation(int sureNo, TranslationType type)
{
    if (m_running) {
        emit translationError(tr("Translation already in progress"));
        return;
    }
    
    if (m_apiKey.isEmpty()) {
        emit translationError(tr("API Key is not set"));
        return;
    }
    
    m_running = true;
    m_cancelled = false;
    m_currentType = type;
    m_currentIndex = 0;
    m_queue.clear();
    m_sureList.clear();
    m_currentSureIndex = 0;
    
    // If sureNo is -1, process all surahs (1-114)
    if (sureNo == -1) {
        for (int i = 1; i <= 114; i++) {
            m_sureList.append(i);
        }
        emit logMessage(tr("Starting translation for all surahs (1-114)"));
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

void TranslationWorker::cancelTranslation()
{
    m_cancelled = true;
    m_running = false;
    m_processTimer->stop();
    emit logMessage(tr("Translation cancelled"));
}

void TranslationWorker::processNextSure()
{
    if (m_cancelled || m_currentSureIndex >= m_sureList.size()) {
        m_running = false;
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

void TranslationWorker::translateMeal(int sureNo)
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
            QTimer::singleShot(100, this, &TranslationWorker::processNextSure);
        } else {
            m_running = false;
            emit translationComplete();
        }
        return;
    }
    
    emit logMessage(tr("Starting meal translation for Surah %1: %2 verses").arg(sureNo).arg(m_totalItems));
    emit progressChanged(0, m_totalItems);
    
    // Start processing
    processNextItem();
}

void TranslationWorker::translateWord(int sureNo)
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
            QTimer::singleShot(100, this, &TranslationWorker::processNextSure);
        } else {
            m_running = false;
            emit translationComplete();
        }
        return;
    }
    
    emit logMessage(tr("Starting word translation for Surah %1: %2 words").arg(sureNo).arg(m_totalItems));
    emit progressChanged(0, m_totalItems);
    
    // Start processing
    processNextItem();
}

void TranslationWorker::processNextItem()
{
    if (m_cancelled || m_currentIndex >= m_queue.size()) {
        // Current surah is complete
        if (!m_cancelled) {
            emit logMessage(tr("Surah %1 translation completed").arg(m_currentSureNo));
            
            // Move to next surah if processing multiple surahs
            if (m_sureList.size() > 1) {
                m_currentSureIndex++;
                QTimer::singleShot(1000, this, &TranslationWorker::processNextSure);
            } else {
                m_running = false;
                emit translationComplete();
            }
        } else {
            m_running = false;
        }
        return;
    }
    
    const TranslationItem &item = m_queue[m_currentIndex];
    sendTranslationRequest(item.text, m_currentIndex);
}

void TranslationWorker::sendTranslationRequest(const QString &text, int recordId)
{
    QUrl url("https://api.anthropic.com/v1/messages");
    QNetworkRequest request(url);
    
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", m_apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    
    // Enable SSL error handling
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    
    QString systemPrompt;
    if (m_currentType == TranslateMeal) {
        systemPrompt = "You are a professional translator specializing in religious texts. "
                       "Translate the given Turkish Quran verse interpretation (meal) to English. "
                       "Provide only the translation, no explanations or additional text. "
                       "Maintain the reverent and formal tone appropriate for religious scripture.";
    } else {
        systemPrompt = "You are a professional translator. "
                       "Translate the given Turkish word or phrase to English. "
                       "This is a word/phrase from Quran word analysis. "
                       "Provide only the direct English translation, no explanations. "
                       "If there are multiple meanings, separate them with commas.";
    }
    
    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = text;
    
    QJsonArray messagesArray;
    messagesArray.append(messageObj);
    
    QJsonObject requestBody;
    requestBody["model"] = "claude-sonnet-4-20250514";
    requestBody["max_tokens"] = 1024;
    requestBody["system"] = systemPrompt;
    requestBody["messages"] = messagesArray;
    
    QJsonDocument doc(requestBody);
    QByteArray data = doc.toJson();
    
    // Store index in request for later retrieval
    request.setAttribute(QNetworkRequest::User, recordId);
    
    emit logMessage(tr("Sending request for: %1...").arg(text.left(40)));
    
    m_networkManager->post(request, data);
}

void TranslationWorker::onNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();
    
    if (m_cancelled) {
        return;
    }
    
    int itemIndex = reply->request().attribute(QNetworkRequest::User).toInt();
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray errorData = reply->readAll();
        QString errorMsg = tr("Network error (HTTP %1): %2").arg(httpStatus).arg(reply->errorString());
        emit logMessage(errorMsg);
        
        // Log raw response for debugging
        if (!errorData.isEmpty()) {
            emit logMessage(tr("Server response: %1").arg(QString::fromUtf8(errorData).left(500)));
        }
        
        // Check if it's a rate limit error (HTTP 429)
        if (httpStatus == 429) {
            emit logMessage(tr("Rate limited, waiting 60 seconds..."));
            m_processTimer->start(60000);  // Wait 60 seconds
            return;
        }
        
        // Check for authentication error (HTTP 401)
        if (httpStatus == 401) {
            emit translationError(tr("Invalid API Key. Please check your API key."));
            m_running = false;
            return;
        }
        
        // For other errors, skip this item and continue
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(2000);  // Wait 2 seconds before next
        return;
    }
    
    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject jsonObj = jsonDoc.object();
    
    // Check for API error
    if (jsonObj.contains("error")) {
        QJsonObject errorObj = jsonObj["error"].toObject();
        QString errorType = errorObj["type"].toString();
        QString errorMessage = errorObj["message"].toString();
        
        emit logMessage(tr("API Error: %1 - %2").arg(errorType, errorMessage));
        
        if (errorType == "rate_limit_error") {
            emit logMessage(tr("Rate limited, waiting 60 seconds..."));
            m_processTimer->start(60000);
            return;
        }
        
        // Skip this item
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(1000);
        return;
    }
    
    // Extract translation from response
    QString translation;
    if (jsonObj.contains("content")) {
        QJsonArray contentArray = jsonObj["content"].toArray();
        if (!contentArray.isEmpty()) {
            QJsonObject contentObj = contentArray[0].toObject();
            translation = contentObj["text"].toString().trimmed();
        }
    }
    
    if (translation.isEmpty()) {
        emit logMessage(tr("Empty translation received for item %1").arg(itemIndex));
        m_currentIndex++;
        emit progressChanged(m_currentIndex, m_totalItems);
        m_processTimer->start(500);
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
        m_processTimer->start(500);
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
        emit logMessage(tr("Translated [%1:%2]: %3 -> %4").arg(item.sureno).arg(item.ayet).arg(item.text.left(50)).arg(translation.left(50)));
    }
    
    m_currentIndex++;
    emit progressChanged(m_currentIndex, m_totalItems);
    
    // Small delay to avoid rate limiting (500ms between requests)
    m_processTimer->start(500);
}
