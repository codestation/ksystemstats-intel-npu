/*
    SPDX-FileCopyrightText: 2026 Intel NPU KSystemStats contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QString>

#include <chrono>
#include <functional>
#include <optional>

class IntelNpuBackend
{
public:
    enum class CounterUnit {
        Microseconds,
        Milliseconds,
    };

    struct Device {
        QString name;
        QString counterPath;
        CounterUnit counterUnit = CounterUnit::Milliseconds;
        bool valid = false;
    };

    using FileReader = std::function<std::optional<QString>(const QString &)>;
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    explicit IntelNpuBackend(QString devicePath = QStringLiteral("/sys/class/accel/accel0/device"),
                             FileReader fileReader = {},
                             Clock clock = {});

    bool detect();
    bool isValid() const;
    QString deviceName() const;
    Device device() const;

    std::optional<double> sampleUsagePercent();

private:
    static std::optional<QString> readFile(const QString &path);
    static QHash<QString, QString> parseUevent(const QString &uevent);
    static QString deviceNameForPciId(const QString &pciId);

    std::optional<quint64> readCounterMilliseconds() const;

    QString m_devicePath;
    FileReader m_fileReader;
    Clock m_clock;
    Device m_device;
    std::optional<quint64> m_previousCounterMilliseconds;
    std::optional<std::chrono::steady_clock::time_point> m_previousTimestamp;
};
