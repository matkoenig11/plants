#pragma once

#include <QObject>
#include <QSqlDatabase>

#include "ReminderSettings.h"
#include "ReminderSettingsRepository.h"

class ReminderSettingsViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dayBeforeEnabled READ dayBeforeEnabled WRITE setDayBeforeEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString dayBeforeTime READ dayBeforeTime WRITE setDayBeforeTime NOTIFY settingsChanged)
    Q_PROPERTY(bool dayOfEnabled READ dayOfEnabled WRITE setDayOfEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString dayOfTime READ dayOfTime WRITE setDayOfTime NOTIFY settingsChanged)
    Q_PROPERTY(bool overdueEnabled READ overdueEnabled WRITE setOverdueEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int overdueCadenceDays READ overdueCadenceDays WRITE setOverdueCadenceDays NOTIFY settingsChanged)
    Q_PROPERTY(QString overdueTime READ overdueTime WRITE setOverdueTime NOTIFY settingsChanged)
    Q_PROPERTY(QString quietHoursStart READ quietHoursStart WRITE setQuietHoursStart NOTIFY settingsChanged)
    Q_PROPERTY(QString quietHoursEnd READ quietHoursEnd WRITE setQuietHoursEnd NOTIFY settingsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ReminderSettingsViewModel(QSqlDatabase db, QObject *parent = nullptr);

    bool dayBeforeEnabled() const;
    QString dayBeforeTime() const;
    bool dayOfEnabled() const;
    QString dayOfTime() const;
    bool overdueEnabled() const;
    int overdueCadenceDays() const;
    QString overdueTime() const;
    QString quietHoursStart() const;
    QString quietHoursEnd() const;
    QString lastError() const;

    void setDayBeforeEnabled(bool value);
    void setDayBeforeTime(const QString &value);
    void setDayOfEnabled(bool value);
    void setDayOfTime(const QString &value);
    void setOverdueEnabled(bool value);
    void setOverdueCadenceDays(int value);
    void setOverdueTime(const QString &value);
    void setQuietHoursStart(const QString &value);
    void setQuietHoursEnd(const QString &value);

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool save();

signals:
    void settingsChanged();
    void lastErrorChanged();

private:
    void setLastError(const QString &message);

    ReminderSettingsRepository m_repository;
    ReminderSettings m_settings;
    QString m_lastError;
};
