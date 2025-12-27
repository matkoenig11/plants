#include <QtTest/QtTest>

#include "ReminderScheduler.h"

class ReminderSchedulerTest : public QObject
{
    Q_OBJECT

private slots:
    void dayOf();
    void dayBefore();
    void overdueCadence();
};

void ReminderSchedulerTest::dayOf()
{
    ReminderScheduler scheduler;
    ReminderSettings settings;
    settings.dayOfEnabled = true;
    settings.dayBeforeEnabled = false;
    settings.overdueEnabled = false;

    Reminder reminder;
    reminder.id = 1;
    reminder.nextDueDate = QDate(2025, 1, 10);

    const QVector<int> due = scheduler.dueReminderIds({reminder}, settings, QDate(2025, 1, 10));
    QCOMPARE(due.size(), 1);
    QCOMPARE(due.first(), 1);
}

void ReminderSchedulerTest::dayBefore()
{
    ReminderScheduler scheduler;
    ReminderSettings settings;
    settings.dayOfEnabled = false;
    settings.dayBeforeEnabled = true;
    settings.overdueEnabled = false;

    Reminder reminder;
    reminder.id = 2;
    reminder.nextDueDate = QDate(2025, 1, 10);

    const QVector<int> due = scheduler.dueReminderIds({reminder}, settings, QDate(2025, 1, 9));
    QCOMPARE(due.size(), 1);
    QCOMPARE(due.first(), 2);
}

void ReminderSchedulerTest::overdueCadence()
{
    ReminderScheduler scheduler;
    ReminderSettings settings;
    settings.dayOfEnabled = false;
    settings.dayBeforeEnabled = false;
    settings.overdueEnabled = true;
    settings.overdueCadenceDays = 2;

    Reminder reminder;
    reminder.id = 3;
    reminder.nextDueDate = QDate(2025, 1, 1);

    const QVector<int> due = scheduler.dueReminderIds({reminder}, settings, QDate(2025, 1, 3));
    QCOMPARE(due.size(), 1);
    QCOMPARE(due.first(), 3);
}

QTEST_MAIN(ReminderSchedulerTest)
#include "test_reminder_scheduler.moc"
