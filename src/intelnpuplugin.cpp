/*
    SPDX-FileCopyrightText: 2026 Intel NPU KSystemStats contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "intelnpuplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorProperty.h>

K_PLUGIN_CLASS_WITH_JSON(IntelNpuPlugin, "intelnpuplugin.json")

IntelNpuPlugin::IntelNpuPlugin(QObject *parent, const QVariantList &args)
    : KSysGuard::SensorPlugin(parent, args)
{
    if (!m_backend.detect()) {
        return;
    }

    auto *container = new KSysGuard::SensorContainer(QStringLiteral("npu"), i18n("NPU"), this);
    auto *object = new KSysGuard::SensorObject(QStringLiteral("intel"), m_backend.deviceName(), container);

    m_usageSensor = new KSysGuard::SensorProperty(QStringLiteral("usage"), i18n("%1 Usage", m_backend.deviceName()), 0.0, object);
    m_usageSensor->setShortName(i18n("Usage"));
    m_usageSensor->setDescription(i18n("Intel NPU usage"));
    m_usageSensor->setUnit(KSysGuard::UnitPercent);
    m_usageSensor->setMin(0);
    m_usageSensor->setMax(100);
    m_usageSensor->setVariantType(QVariant::Double);
    object->addProperty(m_usageSensor);

    container->addObject(object);
    addContainer(container);
}

QString IntelNpuPlugin::providerName() const
{
    return QStringLiteral("intelnpu");
}

void IntelNpuPlugin::update()
{
    if (!m_usageSensor) {
        return;
    }

    const auto usage = m_backend.sampleUsagePercent();
    if (!usage) {
        return;
    }

    m_usageSensor->setValue(*usage);
}

#include "intelnpuplugin.moc"
