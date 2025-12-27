#include "ReminderSettingsViewModel.h"

ReminderSettingsViewModel::ReminderSettingsViewModel(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_repository(db)
{
    reload();
}

bool ReminderSettingsViewModel::dayBeforeEnabled() const { return m_settings.dayBeforeEnabled; }
QString ReminderSettingsViewModel::dayBeforeTime() const { return m_settings.dayBeforeTime; }
bool ReminderSettingsViewModel::dayOfEnabled() const { return m_settings.dayOfEnabled; }
QString ReminderSettingsViewModel::dayOfTime() const { return m_settings.dayOfTime; }
bool ReminderSettingsViewModel::overdueEnabled() const { return m_settings.overdueEnabled; }
int ReminderSettingsViewModel::overdueCadenceDays() const { return m_settings.overdueCadenceDays; }
QString ReminderSettingsViewModel::overdueTime() const { return m_settings.overdueTime; }
QString ReminderSettingsViewModel::quietHoursStart() const { return m_settings.quietHoursStart; }
QString ReminderSettingsViewModel::quietHoursEnd() const { return m_settings.quietHoursEnd; }
QString ReminderSettingsViewModel::lastError() const { return m_lastError; }

void ReminderSettingsViewModel::setDayBeforeEnabled(bool value)
{
    if (m_settings.dayBeforeEnabled == value) {
        return;
    }
    m_settings.dayBeforeEnabled = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setDayBeforeTime(const QString &value)
{
    if (m_settings.dayBeforeTime == value) {
        return;
    }
    m_settings.dayBeforeTime = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setDayOfEnabled(bool value)
{
    if (m_settings.dayOfEnabled == value) {
        return;
    }
    m_settings.dayOfEnabled = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setDayOfTime(const QString &value)
{
    if (m_settings.dayOfTime == value) {
        return;
    }
    m_settings.dayOfTime = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setOverdueEnabled(bool value)
{
    if (m_settings.overdueEnabled == value) {
        return;
    }
    m_settings.overdueEnabled = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setOverdueCadenceDays(int value)
{
    if (m_settings.overdueCadenceDays == value) {
        return;
    }
    m_settings.overdueCadenceDays = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setOverdueTime(const QString &value)
{
    if (m_settings.overdueTime == value) {
        return;
    }
    m_settings.overdueTime = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setQuietHoursStart(const QString &value)
{
    if (m_settings.quietHoursStart == value) {
        return;
    }
    m_settings.quietHoursStart = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::setQuietHoursEnd(const QString &value)
{
    if (m_settings.quietHoursEnd == value) {
        return;
    }
    m_settings.quietHoursEnd = value;
    emit settingsChanged();
}

void ReminderSettingsViewModel::reload()
{
    m_settings = m_repository.load();
    emit settingsChanged();
    setLastError(QString());
}

bool ReminderSettingsViewModel::save()
{
    if (!m_repository.save(m_settings)) {
        setLastError(QStringLiteral("Failed to save reminder settings."));
        return false;
    }

    setLastError(QString());
    return true;
}

void ReminderSettingsViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}
