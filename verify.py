#!/usr/bin/env python3
"""Check resolved hardware configuration, partition fit, and touch decoding."""
from pathlib import Path
import struct
import subprocess
import tempfile
import re

from prepare import ROOT, config_entries


def main():
    build = ROOT / "build"
    # Reconstruct every patched file from the pinned revision, then apply the
    # saved patch to prove that a fresh checkout reproduces the compiled source.
    patch = ROOT / "board.patch"
    names = re.findall(r"^\+\+\+ b/(.+)$", patch.read_text(), re.MULTILINE)
    source = ROOT / "upstream"
    changed = set(subprocess.run(
        ["git", "-C", str(source), "diff", "--name-only", "HEAD"],
        capture_output=True, text=True, check=True).stdout.splitlines())
    changed.update(subprocess.run(
        ["git", "-C", str(source), "ls-files", "--others", "--exclude-standard"],
        capture_output=True, text=True, check=True).stdout.splitlines())
    changed.discard("dependencies.lock")  # copied from the separately packaged lock
    if changed != set(names):
        raise SystemExit(f"Source patch coverage differs: missing={sorted(changed - set(names))}, "
                         f"extra={sorted(set(names) - changed)}")
    with tempfile.TemporaryDirectory(prefix="ghostesp-patch-") as temporary:
        clean = Path(temporary)
        for name in names:
            destination = clean / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            original = subprocess.run(["git", "-C", str(ROOT / "upstream"), "show", f"HEAD:{name}"],
                                      capture_output=True)
            if original.returncode == 0:
                destination.write_bytes(original.stdout)
        subprocess.run(["git", "apply", str(patch)], cwd=clean, check=True)
        for name in names:
            if (clean / name).read_bytes() != (ROOT / "upstream" / name).read_bytes():
                raise SystemExit(f"Source differs from saved board.patch: {name}")
    print(f"Source patch reproduced: {len(names)} files")
    if "#define ROUND_CREW_PROFILE 0" not in (ROOT / "upstream/include/gui/round_demo.h").read_text():
        raise SystemExit("Development-only Copper Dream profiling copies enabled in release")
    config = config_entries((build / "sdkconfig").read_text())
    expected = {
        "IDF_TARGET": '"esp32c3"', "SUNTON_ESP32_2424S012": "y",
        "GHOSTESP_ROUND_DISPLAY": "y", "WITH_SCREEN": "y", "USE_TOUCHSCREEN": "y",
        "ESPTOOLPY_FLASHSIZE": '"4MB"', "ESP_CONSOLE_USB_SERIAL_JTAG": "y",
        "ESP_WIFI_STATIC_TX_BUFFER_NUM": "6",
        "LOG_MAXIMUM_LEVEL": "3", "LOG_DEFAULT_LEVEL": "2",
        "COMPILER_OPTIMIZATION_ASSERTIONS_SILENT": "y",
        "COMPILER_OPTIMIZATION_ASSERTION_LEVEL": "1",
        "COMPILER_OPTIMIZATION_CHECKS_SILENT": "y",
        "LV_HOR_RES_MAX": "240", "LV_VER_RES_MAX": "240",
        "LV_TFT_DISPLAY_CONTROLLER_GC9A01": "y", "LV_DISPLAY_ORIENTATION": "0",
        "LV_INVERT_COLORS": "y", "LV_DISP_SPI_CLK": "6", "LV_DISP_SPI_MOSI": "7",
        "LV_DISP_SPI_CS": "10", "LV_DISP_PIN_DC": "2", "LV_DISP_PIN_BCKL": "3",
        "LV_BACKLIGHT_ACTIVE_LVL": "y", "LV_TFT_CUSTOM_SPI_CLK_DIVIDER": "4",
        "LV_TOUCH_CONTROLLER_CST820": "y", "LV_I2C_TOUCH_PORT": "0",
        "I2C_MANAGER_0_SDA": "4", "I2C_MANAGER_0_SCL": "5",
        "I2C_MANAGER_0_FREQ_HZ": "100000", "LV_INDEV_DEF_READ_PERIOD": "10",
        "RED_RGB_PIN": "-1", "GREEN_RGB_PIN": "-1", "BLUE_RGB_PIN": "-1",
    }
    for key, value in expected.items():
        key = "CONFIG_" + key
        if config.get(key) != f"{key}={value}":
            raise SystemExit(f"Unexpected resolved config: {key}: {config.get(key)}")
    for key in ("SPIRAM", "HAS_GPS", "HAS_NFC", "HAS_INFRARED", "HAS_BADUSB",
                "HAS_BATTERY", "USE_ENCODER", "USE_JOYSTICK", "USING_SPI", "USING_MMC",
                "USING_MMC_1_BIT", "LV_DISP_USE_RST", "ENABLE_NATIVE_SD_APPS",
                "ENABLE_GHOSTSCRIPT", "CROWPANEL_1P28_ROTARY", "ESP_CONSOLE_UART_DEFAULT"):
        key = "CONFIG_" + key
        if config.get(key) == f"{key}=y":
            raise SystemExit(f"Unexpected peripheral enabled: {key}")
    table = (build / "idf/partition_table/partition-table.bin").read_bytes()
    apps = []
    for pos in range(0, len(table), 32):
        magic, kind, subtype, offset, size, label, flags = struct.unpack_from("<HBBII16sI", table, pos)
        if magic == 0x50AA and kind == 0:
            apps.append((offset, size))
    if len(apps) != 1 or apps[0][0] != 0x10000:
        raise SystemExit(f"Unexpected app partitions: {apps}")
    size = (build / "idf/Ghost_ESP_IDF.bin").stat().st_size
    if size > apps[0][1] or apps[0][0] + apps[0][1] > 4 * 1024 * 1024:
        raise SystemExit("Firmware does not fit this board's flash")
    test = build / "touch_frame_test"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I",
                    str(ROOT / "upstream/components/lvgl_esp32_drivers/lvgl_touch"),
                    str(ROOT / "tests/touch_frame_test.c"), "-o", str(test)], check=True)
    subprocess.run([str(test)], check=True)
    geometry_test = build / "round_geometry_test"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I",
                    str(ROOT / "upstream/include"), str(ROOT / "tests/round_geometry_test.c"),
                    "-o", str(geometry_test)], check=True)
    subprocess.run([str(geometry_test)], check=True)
    print(f"Board config and partition verified: {size} bytes, {apps[0][1] - size} bytes free")


if __name__ == "__main__":
    main()
