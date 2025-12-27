#include "ReminderSettingsRepository.h"

#include <QSqlQuery>
#include <QVariant>

ReminderSettingsRepository::ReminderSettingsRepository(QSqlDatabase db)
    : m_db(db)
{
}

ReminderSettings ReminderSettingsRepository::load()
{
    ReminderSettings settings;
    if (!m_db.isOpen()) {
        return settings;
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            "SELECT day_before_enabled, day_before_time, day_of_enabled, day_of_time, "
            "overdue_enabled, overdue_cadence_days, overdue_time, quiet_hours_start, quiet_hours_end "
            "FROM reminder_settings WHERE id = 1;")) {
        return settings;
    }

    if (!query.next()) {
        return settings;
    }

    settings.dayBeforeEnabled = query.value(0).toInt() != 0;
    settings.dayBeforeTime = query.value(1).toString();
    settings.dayOfEnabled = query.value(2).toInt() != 0;
    settings.dayOfTime = query.value(3).toString();
    settings.overdueEnabled = query.value(4).toInt() != 0;
    settings.overdueCadenceDays = query.value(5).toInt();
    settings.overdueTime = query.value(6).toString();
    settings.quietHoursStart = query.value(7).toString();
    settings.quietHoursEnd = query.value(8).toString();
    return settings;
}

bool ReminderSettingsRepository::save(const ReminderSettings &settings)
{
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE reminder_settings SET "
        "day_before_enabled = :day_before_enabled, "
        "day_before_time = :day_before_time, "
        "day_of_enabled = :day_of_enabled, "
        "day_of_time = :day_of_time, "
        "overdue_enabled = :overdue_enabled, "
        "overdue_cadence_days = :overdue_cadence_days, "
        "overdue_time = :overdue_time, "
        "quiet_hours_start = :quiet_hours_start, "
        "quiet_hours_end = :quiet_hours_end "
        "WHERE id = 1;"
    );
    query.bindValue(":day_before_enabled", settings.dayBeforeEnabled ? 1 : 0);
    query.bindValue(":day_before_time", settings.dayBeforeTime);
    query.bindValue(":day_of_enabled", settings.dayOfEnabled ? 1 : 0);
    query.bindValue(":day_of_time", settings.dayOfTime);
    query.bindValue(":overdue_enabled", settings.overdueEnabled ? 1 : 0);
    query.bindValue(":overdue_cadence_days", settings.overdueCadenceDays);
    query.bindValue(":overdue_time", settings.overdueTime);
    query.bindValue(":quiet_hours_start", settings.quietHoursStart);
    query.bindValue(":quiet_hours_end", settings.quietHoursEnd);

    return query.exec();
}
