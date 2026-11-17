#!/usr/bin/env python3
"""Package the image with its exact build inputs and checksums."""
import hashlib
import json
import shutil
import subprocess
import sys

from prepare import ROOT, LOCK


def main():
    build = ROOT / "build/idf"
    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    args = json.loads((build / "flasher_args.json").read_text())
    merged = dist / "ghostesp-esp32-2424s012-merged.bin"
    command = [sys.executable, "-m", "esptool", "--chip", "esp32c3", "merge-bin",
               "--output", str(merged), *args["write_flash_args"]]
    for offset, filename in args["flash_files"].items():
        command.extend([offset, str(build / filename)])
    subprocess.run(command, check=True)
    files = {
        build / args["app"]["file"]: "ghostesp-esp32-2424s012-app.bin",
        build / args["bootloader"]["file"]: "bootloader.bin",
        build / args["partition-table"]["file"]: "partition-table.bin",
        ROOT / "build/sdkconfig": "sdkconfig",
        ROOT / "dependencies.lock": "dependencies.lock",
        ROOT / "upstream.json": "upstream.json",
        ROOT / "board.patch": "board.patch",
        ROOT / "upstream/LICENSE": "LICENSE.upstream",
        ROOT / "README.md": "README.md",
        ROOT / "BINARY_OPTIMISATION.md": "BINARY_OPTIMISATION.md",
        ROOT / "SCREENSAVER.md": "SCREENSAVER.md",
        ROOT / "RISC_V_OPTIMISATION.md": "RISC_V_OPTIMISATION.md",
        ROOT / "ORBITAL_VECTOR.md": "ORBITAL_VECTOR.md",
        ROOT / "ASTEROIDS.md": "ASTEROIDS.md",
        ROOT / "LAB_POC.md": "LAB_POC.md",
        ROOT / "RF_WATCH.md": "RF_WATCH.md",
        ROOT / "RECON_FOX_HUNT.md": "RECON_FOX_HUNT.md",
        ROOT / "docs/copper-dream.png": "docs/copper-dream.png",
        ROOT / "docs/copper-dream.gif": "docs/copper-dream.gif",
        ROOT / "docs/neon-vortex.png": "docs/neon-vortex.png",
        ROOT / "docs/neon-vortex.gif": "docs/neon-vortex.gif",
        ROOT / "docs/orbital-vector-menu.png": "docs/orbital-vector-menu.png",
        ROOT / "docs/orbital-vector-play.png": "docs/orbital-vector-play.png",
    }
    for source, name in files.items():
        (dist / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, dist / name)
    manifest = {
        "board": "ESP32-2424S012", "chip": "esp32c3", "flash_bytes": 4194304,
        "source": LOCK, "merged_flash_offset": "0x0", "app_flash_offset": "0x10000",
        "hardware_tested": False,
        "board_patch_sha256": hashlib.sha256((ROOT / "board.patch").read_bytes()).hexdigest(),
        "sdkconfig_sha256": hashlib.sha256((dist / "sdkconfig").read_bytes()).hexdigest(),
    }
    (dist / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    names = sorted([merged.name, *files.values(), "manifest.json"])
    (dist / "SHA256SUMS").write_text("".join(
        f"{hashlib.sha256((dist / name).read_bytes()).hexdigest()}  {name}\n" for name in names))
    print(f"Packaged firmware: {merged}")


if __name__ == "__main__":
    main()
