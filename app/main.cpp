#include <QGuiApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>

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
    const QString overridePath = qEnvironmentVariable("PLANT_JOURNAL_DB_PATH");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }

    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (ensureWritableDir(baseDir)) {
        return QDir(baseDir).filePath("plant_journal.sqlite");
    }

    return QDir::current().filePath("plant_journal.sqlite");
}
}

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle("Material");
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databasePath());
    if (!db.open()) {
        qWarning("Failed to open SQLite database.");
    } else {
        MigrationRunner runner;
        if (!runner.run(db)) {
            qWarning("Failed to apply migrations.");
        }
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
    const QUrl url(QStringLiteral("qrc:/app/Main.qml"));
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
