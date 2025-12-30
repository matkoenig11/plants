#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>

#include "MigrationRunner.h"
#include "PlantListViewModel.h"
#include "JournalEntryListViewModel.h"
#include "ReminderListViewModel.h"
#include "ReminderSettingsViewModel.h"

namespace {
bool ensureWritableDir(const QString &dirPath)
{
    if (dirPath.isEmpty()) {
        return false;
    }

    if (!QDir().mkpath(dirPath)) {
        return false;
    }

    QFile probe(QDir(dirPath).filePath(".write_test"));
    if (!probe.open(QIODevice::WriteOnly)) {
        return false;
    }
    probe.close();
    probe.remove();
    return true;
}

QString databasePath()
{
    // const QString overridePath = qEnvironmentVariable("PLANT_JOURNAL_DB_PATH");
    // if (!overridePath.isEmpty()) {
    //     return overridePath;
    // }

    // const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // if (ensureWritableDir(baseDir)) {
    //     return QDir(baseDir).filePath("plant_journal.sqlite");
    // }

    return QDir::current().filePath("../../plant_journal.sqlite");
}

bool seedPlantsIfEmpty(QSqlDatabase &db)
{
    QSqlQuery countQuery(db);
    if (!countQuery.exec("SELECT COUNT(*) FROM plants")) {
        qWarning() << "Failed to count plants for seed:" << countQuery.lastError().text();
        return false;
    }
    if (countQuery.next() && countQuery.value(0).toInt() > 0) {
        return true;
    }

    QFile seedFile(":/migrations/005_seed_plants.sql");
    if (!seedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Seed file not found";
        return false;
    }
    const QString content = QString::fromUtf8(seedFile.readAll());
    const QStringList statements = content.split(';');
    for (const QString &statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("--")) {
            continue;
        }
        QSqlQuery q(db);
        if (!q.exec(trimmed)) {
            qWarning() << "Seed insert failed:" << q.lastError().text();
            return false;
        }
    }
    return true;
}
}

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString databasePathStr = databasePath();
    qDebug() << "Using database path:" << databasePathStr;
    db.setDatabaseName(databasePath());
    if (!db.open()) {
        qWarning("Failed to open SQLite database.");
    // } else {
    //     MigrationRunner runner;
    //     if (!runner.run(db)) {
    //         qWarning("Failed to apply migrations.");
    //     }
    //     seedPlantsIfEmpty(db);
    }




    PlantListViewModel plantListViewModel(db);
    JournalEntryListViewModel journalEntryViewModel(db);
    ReminderListViewModel reminderListViewModel(db);
    ReminderSettingsViewModel reminderSettingsViewModel(db);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("plantListViewModel", &plantListViewModel);
    engine.rootContext()->setContextProperty("journalEntryViewModel", &journalEntryViewModel);
    engine.rootContext()->setContextProperty("reminderListViewModel", &reminderListViewModel);
    engine.rootContext()->setContextProperty("reminderSettingsViewModel", &reminderSettingsViewModel);

    const QUrl url(QStringLiteral("qrc:/mobile_app/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
