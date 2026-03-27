#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

#include "MigrationRunner.h"

namespace {
QString uniqueConnectionName()
{
    return QStringLiteral("migration_runner_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool columnExists(QSqlDatabase &db, const QString &tableName, const QString &columnName)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1);").arg(tableName))) {
        return false;
    }

    while (query.next()) {
        if (query.value(QStringLiteral("name")).toString() == columnName) {
            return true;
        }
    }

    return false;
}

int schemaVersion(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT version FROM schema_version WHERE id = 1;")) || !query.next()) {
        return -1;
    }
    return query.value(0).toInt();
}

bool triggerExists(QSqlDatabase &db, const QString &triggerName)
{
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type = 'trigger' AND name = :name;"));
    query.bindValue(QStringLiteral(":name"), triggerName);
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toInt() == 1;
}

bool tableExists(QSqlDatabase &db, const QString &tableName)
{
    QSqlQuery query(db);
    query.prepare(
        QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = :name;"));
    query.bindValue(QStringLiteral(":name"), tableName);
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toInt() == 1;
}
}

class MigrationRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void run_appliesSyncMetadata();
    void run_recoversFromPartialSyncMigration();
    void run_usesDeletionTimeForTombstones();
};

void MigrationRunnerTest::run_appliesSyncMetadata()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        QCOMPARE(schemaVersion(db), 14);
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("sync_uuid")));
        QVERIFY(!columnExists(db, QStringLiteral("plants"), QStringLiteral("winter_location")));
        QVERIFY(!columnExists(db, QStringLiteral("plants"), QStringLiteral("summer_location")));
        QVERIFY(!columnExists(db, QStringLiteral("plants"), QStringLiteral("pot_size")));
        QVERIFY(!columnExists(db, QStringLiteral("plants"), QStringLiteral("current_health_status")));
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("poisonous_to_humans")));
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("poisonous_to_pets")));
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("indoor")));
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("flowering_season")));
        QVERIFY(columnExists(db, QStringLiteral("plants"), QStringLiteral("tags")));
        QVERIFY(tableExists(db, QStringLiteral("plant_tags_catalog")));
        QVERIFY(columnExists(db, QStringLiteral("plant_tags_catalog"), QStringLiteral("sync_uuid")));
        QVERIFY(columnExists(db, QStringLiteral("plant_tags_catalog"), QStringLiteral("updated_at")));
        QVERIFY(columnExists(db, QStringLiteral("journal_entries"), QStringLiteral("sync_uuid")));
        QVERIFY(columnExists(db, QStringLiteral("journal_entries"), QStringLiteral("updated_at")));
        QVERIFY(columnExists(db, QStringLiteral("reminders"), QStringLiteral("sync_uuid")));
        QVERIFY(columnExists(db, QStringLiteral("plant_images"), QStringLiteral("sync_uuid")));
        QVERIFY(columnExists(db, QStringLiteral("plant_care_schedules"), QStringLiteral("sync_uuid")));
        QVERIFY(triggerExists(db, QStringLiteral("plants_sync_defaults_after_insert")));
        QVERIFY(triggerExists(db, QStringLiteral("plant_tags_catalog_sync_defaults_after_insert")));

        QSqlQuery insertQuery(db);
        QVERIFY(insertQuery.exec(QStringLiteral("INSERT INTO plants (name) VALUES ('Trigger Test');")));

        QSqlQuery selectQuery(db);
        QVERIFY(selectQuery.exec(QStringLiteral(
            "SELECT sync_uuid, updated_at FROM plants WHERE name = 'Trigger Test';")));
        QVERIFY(selectQuery.next());
        QVERIFY(!selectQuery.value(0).toString().trimmed().isEmpty());
        QVERIFY(!selectQuery.value(1).toString().trimmed().isEmpty());

        QSqlQuery insertTagQuery(db);
        QVERIFY(insertTagQuery.exec(QStringLiteral("INSERT INTO plant_tags_catalog (name) VALUES ('indoor');")));

        QSqlQuery selectTagQuery(db);
        QVERIFY(selectTagQuery.exec(QStringLiteral(
            "SELECT sync_uuid, updated_at FROM plant_tags_catalog WHERE name = 'indoor';")));
        QVERIFY(selectTagQuery.next());
        QCOMPARE(selectTagQuery.value(0).toString(), QStringLiteral("indoor"));
        QVERIFY(!selectTagQuery.value(1).toString().trimmed().isEmpty());

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void MigrationRunnerTest::run_recoversFromPartialSyncMigration()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("UPDATE schema_version SET version = 7 WHERE id = 1;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plants_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plants_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plants_sync_tombstone_after_delete;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS journal_entries_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS journal_entries_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS journal_entries_sync_tombstone_after_delete;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS reminders_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS reminders_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS reminders_sync_tombstone_after_delete;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS reminder_settings_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS reminder_settings_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_images_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_images_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_images_sync_tombstone_after_delete;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_care_schedules_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_care_schedules_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_care_schedules_sync_tombstone_after_delete;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_tags_catalog_sync_defaults_after_insert;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_tags_catalog_sync_touch_after_update;")));
        QVERIFY(query.exec(QStringLiteral("DROP TRIGGER IF EXISTS plant_tags_catalog_sync_tombstone_after_delete;")));

        QVERIFY(runner.run(db));

        QCOMPARE(schemaVersion(db), 14);
        QVERIFY(triggerExists(db, QStringLiteral("plants_sync_defaults_after_insert")));
        QVERIFY(triggerExists(db, QStringLiteral("journal_entries_sync_defaults_after_insert")));
        QVERIFY(triggerExists(db, QStringLiteral("reminders_sync_defaults_after_insert")));
        QVERIFY(triggerExists(db, QStringLiteral("plant_tags_catalog_sync_defaults_after_insert")));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void MigrationRunnerTest::run_usesDeletionTimeForTombstones()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        QSqlQuery insertQuery(db);
        QVERIFY(insertQuery.exec(QStringLiteral(
            "INSERT INTO plants (name, sync_uuid, updated_at) "
            "VALUES ('Delete Test', 'delete-test-sync-uuid', '2000-01-01T00:00:00.000Z');")));

        QSqlQuery deleteQuery(db);
        QVERIFY(deleteQuery.exec(QStringLiteral(
            "DELETE FROM plants WHERE sync_uuid = 'delete-test-sync-uuid';")));

        QSqlQuery tombstoneQuery(db);
        QVERIFY(tombstoneQuery.exec(QStringLiteral(
            "SELECT deleted_at FROM sync_tombstones "
            "WHERE table_name = 'plants' AND sync_uuid = 'delete-test-sync-uuid';")));
        QVERIFY(tombstoneQuery.next());

        const QString deletedAt = tombstoneQuery.value(0).toString();
        QVERIFY(!deletedAt.isEmpty());
        QVERIFY(deletedAt != QStringLiteral("2000-01-01T00:00:00.000Z"));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

QTEST_MAIN(MigrationRunnerTest)
#include "test_migration_runner.moc"
