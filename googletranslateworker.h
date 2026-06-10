#ifndef GOOGLETRANSLATEWORKER_H
#define GOOGLETRANSLATEWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QTimer>
#include <QQueue>

class GoogleTranslateWorker : public QObject
{
    Q_OBJECT

public:
    enum TranslationType {
        TranslateMeal,      // meal_saf -> meal_en
        TranslateWord       // turkce -> english
    };

    explicit GoogleTranslateWorker(QObject *parent = nullptr);
    ~GoogleTranslateWorker();

    void setDatabase(const QString &dbPath);
    void startTranslation(int sureNo, TranslationType type);  // sureNo=-1 means all surahs
    void cancelTranslation();
    
    bool isRunning() const { return m_running; }

signals:
    void progressChanged(int current, int total);
    void translationComplete();
    void translationError(const QString &error);
    void logMessage(const QString &message);

private slots:
    void processNextItem();
    void onNetworkReply(QNetworkReply *reply);

private:
    void translateMeal(int sureNo);
    void translateWord(int sureNo);
    void sendTranslationRequest(const QString &text, int recordId);
    void processNextSure();
    QString extractTranslation(const QString &html);
    int backoffDelayMs() const;
    void scheduleRetryOrSkip(const QString &reason);
    
    QNetworkAccessManager *m_networkManager;
    QString m_dbPath;
    
    bool m_running;
    bool m_cancelled;
    TranslationType m_currentType;
    int m_currentSureNo;
    
    // For processing multiple surahs
    QList<int> m_sureList;
    int m_currentSureIndex;
    
    // Queue for batch processing
    struct TranslationItem {
        int id;           // ayet number for meal, rowid for word
        QString text;     // text to translate
        int sureno;
        int ayet;
    };
    QList<TranslationItem> m_queue;
    int m_currentIndex;
    int m_totalItems;
    
    QTimer *m_processTimer;
    
    // Rate limiting
    int m_requestCount;
    QTimer *m_rateLimitTimer;
    static const int MAX_REQUESTS_PER_MINUTE = 15; // Google'ın rate limit'ini aşmamak için

    // Otomatik yeniden deneme / kademeli bekleme (self-healing)
    int m_retryCount;          // mevcut item icin yapilan deneme sayisi
    int m_consecutiveErrors;   // arka arkaya hata sayisi (adaptive backoff icin)
    static const int MAX_RETRIES = 5;          // bir item icin maksimum deneme
    static const int BASE_DELAY_MS = 4000;     // istekler arasi normal bekleme (~15/dk)
    static const int MAX_BACKOFF_MS = 120000;  // backoff tavani (2 dakika)
    static const int REQUEST_TIMEOUT_MS = 15000; // tek istek icin transfer timeout (asili kalmayi onler)
};

#endif // GOOGLETRANSLATEWORKER_H
