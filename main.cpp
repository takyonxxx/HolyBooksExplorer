#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStyleFactory>
#include <QStandardPaths>

// Veritabanını resource'dan uygulama dizinine kopyala
bool copyDatabaseFromResources()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString dbDestPath = appDir + "/Kutsal_Kitaplar.db";
    QString dbSourcePath = ":/data/Kutsal_Kitaplar.db";
    
    // Eğer veritabanı zaten varsa ve boyutu doğruysa kopyalama
    QFile destFile(dbDestPath);
    QFile sourceFile(dbSourcePath);
    
    if (destFile.exists()) {
        // Boyut kontrolü - eğer farklıysa güncelle
        if (sourceFile.open(QIODevice::ReadOnly)) {
            qint64 sourceSize = sourceFile.size();
            sourceFile.close();
            
            if (destFile.size() == sourceSize) {
                return true; // Zaten güncel
            }
            // Eski dosyayı sil
            destFile.remove();
        }
    }
    
    // Resource'dan kopyala
    if (QFile::copy(dbSourcePath, dbDestPath)) {
        // Yazma izni ver (resource'dan kopyalanan dosya read-only olabilir)
        QFile::setPermissions(dbDestPath, 
            QFileDevice::ReadOwner | QFileDevice::WriteOwner |
            QFileDevice::ReadGroup | QFileDevice::ReadOther);
        return true;
    }
    
    return false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Application info
    app.setApplicationName("HolyBooksExplorer");
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("MarenRobotics");
    app.setOrganizationDomain("marenrobotics.com");
    
    // Set Fusion style for consistent look
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Veritabanını kopyala
    if (!copyDatabaseFromResources()) {
        qWarning() << "Could not copy database from resources!";
    }
    
    // Load settings
    QSettings settings;
    QString language = settings.value("language", "tr").toString();
    
    // Load translator
    QTranslator translator;
    QString translationFile = QString(":/translations/holybooksexplorer_%1.qm").arg(language);
    if (translator.load(translationFile)) {
        app.installTranslator(&translator);
    }
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
