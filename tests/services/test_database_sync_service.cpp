#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QUuid>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "DatabaseSyncService.h"
#include "MigrationRunner.h"

namespace {
QString uniqueConnectionName()
{
    return QStringLiteral("database_sync_service_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

class FakeSyncServer : public QObject
{
    Q_OBJECT

public:
    explicit FakeSyncServer(QObject *parent = nullptr)
        : QObject(parent)
    {
        QObject::connect(&m_server, &QTcpServer::newConnection, this, [this]() {
            while (m_server.hasPendingConnections()) {
                QTcpSocket *socket = m_server.nextPendingConnection();
                QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                    m_buffer.append(socket->readAll());

                    const QByteArray separator("\r\n\r\n");
                    const int headerEnd = m_buffer.indexOf(separator);
                    if (headerEnd < 0) {
                        return;
                    }

                    const QByteArray headers = m_buffer.left(headerEnd);
                    int contentLength = 0;
                    for (const QByteArray &line : headers.split('\n')) {
                        const QByteArray trimmed = line.trimmed();
                        const QByteArray lower = trimmed.toLower();
                        if (lower.startsWith("content-length:")) {
                            contentLength = trimmed.mid(sizeof("Content-Length:") - 1).trimmed().toInt();
                        } else if (lower.startsWith("authorization:")) {
                            m_authorizationHeaders.append(
                                trimmed.mid(sizeof("Authorization:") - 1).trimmed());
                        }
                    }

                    const QByteArray body = m_buffer.mid(headerEnd + separator.size());
                    if (body.size() < contentLength) {
                        return;
                    }

                    m_requestBodies.append(body.left(contentLength));

                    const QPair<int, QJsonObject> responseDef = m_responses.isEmpty()
                        ? qMakePair(200, QJsonObject())
                        : m_responses.takeFirst();
                    const QByteArray responseBody = QJsonDocument(responseDef.second).toJson(QJsonDocument::Compact);
                    const QByteArray statusText = responseDef.first == 200 ? "OK" : "ERROR";
                    const QByteArray response =
                        "HTTP/1.1 " + QByteArray::number(responseDef.first) + ' ' + statusText + "\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: " + QByteArray::number(responseBody.size()) + "\r\n"
                        "Connection: close\r\n\r\n"
                        + responseBody;

                    socket->write(response);
                    socket->disconnectFromHost();
                    m_buffer.clear();
                });
            }
        });
    }

    bool start()
    {
        return m_server.listen(QHostAddress::LocalHost);
    }

    QString baseUrl() const
    {
        return QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort());
    }

    void setResponse(int statusCode, const QJsonObject &body)
    {
        m_responses.clear();
        enqueueResponse(statusCode, body);
    }

    void enqueueResponse(int statusCode, const QJsonObject &body)
    {
        m_responses.append({statusCode, body});
    }

    QByteArray authorizationHeader(int requestIndex = -1) const
    {
        const int index = requestIndex < 0 ? m_authorizationHeaders.size() - 1 : requestIndex;
        return (index >= 0 && index < m_authorizationHeaders.size()) ? m_authorizationHeaders.at(index)
                                                                     : QByteArray();
    }

    QByteArray requestBody(int requestIndex = -1) const
    {
        const int index = requestIndex < 0 ? m_requestBodies.size() - 1 : requestIndex;
        return (index >= 0 && index < m_requestBodies.size()) ? m_requestBodies.at(index)
                                                              : QByteArray();
    }

    int requestCount() const { return m_requestBodies.size(); }

private:
    QTcpServer m_server;
    QByteArray m_buffer;
    QList<QByteArray> m_authorizationHeaders;
    QList<QByteArray> m_requestBodies;
    QList<QPair<int, QJsonObject>> m_responses;
};

QJsonObject emptyApplied()
{
    return {
        { QStringLiteral("plants"), 0 },
        { QStringLiteral("journal_entries"), 0 },
        { QStringLiteral("reminders"), 0 },
        { QStringLiteral("plant_images"), 0 },
        { QStringLiteral("plant_care_schedules"), 0 },
        { QStringLiteral("reminder_settings"), 0 },
        { QStringLiteral("tombstones"), 0 }
    };
}

QJsonObject emptyChanges()
{
    return {
        { QStringLiteral("plants"), QJsonArray() },
        { QStringLiteral("journal_entries"), QJsonArray() },
        { QStringLiteral("reminders"), QJsonArray() },
        { QStringLiteral("plant_images"), QJsonArray() },
        { QStringLiteral("plant_care_schedules"), QJsonArray() },
        { QStringLiteral("reminder_settings"), QJsonValue::Null },
        { QStringLiteral("tombstones"), QJsonArray() }
    };
}
}

class DatabaseSyncServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void synchronize_appliesServerChanges();
    void synchronize_httpFailureLeavesDatabaseUntouched();
    void forcePull_replacesLocalDataWithServerSnapshot();
    void synchronize_afterForcePullUploadsDeletedPlantTombstone();
};

void DatabaseSyncServiceTest::synchronize_appliesServerChanges()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        FakeSyncServer server;
        QVERIFY(server.start());
        server.setResponse(200, {
            { QStringLiteral("new_cursor"), QStringLiteral("2026-03-26T12:00:00.000Z") },
            { QStringLiteral("applied"), emptyApplied() },
            { QStringLiteral("server_changes"), QJsonObject{
                { QStringLiteral("plants"), QJsonArray{
                    QJsonObject{
                        { QStringLiteral("sync_uuid"), QStringLiteral("server-plant-1") },
                        { QStringLiteral("name"), QStringLiteral("Server Plant") },
                        { QStringLiteral("species"), QStringLiteral("Epipremnum aureum") },
                        { QStringLiteral("location"), QStringLiteral("Living room") },
                        { QStringLiteral("scientific_name"), QStringLiteral("Epipremnum aureum") },
                        { QStringLiteral("plant_type"), QStringLiteral("Vine") },
                        { QStringLiteral("light_requirement"), QStringLiteral("Bright indirect") },
                        { QStringLiteral("watering_frequency"), QStringLiteral("7 days") },
                        { QStringLiteral("watering_notes"), QStringLiteral("Water when top soil is dry") },
                        { QStringLiteral("humidity_preference"), QStringLiteral("Medium") },
                        { QStringLiteral("soil_type"), QStringLiteral("Chunky mix") },
                        { QStringLiteral("last_watered"), QStringLiteral("2026-03-20") },
                        { QStringLiteral("fertilizing_schedule"), QStringLiteral("Monthly") },
                        { QStringLiteral("last_fertilized"), QStringLiteral("2026-03-01") },
                        { QStringLiteral("pruning_time"), QStringLiteral("Spring") },
                        { QStringLiteral("pruning_notes"), QStringLiteral("Trim runners") },
                        { QStringLiteral("last_pruned"), QStringLiteral("2026-02-15") },
                        { QStringLiteral("growth_rate"), QStringLiteral("Fast") },
                        { QStringLiteral("issues_pests"), QStringLiteral("") },
                        { QStringLiteral("temperature_tolerance"), QStringLiteral("18-27C") },
                        { QStringLiteral("toxic_to_pets"), QStringLiteral("Yes") },
                        { QStringLiteral("poisonous_to_humans"), QStringLiteral("No") },
                        { QStringLiteral("poisonous_to_pets"), QStringLiteral("Yes") },
                        { QStringLiteral("indoor"), QStringLiteral("Yes") },
                        { QStringLiteral("flowering_season"), QStringLiteral("Summer") },
                        { QStringLiteral("acquired_on"), QStringLiteral("2025-05-01") },
                        { QStringLiteral("source"), QStringLiteral("Nursery") },
                        { QStringLiteral("notes"), QStringLiteral("Imported from server") },
                        { QStringLiteral("created_at"), QStringLiteral("2026-03-26T11:00:00.000Z") },
                        { QStringLiteral("updated_at"), QStringLiteral("2026-03-26T11:00:00.000Z") }
                    }
                } },
                { QStringLiteral("journal_entries"), QJsonArray() },
                { QStringLiteral("reminders"), QJsonArray() },
                { QStringLiteral("plant_images"), QJsonArray() },
                { QStringLiteral("plant_care_schedules"), QJsonArray() },
                { QStringLiteral("reminder_settings"), QJsonValue::Null },
                { QStringLiteral("tombstones"), QJsonArray() }
            } },
            { QStringLiteral("conflicts"), QJsonArray() }
        });

        SyncServerSettings settings;
        settings.baseUrl = server.baseUrl();
        settings.deviceToken = QStringLiteral("test-token");
        settings.clientId = QStringLiteral("client-1");

        DatabaseSyncService service(db);
        QString summary;
        QString error;
        QVERIFY2(service.synchronize(&settings, &summary, &error), qPrintable(error));
        QCOMPARE(settings.lastSyncCursor, QStringLiteral("2026-03-26T12:00:00.000Z"));
        QVERIFY(summary.contains(QStringLiteral("Synchronization finished.")));
        QCOMPARE(server.authorizationHeader(), QByteArray("Bearer test-token"));

        const QJsonObject requestObject = QJsonDocument::fromJson(server.requestBody()).object();
        QCOMPARE(requestObject.value(QStringLiteral("client_id")).toString(), QStringLiteral("client-1"));

        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT name FROM plants WHERE sync_uuid = :sync_uuid;"));
        query.bindValue(QStringLiteral(":sync_uuid"), QStringLiteral("server-plant-1"));
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Server Plant"));
        QVERIFY(query.exec(QStringLiteral("SELECT poisonous_to_pets, indoor FROM plants WHERE sync_uuid = 'server-plant-1';")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Yes"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("Yes"));

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void DatabaseSyncServiceTest::synchronize_httpFailureLeavesDatabaseUntouched()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        FakeSyncServer server;
        QVERIFY(server.start());
        server.setResponse(500, {
            { QStringLiteral("detail"), QStringLiteral("sync failed on server") }
        });

        SyncServerSettings settings;
        settings.baseUrl = server.baseUrl();
        settings.deviceToken = QStringLiteral("test-token");
        settings.clientId = QStringLiteral("client-2");

        DatabaseSyncService service(db);
        QString summary;
        QString error;
        QVERIFY(!service.synchronize(&settings, &summary, &error));
        QVERIFY(error.contains(QStringLiteral("sync failed on server")));
        QVERIFY(settings.lastSyncCursor.isEmpty());

        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT COUNT(*) FROM plants WHERE sync_uuid = 'server-plant-1';")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void DatabaseSyncServiceTest::forcePull_replacesLocalDataWithServerSnapshot()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        QSqlQuery seedLocal(db);
        QVERIFY(seedLocal.exec(
            QStringLiteral("INSERT INTO plants (name, sync_uuid, created_at, updated_at) "
                           "VALUES ('Local Plant', 'local-plant-1', '2026-03-26T08:00:00.000Z', '2026-03-26T08:00:00.000Z');")));

        FakeSyncServer server;
        QVERIFY(server.start());
        server.setResponse(200, {
            { QStringLiteral("new_cursor"), QStringLiteral("2026-03-26T14:00:00.000Z") },
            { QStringLiteral("applied"), emptyApplied() },
            { QStringLiteral("server_changes"), QJsonObject{
                { QStringLiteral("plants"), QJsonArray{
                    QJsonObject{
                        { QStringLiteral("sync_uuid"), QStringLiteral("server-plant-2") },
                        { QStringLiteral("name"), QStringLiteral("Fresh Server Plant") },
                        { QStringLiteral("species"), QStringLiteral("Monstera deliciosa") },
                        { QStringLiteral("location"), QStringLiteral("Office") },
                        { QStringLiteral("scientific_name"), QStringLiteral("Monstera deliciosa") },
                        { QStringLiteral("plant_type"), QStringLiteral("Aroid") },
                        { QStringLiteral("light_requirement"), QStringLiteral("Bright indirect") },
                        { QStringLiteral("watering_frequency"), QStringLiteral("10 days") },
                        { QStringLiteral("watering_notes"), QStringLiteral("Let top third dry") },
                        { QStringLiteral("humidity_preference"), QStringLiteral("High") },
                        { QStringLiteral("soil_type"), QStringLiteral("Chunky") },
                        { QStringLiteral("last_watered"), QStringLiteral("2026-03-20") },
                        { QStringLiteral("fertilizing_schedule"), QStringLiteral("Monthly") },
                        { QStringLiteral("last_fertilized"), QStringLiteral("2026-03-05") },
                        { QStringLiteral("pruning_time"), QStringLiteral("Spring") },
                        { QStringLiteral("pruning_notes"), QStringLiteral("Trim aerial roots if needed") },
                        { QStringLiteral("last_pruned"), QStringLiteral("2026-02-01") },
                        { QStringLiteral("growth_rate"), QStringLiteral("Medium") },
                        { QStringLiteral("issues_pests"), QStringLiteral("") },
                        { QStringLiteral("temperature_tolerance"), QStringLiteral("18-28C") },
                        { QStringLiteral("toxic_to_pets"), QStringLiteral("Yes") },
                        { QStringLiteral("poisonous_to_humans"), QStringLiteral("No") },
                        { QStringLiteral("poisonous_to_pets"), QStringLiteral("Yes") },
                        { QStringLiteral("indoor"), QStringLiteral("Yes") },
                        { QStringLiteral("flowering_season"), QStringLiteral("Summer") },
                        { QStringLiteral("acquired_on"), QStringLiteral("2024-06-01") },
                        { QStringLiteral("source"), QStringLiteral("Server") },
                        { QStringLiteral("notes"), QStringLiteral("Pulled from server") },
                        { QStringLiteral("created_at"), QStringLiteral("2026-03-26T13:00:00.000Z") },
                        { QStringLiteral("updated_at"), QStringLiteral("2026-03-26T13:00:00.000Z") }
                    }
                } },
                { QStringLiteral("journal_entries"), QJsonArray() },
                { QStringLiteral("reminders"), QJsonArray() },
                { QStringLiteral("plant_images"), QJsonArray() },
                { QStringLiteral("plant_care_schedules"), QJsonArray() },
                { QStringLiteral("reminder_settings"), QJsonValue::Null },
                { QStringLiteral("tombstones"), QJsonArray() }
            } },
            { QStringLiteral("conflicts"), QJsonArray() }
        });

        SyncServerSettings settings;
        settings.baseUrl = server.baseUrl();
        settings.deviceToken = QStringLiteral("test-token");
        settings.clientId = QStringLiteral("client-force-pull");
        settings.lastSyncCursor = QStringLiteral("2026-03-26T09:00:00.000Z");

        DatabaseSyncService service(db);
        QString summary;
        QString error;
        QVERIFY2(service.forcePull(&settings, &summary, &error), qPrintable(error));
        QCOMPARE(settings.lastSyncCursor, QStringLiteral("2026-03-26T14:00:00.000Z"));
        QVERIFY(summary.contains(QStringLiteral("Force pull completed.")));

        const QJsonObject requestObject = QJsonDocument::fromJson(server.requestBody()).object();
        QVERIFY(requestObject.value(QStringLiteral("last_sync_cursor")).isNull());
        const QJsonObject changesObject = requestObject.value(QStringLiteral("changes")).toObject();
        QCOMPARE(changesObject.value(QStringLiteral("plants")).toArray().size(), 0);
        QVERIFY(changesObject.value(QStringLiteral("reminder_settings")).isNull());

        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral("SELECT name, sync_uuid FROM plants ORDER BY name;")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Fresh Server Plant"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("server-plant-2"));
        QVERIFY(!query.next());

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

void DatabaseSyncServiceTest::synchronize_afterForcePullUploadsDeletedPlantTombstone()
{
    const QString connectionName = uniqueConnectionName();

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(db.open());

        MigrationRunner runner;
        QVERIFY(runner.run(db));

        FakeSyncServer server;
        QVERIFY(server.start());
        server.enqueueResponse(200, {
            { QStringLiteral("new_cursor"), QStringLiteral("2999-01-01T00:00:00.000Z") },
            { QStringLiteral("applied"), emptyApplied() },
            { QStringLiteral("server_changes"), QJsonObject{
                { QStringLiteral("plants"), QJsonArray{
                    QJsonObject{
                        { QStringLiteral("sync_uuid"), QStringLiteral("astrantia-sync-1") },
                        { QStringLiteral("name"), QStringLiteral("Astrantia") },
                        { QStringLiteral("species"), QStringLiteral("Astrantia major") },
                        { QStringLiteral("location"), QStringLiteral("Garden") },
                        { QStringLiteral("scientific_name"), QStringLiteral("Astrantia major") },
                        { QStringLiteral("plant_type"), QStringLiteral("Perennial") },
                        { QStringLiteral("light_requirement"), QStringLiteral("Part shade") },
                        { QStringLiteral("watering_frequency"), QStringLiteral("4 days") },
                        { QStringLiteral("watering_notes"), QStringLiteral("Keep evenly moist") },
                        { QStringLiteral("humidity_preference"), QStringLiteral("Medium") },
                        { QStringLiteral("soil_type"), QStringLiteral("Rich loam") },
                        { QStringLiteral("last_watered"), QStringLiteral("2026-03-20") },
                        { QStringLiteral("fertilizing_schedule"), QStringLiteral("Monthly") },
                        { QStringLiteral("last_fertilized"), QStringLiteral("2026-03-01") },
                        { QStringLiteral("pruning_time"), QStringLiteral("Autumn") },
                        { QStringLiteral("pruning_notes"), QStringLiteral("") },
                        { QStringLiteral("last_pruned"), QStringLiteral("2025-10-01") },
                        { QStringLiteral("growth_rate"), QStringLiteral("Medium") },
                        { QStringLiteral("issues_pests"), QStringLiteral("") },
                        { QStringLiteral("temperature_tolerance"), QStringLiteral("Hardy") },
                        { QStringLiteral("toxic_to_pets"), QStringLiteral("No") },
                        { QStringLiteral("poisonous_to_humans"), QStringLiteral("No") },
                        { QStringLiteral("poisonous_to_pets"), QStringLiteral("No") },
                        { QStringLiteral("indoor"), QStringLiteral("No") },
                        { QStringLiteral("flowering_season"), QStringLiteral("Summer") },
                        { QStringLiteral("acquired_on"), QStringLiteral("2024-05-01") },
                        { QStringLiteral("source"), QStringLiteral("Server") },
                        { QStringLiteral("notes"), QStringLiteral("Should be deleted on sync") },
                        { QStringLiteral("created_at"), QStringLiteral("2026-03-26T13:00:00.000Z") },
                        { QStringLiteral("updated_at"), QStringLiteral("2026-03-26T13:00:00.000Z") }
                    }
                } },
                { QStringLiteral("journal_entries"), QJsonArray() },
                { QStringLiteral("reminders"), QJsonArray() },
                { QStringLiteral("plant_images"), QJsonArray() },
                { QStringLiteral("plant_care_schedules"), QJsonArray() },
                { QStringLiteral("reminder_settings"), QJsonValue::Null },
                { QStringLiteral("tombstones"), QJsonArray() }
            } },
            { QStringLiteral("conflicts"), QJsonArray() }
        });
        server.enqueueResponse(200, {
            { QStringLiteral("new_cursor"), QStringLiteral("2999-01-01T00:05:00.000Z") },
            { QStringLiteral("applied"), QJsonObject{
                { QStringLiteral("plants"), 0 },
                { QStringLiteral("journal_entries"), 0 },
                { QStringLiteral("reminders"), 0 },
                { QStringLiteral("plant_images"), 0 },
                { QStringLiteral("plant_care_schedules"), 0 },
                { QStringLiteral("reminder_settings"), 0 },
                { QStringLiteral("tombstones"), 1 }
            } },
            { QStringLiteral("server_changes"), emptyChanges() },
            { QStringLiteral("conflicts"), QJsonArray() }
        });

        SyncServerSettings settings;
        settings.baseUrl = server.baseUrl();
        settings.deviceToken = QStringLiteral("test-token");
        settings.clientId = QStringLiteral("client-delete-sequence");

        DatabaseSyncService service(db);
        QString summary;
        QString error;
        QVERIFY2(service.forcePull(&settings, &summary, &error), qPrintable(error));
        QCOMPARE(settings.lastSyncCursor, QStringLiteral("2999-01-01T00:00:00.000Z"));

        QSqlQuery deleteQuery(db);
        QVERIFY(deleteQuery.exec(QStringLiteral(
            "DELETE FROM plants WHERE sync_uuid = 'astrantia-sync-1';")));

        QVERIFY2(service.synchronize(&settings, &summary, &error), qPrintable(error));
        QCOMPARE(server.requestCount(), 2);

        const QJsonObject secondRequest = QJsonDocument::fromJson(server.requestBody(1)).object();
        const QJsonObject changesObject = secondRequest.value(QStringLiteral("changes")).toObject();
        const QJsonArray tombstones = changesObject.value(QStringLiteral("tombstones")).toArray();

        bool foundAstrantiaTombstone = false;
        for (const QJsonValue &value : tombstones) {
            const QJsonObject tombstone = value.toObject();
            if (tombstone.value(QStringLiteral("entity_type")).toString() == QStringLiteral("plants")
                && tombstone.value(QStringLiteral("sync_uuid")).toString() == QStringLiteral("astrantia-sync-1")) {
                foundAstrantiaTombstone = true;
                break;
            }
        }
        QVERIFY2(foundAstrantiaTombstone, "Deleted Astrantia plant tombstone was not uploaded during synchronize.");

        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}

QTEST_MAIN(DatabaseSyncServiceTest)
#include "test_database_sync_service.moc"
