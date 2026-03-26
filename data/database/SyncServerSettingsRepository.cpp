#include "SyncServerSettingsRepository.h"

#include <QSettings>
#include <QUuid>

namespace {
constexpr auto kSettingsGroup = "syncServer";

QString ensureClientId(QString clientId)
{
    clientId = clientId.trimmed();
    if (!clientId.isEmpty()) {
        return clientId;
    }

    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
}

SyncServerSettings SyncServerSettingsRepository::load() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kSettingsGroup));

    SyncServerSettings result;
    result.baseUrl = settings.value(QStringLiteral("baseUrl")).toString();
    result.deviceToken = settings.value(QStringLiteral("deviceToken")).toString();
    result.deviceLabel = settings.value(QStringLiteral("deviceLabel")).toString();
    result.lastSyncCursor = settings.value(QStringLiteral("lastSyncCursor")).toString();
    result.clientId = ensureClientId(settings.value(QStringLiteral("clientId")).toString());

    settings.endGroup();
    return result;
}

bool SyncServerSettingsRepository::save(const SyncServerSettings &syncSettings) const
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kSettingsGroup));
    settings.setValue(QStringLiteral("baseUrl"), syncSettings.baseUrl.trimmed());
    settings.setValue(QStringLiteral("deviceToken"), syncSettings.deviceToken.trimmed());
    settings.setValue(QStringLiteral("deviceLabel"), syncSettings.deviceLabel.trimmed());
    settings.setValue(QStringLiteral("lastSyncCursor"), syncSettings.lastSyncCursor.trimmed());
    settings.setValue(QStringLiteral("clientId"), ensureClientId(syncSettings.clientId));
    settings.endGroup();
    settings.sync();
    return settings.status() == QSettings::NoError;
}
