#include "ReminderScheduler.h"

QVector<int> ReminderScheduler::dueReminderIds(const QVector<Reminder> &reminders,
                                               const ReminderSettings &settings,
                                               const QDate &date) const
{
    QVector<int> due;
    if (!date.isValid()) {
        return due;
    }

    for (const Reminder &reminder : reminders) {
        if (!reminder.nextDueDate.isValid()) {
            continue;
        }

        if (settings.dayOfEnabled && reminder.nextDueDate == date) {
            due.append(reminder.id);
            continue;
        }

        if (settings.dayBeforeEnabled && reminder.nextDueDate.addDays(-1) == date) {
            due.append(reminder.id);
            continue;
        }

        if (settings.overdueEnabled && date > reminder.nextDueDate) {
            const int daysLate = reminder.nextDueDate.daysTo(date);
            if (daysLate > 0 && settings.overdueCadenceDays > 0 && daysLate % settings.overdueCadenceDays == 0) {
                due.append(reminder.id);
            }
        }
    }

    return due;
}
