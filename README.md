# KSystemStats Intel NPU Plugin

External KSystemStats plugin that exposes an Intel NPU usage sensor in Plasma
System Monitor.

Sensor path:

```text
npu/intel/usage
```

Visible name:

```text
<detected Intel NPU> Usage
```

## Detection

The plugin looks for an Intel NPU at:

```text
/sys/class/accel/accel0/device
```

It reads:

```text
/sys/class/accel/accel0/device/uevent
```

The device is considered valid when `uevent` contains `intel_vpu`.

Known models:

```text
8086:7D1D -> Intel NPU (Meteor Lake)
8086:643E -> Intel NPU (Lunar Lake)
8086:B03E -> Intel NPU (Panther Lake)
```

The preferred usage counter is:

```text
/sys/class/accel/accel0/device/npu_busy_time_us
```

If that counter does not exist, the plugin falls back to this runtime PM
counter:

```text
/sys/devices/pci0000:00/<PCI_SLOT_NAME>/power/runtime_active_time
```

`npu_busy_time_us` represents real NPU busy time in microseconds.
`runtime_active_time` represents runtime PM active time and can include activity
that is not effective NPU work, so it is only used as a fallback approximation.

## Dependencies On Arch Linux

Install the build dependencies:

```bash
sudo pacman -S --needed base-devel cmake ninja extra-cmake-modules qt6-base qt6-tools kcoreaddons ki18n libksysguard ksystemstats
```

## Build

From the project directory:

```bash
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Per-User Installation

Install under `~/.local`:

```bash
cmake --install build --prefix "$HOME/.local"
```

The plugin will be installed at:

```text
~/.local/lib/qt6/plugins/ksystemstats/ksystemstats_plugin_intelnpu.so
```

Restart KSystemStats:

```bash
systemctl --user restart plasma-ksystemstats.service
```

If Plasma does not load plugins from `~/.local/lib/qt6/plugins`, log out and
log back in. To test it in the current session, export the path to the user
systemd environment and restart the service:

```bash
systemctl --user set-environment QT_PLUGIN_PATH="$HOME/.local/lib/qt6/plugins:$QT_PLUGIN_PATH"
systemctl --user restart plasma-ksystemstats.service
```

To make it persistent, create:

```text
~/.config/environment.d/10-qt-plugin-path.conf
```

with this content, replacing `/home/your_user` with your real home directory:

```text
QT_PLUGIN_PATH=/home/your_user/.local/lib/qt6/plugins
```

Then log out and log back in.

## System-Wide Installation

Install under `/usr`:

```bash
sudo cmake --install build --prefix /usr
```

The plugin will be installed at:

```text
/usr/lib/qt6/plugins/ksystemstats/ksystemstats_plugin_intelnpu.so
```

Restart KSystemStats:

```bash
systemctl --user restart plasma-ksystemstats.service
```

## Verification

Check that the plugin is installed in the expected location:

```bash
find "$HOME/.local/lib/qt6/plugins/ksystemstats" /usr/lib/qt6/plugins/ksystemstats -name 'ksystemstats_plugin_intelnpu.so' 2>/dev/null
```

Check whether your kernel exposes the NPU:

```bash
cat /sys/class/accel/accel0/device/uevent
```

It should contain something related to:

```text
intel_vpu
```

If the preferred counter exists, this should print a number:

```bash
cat /sys/class/accel/accel0/device/npu_busy_time_us
```

The sensor should appear in Plasma System Monitor after restarting
`plasma-ksystemstats.service`.

## Uninstall

Per-user installation:

```bash
rm -f "$HOME/.local/lib/qt6/plugins/ksystemstats/ksystemstats_plugin_intelnpu.so"
systemctl --user restart plasma-ksystemstats.service
```

System-wide installation:

```bash
sudo rm -f /usr/lib/qt6/plugins/ksystemstats/ksystemstats_plugin_intelnpu.so
systemctl --user restart plasma-ksystemstats.service
```

## Expected Behavior

On systems without an Intel NPU, without read permissions, or with incomplete
sysfs data, the plugin fails silently and does not register sensors.

Usage is calculated from two samples:

```text
usage_percent = delta_busy_time / delta_wall_time * 100
```

The result is clamped to the `0..100` range.
