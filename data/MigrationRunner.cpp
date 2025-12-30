#include "MigrationRunner.h"

#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

bool MigrationRunner::run(QSqlDatabase &db)
{
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery pragmaQuery(db);
    pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    if (!ensureSchemaVersionTable(db)) {
        return false;
    }

    const int current = currentVersion(db);
    const QStringList list = migrations();

    for (int index = 0; index < list.size(); ++index) {
        const int targetVersion = index + 1;
        if (targetVersion <= current) {
            continue;
        }

        if (!applyMigration(db, list.at(index))) {
            return false;
        }

        if (!setVersion(db, targetVersion)) {
            return false;
        }
    }

    return true;
}

bool MigrationRunner::ensureSchemaVersionTable(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS schema_version ("
                    "id INTEGER PRIMARY KEY CHECK (id = 1),"
                    "version INTEGER NOT NULL"
                    ");")) {
        qWarning() << "Failed to create schema_version:" << query.lastError().text();
        return false;
    }

    if (!query.exec("INSERT OR IGNORE INTO schema_version (id, version) VALUES (1, 0);")) {
        qWarning() << "Failed to seed schema_version:" << query.lastError().text();
        return false;
    }

    return true;
}

int MigrationRunner::currentVersion(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec("SELECT version FROM schema_version WHERE id = 1;")) {
        return 0;
    }
    if (!query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

bool MigrationRunner::setVersion(QSqlDatabase &db, int version)
{
    QSqlQuery query(db);
    query.prepare("UPDATE schema_version SET version = :version WHERE id = 1;");
    query.bindValue(":version", version);
    if (!query.exec()) {
        qWarning() << "Failed to update schema_version:" << query.lastError().text();
        return false;
    }
    return true;
}

bool MigrationRunner::applyMigration(QSqlDatabase &db, const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open migration resource:" << resourcePath;
        return false;
    }

    const QString content = QString::fromUtf8(file.readAll());
    const QStringList lines = content.split('\n');
    QString filtered;
    filtered.reserve(content.size());
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith("--")) {
            continue;
        }
        filtered.append(line);
        filtered.append('\n');
    }

    const QStringList statements = filtered.split(';');
    for (const QString &statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        QSqlQuery query(db);
        if (!query.exec(trimmed)) {
            qWarning() << "Migration failed:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

QStringList MigrationRunner::migrations() const
{
    return {
        ":/migrations/001_init.sql",
        ":/migrations/002_add_plant_fields.sql",
        ":/migrations/003_reminders_multi.sql",
        ":/migrations/004_add_reminder_name.sql",
        ":/migrations/005_seed_plants.sql"
    };
}
