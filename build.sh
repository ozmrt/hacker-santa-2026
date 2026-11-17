#!/usr/bin/env bash
set -euo pipefail
PORT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export IDF_PATH="${GHOSTESP_IDF_PATH:-$PORT_ROOT/.tools/esp-idf}"
export IDF_TOOLS_PATH="${GHOSTESP_IDF_TOOLS_PATH:-$PORT_ROOT/.tools/idf-tools}"
export IDF_COMPONENT_CACHE_PATH="$PORT_ROOT/.tools/component-cache"
export XDG_CACHE_HOME="$PORT_ROOT/.tools/cache"
export IDF_TARGET=esp32c3
mkdir -p "$PORT_ROOT/build"

# A fresh source copy has no toolchain. Install the pinned IDF and C3 tools on
# first build; later builds reuse the installed tools and the IDF build tree.
if [[ ! -f "$IDF_PATH/export.sh" ]]; then
    if [[ -n "${GHOSTESP_IDF_PATH:-}" ]]; then
        printf '%s\n' "Custom ESP-IDF checkout missing: $IDF_PATH" >&2
        exit 1
    fi
    python3 "$PORT_ROOT/setup.py"
fi
EXPECTED_IDF=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["esp_idf_revision"])' "$PORT_ROOT/upstream.json")
if [[ "$(git -C "$IDF_PATH" rev-parse HEAD)" != "$EXPECTED_IDF" ]]; then
    printf '%s\n' "ESP-IDF revision differs from upstream.json: $IDF_PATH" >&2
    exit 1
fi

load_idf() {
    source "$IDF_PATH/export.sh" > "$PORT_ROOT/build/idf-export.log" 2>&1 &&
        command -v idf.py > /dev/null &&
        command -v riscv32-esp-elf-gcc > /dev/null
}
if ! load_idf; then
    printf '%s\n' "Installing missing ESP32-C3 build tools under $IDF_TOOLS_PATH"
    bash "$IDF_PATH/install.sh" esp32c3
    if ! load_idf; then
        cat "$PORT_ROOT/build/idf-export.log" >&2
        exit 1
    fi
fi

python3 "$PORT_ROOT/prepare.py"
cd "$PORT_ROOT/upstream"
if [[ $# -eq 0 ]]; then set -- build; fi
idf.py -B "$PORT_ROOT/build/idf" \
    -D "SDKCONFIG=$PORT_ROOT/build/sdkconfig" \
    -D "SDKCONFIG_DEFAULTS=$PORT_ROOT/build/sdkconfig.defaults" \
    "$@"
case " $* " in
    *" build "*|*" all "*)
        python "$PORT_ROOT/verify.py"
        python "$PORT_ROOT/package.py"
        ;;
esac
