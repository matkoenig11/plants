#include "MigrationRunner.h"

#include <QFile>
#include <QList>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace {
bool isTriggerStart(const QString &line)
{
    return line.startsWith(QStringLiteral("CREATE TRIGGER"), Qt::CaseInsensitive)
        || line.startsWith(QStringLiteral("CREATE TEMP TRIGGER"), Qt::CaseInsensitive)
        || line.startsWith(QStringLiteral("CREATE TEMPORARY TRIGGER"), Qt::CaseInsensitive);
}

QList<QString> splitSqlStatements(const QString &content)
{
    QList<QString> statements;
    QString currentStatement;
    bool insideTrigger = false;

    const QStringList lines = content.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("--"))) {
            continue;
        }

        if (currentStatement.isEmpty() && trimmed.isEmpty()) {
            continue;
        }

        if (!currentStatement.isEmpty()) {
            currentStatement.append(QLatin1Char('\n'));
        }
        currentStatement.append(line);

        if (!insideTrigger && isTriggerStart(trimmed)) {
            insideTrigger = true;
        }

        if (insideTrigger) {
            if (trimmed.compare(QStringLiteral("END;"), Qt::CaseInsensitive) == 0) {
                statements.append(currentStatement.trimmed());
                currentStatement.clear();
                insideTrigger = false;
            }
            continue;
        }

        if (trimmed.endsWith(QLatin1Char(';'))) {
            statements.append(currentStatement.trimmed());
            currentStatement.clear();
        }
    }

    if (!currentStatement.trimmed().isEmpty()) {
        statements.append(currentStatement.trimmed());
    }

    return statements;
}

bool isIgnorableSqlError(const QString &statement, const QSqlError &error)
{
    const QString trimmed = statement.trimmed();
    const QString message = error.text().trimmed().toLower();

    const bool isAlterTable = trimmed.startsWith(QStringLiteral("ALTER TABLE"), Qt::CaseInsensitive);
    if (!isAlterTable) {
        return false;
    }

    if (trimmed.contains(QStringLiteral(" ADD COLUMN "), Qt::CaseInsensitive)
        && message.contains(QStringLiteral("duplicate column name"))) {
        return true;
    }

    return trimmed.contains(QStringLiteral(" DROP COLUMN "), Qt::CaseInsensitive)
        && message.contains(QStringLiteral("no such column"));
}
}

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
    const QList<QStringList> list = migrations();

    for (int index = 0; index < list.size(); ++index) {
        const int targetVersion = index + 1;
        if (targetVersion <= current) {
            continue;
        }

        const QStringList scripts = list.at(index);
        for (const QString &script : scripts) {
            if (!applyScript(db, script)) {
                return false;
            }
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

bool MigrationRunner::applyScript(QSqlDatabase &db, const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open SQL resource:" << resourcePath;
        return false;
    }

    const QString content = QString::fromUtf8(file.readAll());
    const QList<QString> statements = splitSqlStatements(content);
    for (const QString &statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        QSqlQuery query(db);
        if (!query.exec(trimmed)) {
            if (isIgnorableSqlError(trimmed, query.lastError())) {
                continue;
            }
            qWarning() << "SQL script failed:" << resourcePath << query.lastError().text()
                       << "Statement:" << trimmed.left(160);
            return false;
        }
    }

    return true;
}

QList<QStringList> MigrationRunner::migrations() const
{
    return {
        {
            ":/sql/schema/001_plants.sql",
            ":/sql/schema/002_journal_entries.sql",
            ":/sql/schema/003_reminders.sql"
        },
        {
            ":/sql/plants/101_identity_and_location.sql",
            ":/sql/plants/102_care_profile.sql",
            ":/sql/plants/103_health_and_source.sql"
        },
        {
            ":/sql/reminders/101_settings.sql",
            ":/sql/reminders/102_multi_plant_rework.sql"
        },
        {
            ":/sql/reminders/201_custom_name.sql"
        },
        {
            ":/sql/seeds/001_plants.sql"
        },
        {
            ":/sql/plants/104_plant_images.sql"
        },
        {
            ":/sql/schedules/001_plant_care_schedules.sql"
        },
        {
            ":/sql/sync/001_sync_metadata.sql"
        },
        {
            ":/sql/sync/002_tombstone_deletion_timestamps.sql"
        },
        {
            ":/sql/plants/105_remove_unused_fields.sql"
        },
        {
            ":/sql/plants/106_library_fields.sql"
        },
        {
            ":/sql/plants/107_tags.sql"
        },
        {
            ":/sql/plants/108_tag_catalog.sql"
        },
        {
            ":/sql/sync/003_tag_catalog_sync.sql"
        }
    };
}
