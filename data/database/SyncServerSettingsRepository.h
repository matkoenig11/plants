#pragma once

#include "SyncServerSettings.h"

class SyncServerSettingsRepository
{
public:
    SyncServerSettings load() const;
    bool save(const SyncServerSettings &settings) const;
};
