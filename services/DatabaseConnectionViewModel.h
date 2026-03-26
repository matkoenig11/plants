#pragma once

#include <QObject>
#include <QSqlDatabase>

#include "DatabaseSyncService.h"
#include "SyncServerSettingsRepository.h"

class DatabaseConnectionViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString deviceToken READ deviceToken WRITE setDeviceToken NOTIFY settingsChanged)
    Q_PROPERTY(QString deviceLabel READ deviceLabel WRITE setDeviceLabel NOTIFY settingsChanged)
    Q_PROPERTY(QString localDatabasePath READ localDatabasePath CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastSyncSummary READ lastSyncSummary NOTIFY lastSyncSummaryChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool connectionOk READ connectionOk NOTIFY connectionStatusChanged)

public:
    explicit DatabaseConnectionViewModel(QSqlDatabase localDatabase, QObject *parent = nullptr);

    QString baseUrl() const;
    QString deviceToken() const;
    QString deviceLabel() const;
    QString localDatabasePath() const;
    QString lastError() const;
    QString lastSyncSummary() const;
    QString connectionStatus() const;
    bool connectionOk() const;

    void setBaseUrl(const QString &value);
    void setDeviceToken(const QString &value);
    void setDeviceLabel(const QString &value);

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool testConnection();
    Q_INVOKABLE bool clearLocalDatabase();
    Q_INVOKABLE bool forcePull();
    Q_INVOKABLE bool synchronize();

signals:
    void settingsChanged();
    void lastErrorChanged();
    void lastSyncSummaryChanged();
    void connectionStatusChanged();
    void synchronizationFinished(bool success);
    void localDatabaseCleared(bool success);

private:
    void setLastError(const QString &message);
    void setLastSyncSummary(const QString &summary);
    void setConnectionStatus(const QString &message, bool ok);
    SyncServerSettings currentSettings() const;
    bool validateSettings(QString *errorMessage) const;

    SyncServerSettingsRepository m_repository;
    DatabaseSyncService m_syncService;
    SyncServerSettings m_settings;
    QString m_localDatabasePath;
    QString m_lastError;
    QString m_lastSyncSummary;
    QString m_connectionStatus;
    bool m_connectionOk = false;
};
