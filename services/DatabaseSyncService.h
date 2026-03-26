#pragma once

#include <QSqlDatabase>
#include <QString>

#include "SyncServerSettings.h"

class DatabaseSyncService
{
public:
    explicit DatabaseSyncService(QSqlDatabase localDatabase);

    bool testConnection(const SyncServerSettings &settings, QString *message);
    bool clearLocalDatabase(QString *errorMessage);
    bool forcePull(SyncServerSettings *settings, QString *summary, QString *errorMessage);
    bool synchronize(SyncServerSettings *settings, QString *summary, QString *errorMessage);

private:
    QSqlDatabase m_localDatabase;
};
