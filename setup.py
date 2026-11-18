#!/usr/bin/env python3
"""Install the pinned ESP-IDF and C3 tools under this port's ignored .tools/."""
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
LOCK = json.loads((ROOT / "upstream.json").read_text())
IDF = ROOT / ".tools/esp-idf"


def run(*args):
    subprocess.run(args, check=True)


def main():
    if not (IDF / ".git").exists():
        IDF.mkdir(parents=True, exist_ok=True)
        run("git", "-C", str(IDF), "init")
        run("git", "-C", str(IDF), "remote", "add", "origin", "https://github.com/espressif/esp-idf.git")
        run("git", "-C", str(IDF), "fetch", "--depth=1", "origin", LOCK["esp_idf_revision"])
        run("git", "-C", str(IDF), "checkout", "--detach", "FETCH_HEAD")
    actual = subprocess.check_output(["git", "-C", str(IDF), "rev-parse", "HEAD"], text=True).strip()
    if actual != LOCK["esp_idf_revision"]:
        raise SystemExit(f"ESP-IDF revision mismatch: {actual}")
    run("git", "-C", str(IDF), "submodule", "update", "--init", "--recursive", "--depth=1")
    env = os.environ.copy()
    env.setdefault("IDF_TOOLS_PATH", str(ROOT / ".tools/idf-tools"))
    subprocess.run(["bash", str(IDF / "install.sh"), "esp32c3"], cwd=IDF, env=env, check=True)
    print("Toolchain ready. Run: bash build.sh")


if __name__ == "__main__":
    main()
