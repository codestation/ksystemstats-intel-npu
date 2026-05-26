/*
    SPDX-FileCopyrightText: 2026 Intel NPU KSystemStats contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "intelnpubackend.h"

#include <systemstats/SensorPlugin.h>

namespace KSysGuard
{
class SensorObject;
class SensorProperty;
}

class IntelNpuPlugin : public KSysGuard::SensorPlugin
{
    Q_OBJECT

public:
    explicit IntelNpuPlugin(QObject *parent, const QVariantList &args);
    QString providerName() const override;
    void update() override;

private:
    IntelNpuBackend m_backend;
    KSysGuard::SensorProperty *m_usageSensor = nullptr;
};
