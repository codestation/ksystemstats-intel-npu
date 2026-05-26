/*
    SPDX-FileCopyrightText: 2026 Intel NPU KSystemStats contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "intelnpubackend.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>

IntelNpuBackend::IntelNpuBackend(QString devicePath, FileReader fileReader, Clock clock)
    : m_devicePath(std::move(devicePath))
    , m_fileReader(std::move(fileReader))
    , m_clock(std::move(clock))
{
    if (!m_fileReader) {
        m_fileReader = readFile;
    }
    if (!m_clock) {
        m_clock = [] {
            return std::chrono::steady_clock::now();
        };
    }
}

bool IntelNpuBackend::detect()
{
    m_device = {};
    m_previousCounterMilliseconds.reset();
    m_previousTimestamp.reset();

    const auto uevent = m_fileReader(m_devicePath + QStringLiteral("/uevent"));
    if (!uevent || !uevent->contains(QLatin1String("intel_vpu"))) {
        return false;
    }

    const auto values = parseUevent(*uevent);
    const QString pciId = values.value(QStringLiteral("PCI_ID"));
    const QString busyTimePath = m_devicePath + QStringLiteral("/npu_busy_time_us");

    if (QFileInfo::exists(busyTimePath) || m_fileReader(busyTimePath).has_value()) {
        m_device = {deviceNameForPciId(pciId), busyTimePath, CounterUnit::Microseconds, true};
        return true;
    }

    const QString pciSlotName = values.value(QStringLiteral("PCI_SLOT_NAME"));
    if (pciSlotName.isEmpty()) {
        return false;
    }

    const QString runtimeActiveTimePath = QStringLiteral("/sys/devices/pci0000:00/%1/power/runtime_active_time").arg(pciSlotName);
    if (!QFileInfo::exists(runtimeActiveTimePath) && !m_fileReader(runtimeActiveTimePath).has_value()) {
        return false;
    }

    // runtime_active_time is a power-management active time counter. It is an
    // approximation used only when the kernel does not expose npu_busy_time_us.
    m_device = {deviceNameForPciId(pciId), runtimeActiveTimePath, CounterUnit::Milliseconds, true};
    return true;
}

bool IntelNpuBackend::isValid() const
{
    return m_device.valid;
}

QString IntelNpuBackend::deviceName() const
{
    return m_device.name;
}

IntelNpuBackend::Device IntelNpuBackend::device() const
{
    return m_device;
}

std::optional<double> IntelNpuBackend::sampleUsagePercent()
{
    if (!m_device.valid) {
        return std::nullopt;
    }

    const auto counterMilliseconds = readCounterMilliseconds();
    if (!counterMilliseconds) {
        return std::nullopt;
    }

    const auto now = m_clock();
    if (!m_previousCounterMilliseconds || !m_previousTimestamp) {
        m_previousCounterMilliseconds = *counterMilliseconds;
        m_previousTimestamp = now;
        return 0.0;
    }

    const auto elapsed = std::chrono::duration<double, std::milli>(now - *m_previousTimestamp).count();
    if (elapsed <= 0.0 || *counterMilliseconds < *m_previousCounterMilliseconds) {
        m_previousCounterMilliseconds = *counterMilliseconds;
        m_previousTimestamp = now;
        return std::nullopt;
    }

    const double activeDelta = static_cast<double>(*counterMilliseconds - *m_previousCounterMilliseconds);
    m_previousCounterMilliseconds = *counterMilliseconds;
    m_previousTimestamp = now;

    return std::clamp((activeDelta / elapsed) * 100.0, 0.0, 100.0);
}

std::optional<QString> IntelNpuBackend::readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }

    return QString::fromUtf8(file.readAll()).trimmed();
}

QHash<QString, QString> IntelNpuBackend::parseUevent(const QString &uevent)
{
    QHash<QString, QString> values;
    const auto lines = uevent.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const qsizetype separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }
        values.insert(line.left(separator), line.mid(separator + 1).trimmed());
    }
    return values;
}

QString IntelNpuBackend::deviceNameForPciId(const QString &pciId)
{
    if (pciId == QLatin1String("8086:7D1D")) {
        return QStringLiteral("Intel NPU (Meteor Lake)");
    }
    if (pciId == QLatin1String("8086:643E")) {
        return QStringLiteral("Intel NPU (Lunar Lake)");
    }
    if (pciId == QLatin1String("8086:B03E")) {
        return QStringLiteral("Intel NPU (Panther Lake)");
    }
    if (!pciId.isEmpty()) {
        return QStringLiteral("Intel NPU (%1)").arg(pciId);
    }
    return QStringLiteral("Intel NPU");
}

std::optional<quint64> IntelNpuBackend::readCounterMilliseconds() const
{
    const auto rawValue = m_fileReader(m_device.counterPath);
    if (!rawValue) {
        return std::nullopt;
    }

    bool ok = false;
    const quint64 value = rawValue->trimmed().toULongLong(&ok);
    if (!ok) {
        return std::nullopt;
    }

    if (m_device.counterUnit == CounterUnit::Microseconds) {
        return value / 1000;
    }
    return value;
}
