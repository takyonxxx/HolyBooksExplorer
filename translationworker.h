#ifndef TRANSLATIONWORKER_H
#define TRANSLATIONWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QTimer>
#include <QSslError>

class TranslationWorker : public QObject
{
    Q_OBJECT

public:
    enum TranslationType {
        TranslateMeal,      // meal_saf -> meal_en
        TranslateWord       // turkce -> english
    };

    explicit TranslationWorker(QObject *parent = nullptr);
    ~TranslationWorker();

    void setApiKey(const QString &apiKey);
    void setDatabase(const QString &dbPath);
    void startTranslation(int sureNo, TranslationType type);
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
    
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_dbPath;
    
    bool m_running;
    bool m_cancelled;
    TranslationType m_currentType;
    int m_currentSureNo;
    
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
};

#endif // TRANSLATIONWORKER_H
