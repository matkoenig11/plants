#pragma once

#include <QString>

struct SyncServerSettings
{
    QString baseUrl;
    QString deviceToken;
    QString deviceLabel;
    QString lastSyncCursor;
    QString clientId;

    bool isConfigured() const
    {
        return !baseUrl.trimmed().isEmpty()
            && !deviceToken.trimmed().isEmpty();
    }
};
