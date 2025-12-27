#include <QtTest/QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "ReminderRepository.h"

class ReminderRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void createWithPlants();

private:
    QSqlDatabase m_db;
};

void ReminderRepositoryTest::initTestCase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "reminders_test");
    m_db.setDatabaseName(":memory:");
    QVERIFY(m_db.open());

    QSqlQuery query(m_db);
    QVERIFY(query.exec(
        "CREATE TABLE plants (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL);"
    ));
    QVERIFY(query.exec(
        "CREATE TABLE reminders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "custom_name TEXT,"
        "task_type TEXT NOT NULL,"
        "schedule_type TEXT NOT NULL,"
        "interval_days INTEGER,"
        "start_date TEXT,"
        "next_due_date TEXT NOT NULL,"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT (datetime('now')),"
        "updated_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"
    ));
    QVERIFY(query.exec(
        "CREATE TABLE reminder_plants ("
        "reminder_id INTEGER NOT NULL,"
        "plant_id INTEGER NOT NULL,"
        "PRIMARY KEY (reminder_id, plant_id)"
        ");"
    ));

    QVERIFY(query.exec("INSERT INTO plants (name) VALUES ('A');"));
    QVERIFY(query.exec("INSERT INTO plants (name) VALUES ('B');"));
}

void ReminderRepositoryTest::createWithPlants()
{
    ReminderRepository repo(m_db);
    Reminder reminder;
    reminder.customName = "Morning water";
    reminder.taskType = "Water";
    reminder.scheduleType = "fixed";
    reminder.nextDueDate = QDate(2025, 1, 10);

    const int id = repo.create(reminder, {1, 2});
    QVERIFY(id > 0);

    const Reminder loaded = repo.findById(id);
    QCOMPARE(loaded.customName, QString("Morning water"));
    QCOMPARE(loaded.taskType, QString("Water"));

    const QVector<int> plantIds = repo.plantIdsForReminder(id);
    QCOMPARE(plantIds.size(), 2);
}

QTEST_MAIN(ReminderRepositoryTest)
#include "test_reminder_repository.moc"
