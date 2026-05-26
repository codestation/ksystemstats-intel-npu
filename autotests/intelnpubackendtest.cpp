/*
    SPDX-FileCopyrightText: 2026 Intel NPU KSystemStats contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "intelnpubackend.h"

#include <QHash>
#include <QObject>
#include <QTest>

using namespace std::chrono_literals;

class IntelNpuBackendTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void detectsBusyTimeCounter();
    void fallsBackToRuntimeActiveTime();
    void rejectsNonIntelVpu();
    void calculatesUsageFromSuccessiveSamples();
    void rejectsInvalidCounterValues();
};

void IntelNpuBackendTest::detectsBusyTimeCounter()
{
    QHash<QString, QString> files = {
        {QStringLiteral("/device/uevent"), QStringLiteral("DRIVER=intel_vpu\nPCI_ID=8086:7D1D\nPCI_SLOT_NAME=0000:00:0b.0\n")},
        {QStringLiteral("/device/npu_busy_time_us"), QStringLiteral("1000")},
    };

    IntelNpuBackend backend(QStringLiteral("/device"), [&files](const QString &path) -> std::optional<QString> {
        if (!files.contains(path)) {
            return std::nullopt;
        }
        return files.value(path);
    });

    QVERIFY(backend.detect());
    QCOMPARE(backend.deviceName(), QStringLiteral("Intel NPU (Meteor Lake)"));
    QCOMPARE(backend.device().counterPath, QStringLiteral("/device/npu_busy_time_us"));
    QCOMPARE(backend.device().counterUnit, IntelNpuBackend::CounterUnit::Microseconds);
}

void IntelNpuBackendTest::fallsBackToRuntimeActiveTime()
{
    QHash<QString, QString> files = {
        {QStringLiteral("/device/uevent"), QStringLiteral("DRIVER=intel_vpu\nPCI_ID=8086:643E\nPCI_SLOT_NAME=0000:00:0b.0\n")},
        {QStringLiteral("/sys/devices/pci0000:00/0000:00:0b.0/power/runtime_active_time"), QStringLiteral("10")},
    };

    IntelNpuBackend backend(QStringLiteral("/device"), [&files](const QString &path) -> std::optional<QString> {
        if (!files.contains(path)) {
            return std::nullopt;
        }
        return files.value(path);
    });

    QVERIFY(backend.detect());
    QCOMPARE(backend.deviceName(), QStringLiteral("Intel NPU (Lunar Lake)"));
    QCOMPARE(backend.device().counterPath, QStringLiteral("/sys/devices/pci0000:00/0000:00:0b.0/power/runtime_active_time"));
    QCOMPARE(backend.device().counterUnit, IntelNpuBackend::CounterUnit::Milliseconds);
}

void IntelNpuBackendTest::rejectsNonIntelVpu()
{
    QHash<QString, QString> files = {
        {QStringLiteral("/device/uevent"), QStringLiteral("DRIVER=other\nPCI_ID=8086:7D1D\n")},
    };

    IntelNpuBackend backend(QStringLiteral("/device"), [&files](const QString &path) -> std::optional<QString> {
        if (!files.contains(path)) {
            return std::nullopt;
        }
        return files.value(path);
    });

    QVERIFY(!backend.detect());
    QVERIFY(!backend.isValid());
}

void IntelNpuBackendTest::calculatesUsageFromSuccessiveSamples()
{
    auto now = std::chrono::steady_clock::time_point{};
    QString counter = QStringLiteral("1000000");
    QHash<QString, QString> files = {
        {QStringLiteral("/device/uevent"), QStringLiteral("DRIVER=intel_vpu\nPCI_ID=8086:B03E\n")},
        {QStringLiteral("/device/npu_busy_time_us"), counter},
    };

    IntelNpuBackend backend(QStringLiteral("/device"),
                            [&files, &counter](const QString &path) -> std::optional<QString> {
                                if (!files.contains(path)) {
                                    return std::nullopt;
                                }
                                if (path.endsWith(QLatin1String("npu_busy_time_us"))) {
                                    return counter;
                                }
                                return files.value(path);
                            },
                            [&now] {
                                return now;
                            });

    QVERIFY(backend.detect());
    QCOMPARE(backend.sampleUsagePercent().value(), 0.0);

    now += 100ms;
    counter = QStringLiteral("1050000");
    QCOMPARE(backend.sampleUsagePercent().value(), 50.0);

    now += 100ms;
    counter = QStringLiteral("1250000");
    QCOMPARE(backend.sampleUsagePercent().value(), 100.0);
}

void IntelNpuBackendTest::rejectsInvalidCounterValues()
{
    QHash<QString, QString> files = {
        {QStringLiteral("/device/uevent"), QStringLiteral("DRIVER=intel_vpu\nPCI_ID=8086:7D1D\n")},
        {QStringLiteral("/device/npu_busy_time_us"), QStringLiteral("not-a-number")},
    };

    IntelNpuBackend backend(QStringLiteral("/device"), [&files](const QString &path) -> std::optional<QString> {
        if (!files.contains(path)) {
            return std::nullopt;
        }
        return files.value(path);
    });

    QVERIFY(backend.detect());
    QVERIFY(!backend.sampleUsagePercent().has_value());
}

QTEST_MAIN(IntelNpuBackendTest)

#include "intelnpubackendtest.moc"
