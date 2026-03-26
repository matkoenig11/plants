#include <QGuiApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "MigrationRunner.h"
#include "PlantListViewModel.h"
#include "JournalEntryListViewModel.h"
#include "ReminderListViewModel.h"
#include "ReminderSettingsViewModel.h"
#include "DatabaseConnectionViewModel.h"

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

QString standardDatabasePath()
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (ensureWritableDir(baseDir)) {
        return QDir(baseDir).filePath("plant_journal.sqlite");
    }
    return QString();
}

void migrateLegacyDatabaseIfNeeded(const QString &targetPath, const QStringList &legacyCandidates)
{
    if (targetPath.isEmpty() || QFileInfo::exists(targetPath)) {
        return;
    }

    for (const QString &candidate : legacyCandidates) {
        const QFileInfo info(candidate);
        if (!info.exists() || !info.isFile()) {
            continue;
        }

        if (QFile::copy(info.absoluteFilePath(), targetPath)) {
            qDebug() << "Migrated existing database to standard path from:" << info.absoluteFilePath();
        } else {
            qWarning() << "Failed to migrate database from:" << info.absoluteFilePath();
        }
        return;
    }
}

QString databasePath()
{
    const QString overridePath = qEnvironmentVariable("PLANT_JOURNAL_DB_PATH");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }

    const QString standardPath = standardDatabasePath();
    if (!standardPath.isEmpty()) {
        migrateLegacyDatabaseIfNeeded(standardPath, {
            QDir::current().filePath("plant_journal.sqlite")
        });
        return standardPath;
    }

    return QDir::current().filePath("plant_journal.sqlite");
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

    QFile seedFile(":/sql/seeds/001_plants.sql");
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
    QCoreApplication::setOrganizationName(QStringLiteral("PlantJournal"));
    QCoreApplication::setApplicationName(QStringLiteral("PlantJournal"));
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    const QString dbPath = databasePath();
    qDebug() << "Using database path:" << dbPath;
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning("Failed to open SQLite database.");
    } else {
        MigrationRunner runner;
        if (!runner.run(db)) {
            qWarning("Failed to apply migrations.");
        }
        seedPlantsIfEmpty(db);
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&db]() {
        if (db.isOpen()) {
            db.close();
        }
    });

    PlantListViewModel plantListViewModel(db);
    JournalEntryListViewModel journalEntryViewModel(db);
    ReminderListViewModel reminderListViewModel(db);
    ReminderSettingsViewModel reminderSettingsViewModel(db);
    DatabaseConnectionViewModel databaseConnectionViewModel(db);

    QObject::connect(&journalEntryViewModel,
                     &JournalEntryListViewModel::entriesChanged,
                     &plantListViewModel,
                     [&plantListViewModel](int plantId) {
                         qDebug() << "Refreshing plant list after journal change for plant:" << plantId;
                         plantListViewModel.refresh();
                     });

    QObject::connect(&databaseConnectionViewModel,
                     &DatabaseConnectionViewModel::synchronizationFinished,
                     &plantListViewModel,
                     [&plantListViewModel,
                      &journalEntryViewModel,
                      &reminderListViewModel,
                      &reminderSettingsViewModel](bool success) {
                         if (!success) {
                             return;
                         }
                         plantListViewModel.refresh();
                         journalEntryViewModel.refresh();
                         reminderListViewModel.refresh();
                         reminderSettingsViewModel.reload();
                     });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("plantListViewModel", &plantListViewModel);
    engine.rootContext()->setContextProperty("journalEntryViewModel", &journalEntryViewModel);
    engine.rootContext()->setContextProperty("reminderListViewModel", &reminderListViewModel);
    engine.rootContext()->setContextProperty("reminderSettingsViewModel", &reminderSettingsViewModel);
    engine.rootContext()->setContextProperty("databaseConnectionViewModel", &databaseConnectionViewModel);
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
