#pragma once

#include <QSqlDatabase>

#include "ReminderSettings.h"

class ReminderSettingsRepository
{
public:
    explicit ReminderSettingsRepository(QSqlDatabase db);

    ReminderSettings load();
    bool save(const ReminderSettings &settings);

private:
    QSqlDatabase m_db;
};
