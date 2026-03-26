#include "DatabaseConnectionViewModel.h"

#include <QUrl>

DatabaseConnectionViewModel::DatabaseConnectionViewModel(QSqlDatabase localDatabase, QObject *parent)
    : QObject(parent)
    , m_syncService(localDatabase)
    , m_localDatabasePath(localDatabase.databaseName())
{
    reload();
}

QString DatabaseConnectionViewModel::baseUrl() const { return m_settings.baseUrl; }
QString DatabaseConnectionViewModel::deviceToken() const { return m_settings.deviceToken; }
QString DatabaseConnectionViewModel::deviceLabel() const { return m_settings.deviceLabel; }
QString DatabaseConnectionViewModel::localDatabasePath() const { return m_localDatabasePath; }
QString DatabaseConnectionViewModel::lastError() const { return m_lastError; }
QString DatabaseConnectionViewModel::lastSyncSummary() const { return m_lastSyncSummary; }
QString DatabaseConnectionViewModel::connectionStatus() const { return m_connectionStatus; }
bool DatabaseConnectionViewModel::connectionOk() const { return m_connectionOk; }

void DatabaseConnectionViewModel::setBaseUrl(const QString &value)
{
    if (m_settings.baseUrl == value) {
        return;
    }
    m_settings.baseUrl = value;
    setConnectionStatus(QString(), false);
    emit settingsChanged();
}

void DatabaseConnectionViewModel::setDeviceToken(const QString &value)
{
    if (m_settings.deviceToken == value) {
        return;
    }
    m_settings.deviceToken = value;
    setConnectionStatus(QString(), false);
    emit settingsChanged();
}

void DatabaseConnectionViewModel::setDeviceLabel(const QString &value)
{
    if (m_settings.deviceLabel == value) {
        return;
    }
    m_settings.deviceLabel = value;
    setConnectionStatus(QString(), false);
    emit settingsChanged();
}

void DatabaseConnectionViewModel::reload()
{
    m_settings = m_repository.load();
    emit settingsChanged();
    setLastError(QString());
    setConnectionStatus(QString(), false);
}

bool DatabaseConnectionViewModel::save()
{
    QString error;
    if (!validateSettings(&error)) {
        setLastError(error);
        return false;
    }

    if (!m_repository.save(currentSettings())) {
        setLastError(QStringLiteral("Failed to save database connection settings."));
        return false;
    }

    setLastError(QString());
    return true;
}

bool DatabaseConnectionViewModel::testConnection()
{
    QString error;
    if (!validateSettings(&error)) {
        setLastError(error);
        setConnectionStatus(error, false);
        return false;
    }

    QString message;
    const bool ok = m_syncService.testConnection(currentSettings(), &message);
    if (!ok) {
        setLastError(message);
        setConnectionStatus(message, false);
        return false;
    }

    setLastError(QString());
    setConnectionStatus(message, true);
    return true;
}

bool DatabaseConnectionViewModel::clearLocalDatabase()
{
    QString error;
    if (!m_syncService.clearLocalDatabase(&error)) {
        setLastError(error.isEmpty() ? QStringLiteral("Clearing local database failed.") : error);
        setConnectionStatus(error.isEmpty() ? QStringLiteral("Clearing local database failed.") : error, false);
        emit localDatabaseCleared(false);
        return false;
    }

    m_settings.lastSyncCursor.clear();
    if (!m_repository.save(m_settings)) {
        setLastError(QStringLiteral("Local database was cleared, but sync settings could not be updated."));
        emit localDatabaseCleared(false);
        return false;
    }

    setLastError(QString());
    setLastSyncSummary(QStringLiteral("Local database cleared. Run Force Pull or Synchronize to repopulate from the server."));
    setConnectionStatus(QStringLiteral("Local database cleared successfully."), true);
    emit settingsChanged();
    emit localDatabaseCleared(true);
    return true;
}

bool DatabaseConnectionViewModel::forcePull()
{
    if (!save()) {
        emit synchronizationFinished(false);
        return false;
    }

    QString summary;
    QString error;
    SyncServerSettings syncSettings = currentSettings();
    const bool ok = m_syncService.forcePull(&syncSettings, &summary, &error);
    if (!ok) {
        setLastError(error.isEmpty() ? QStringLiteral("Force pull failed.") : error);
        setConnectionStatus(error.isEmpty() ? QStringLiteral("Force pull failed.") : error, false);
        emit synchronizationFinished(false);
        return false;
    }

    m_settings = syncSettings;
    if (!m_repository.save(m_settings)) {
        setLastError(QStringLiteral("Force pull succeeded, but sync settings could not be updated."));
        emit synchronizationFinished(false);
        return false;
    }

    setLastError(QString());
    setLastSyncSummary(summary);
    setConnectionStatus(QStringLiteral("Connection test passed. Force pull completed successfully."), true);
    emit settingsChanged();
    emit synchronizationFinished(true);
    return true;
}

bool DatabaseConnectionViewModel::synchronize()
{
    if (!save()) {
        emit synchronizationFinished(false);
        return false;
    }

    QString summary;
    QString error;
    SyncServerSettings syncSettings = currentSettings();
    const bool ok = m_syncService.synchronize(&syncSettings, &summary, &error);
    if (!ok) {
        setLastError(error.isEmpty() ? QStringLiteral("Synchronization failed.") : error);
        emit synchronizationFinished(false);
        return false;
    }

    m_settings = syncSettings;
    if (!m_repository.save(m_settings)) {
        setLastError(QStringLiteral("Synchronization succeeded, but sync settings could not be updated."));
        emit synchronizationFinished(false);
        return false;
    }

    setLastError(QString());
    setLastSyncSummary(summary);
    setConnectionStatus(QStringLiteral("Connection test passed. Last sync completed successfully."), true);
    emit settingsChanged();
    emit synchronizationFinished(true);
    return true;
}

void DatabaseConnectionViewModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void DatabaseConnectionViewModel::setLastSyncSummary(const QString &summary)
{
    if (m_lastSyncSummary == summary) {
        return;
    }
    m_lastSyncSummary = summary;
    emit lastSyncSummaryChanged();
}

void DatabaseConnectionViewModel::setConnectionStatus(const QString &message, bool ok)
{
    if (m_connectionStatus == message && m_connectionOk == ok) {
        return;
    }
    m_connectionStatus = message;
    m_connectionOk = ok;
    emit connectionStatusChanged();
}

SyncServerSettings DatabaseConnectionViewModel::currentSettings() const
{
    return m_settings;
}

bool DatabaseConnectionViewModel::validateSettings(QString *errorMessage) const
{
    const QUrl parsedUrl(m_settings.baseUrl.trimmed());
    if (m_settings.baseUrl.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server URL is required.");
        }
        return false;
    }

    if (!parsedUrl.isValid()
        || parsedUrl.scheme().trimmed().isEmpty()
        || parsedUrl.host().trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Sync server URL must include a scheme and host.");
        }
        return false;
    }

    if (m_settings.deviceToken.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Device token is required.");
        }
        return false;
    }

    return true;
}
