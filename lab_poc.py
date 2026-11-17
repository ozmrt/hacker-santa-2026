#!/usr/bin/env python3
"""Collect fixed-fixture lab portal events from the board's USB console."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import time

import serial


ROOT = Path(__file__).resolve().parent
FRAME = re.compile(rb"@LABPOC1\t(paypal-sandbox|card-test-token)\t([0-9]+)\t([0-9a-fA-F]{8})\t([0-9]+)\r?$")
SSID = re.compile(r"LAB-[A-Za-z0-9_-]{1,27}$")


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_frame(line):
    match = FRAME.search(line.strip())
    if not match:
        return None
    fixture, uptime_ms, client_ipv4_raw, free_heap = match.groups()
    return {
        "type": "fixture_submitted",
        "host_time_utc": utc_now(),
        "fixture": fixture.decode("ascii"),
        "device_uptime_ms": int(uptime_ms),
        "client_ipv4_raw_hex": client_ipv4_raw.decode("ascii"),
        "free_internal_heap_bytes": int(free_heap),
    }


def write_record(file, record):
    file.write(json.dumps(record, sort_keys=True) + "\n")
    file.flush()
    os.fsync(file.fileno())


def run(port, ssid, output, duration):
    if not SSID.fullmatch(ssid):
        raise ValueError("SSID must start LAB- and contain 5-31 safe characters")
    image = ROOT / "dist/ghostesp-esp32-2424s012-app.bin"
    patch = ROOT / "board.patch"
    image_sha256, patch_sha256 = sha256(image), sha256(patch)
    output.parent.mkdir(parents=True, exist_ok=True)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    descriptor = os.open(output, flags, 0o600)
    count = 0
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as artifact:
            write_record(artifact, {
                "type": "session_started", "host_time_utc": utc_now(),
                "ssid": ssid, "board": "ESP32-2424S012",
                "application_sha256": image_sha256,
                "board_patch_sha256": patch_sha256,
                "mode": "offline fixture simulation; no PayPal or card transaction",
                "accepted_fixtures": ["paypal-sandbox", "card-test-token"],
                "synthetic_inputs": {
                    "paypal-sandbox": {
                        "identity": "lab.user@example.invalid",
                        "password": "LAB-ONLY-NOT-A-REAL-PASSWORD",
                    },
                    "card-test-token": {"token": "tok_lab_card_success"},
                },
            })
            connection = serial.Serial()
            connection.port = port
            connection.baudrate = 115200
            connection.timeout = 0.1
            connection.dtr = connection.rts = False
            connection.open()
            command_sent = False
            try:
                # Opening native USB can reset this board. Wait for CLI setup.
                ready = time.monotonic() + 3
                while time.monotonic() < ready:
                    connection.read(4096)
                connection.write(f"labportal {ssid}\n".encode("ascii"))
                command_sent = True
                deadline = time.monotonic() + 12
                started = False
                while time.monotonic() < deadline:
                    line = connection.readline()
                    if b"Lab portal started:" in line:
                        started = True
                        break
                    if b"Lab portal start failed:" in line:
                        raise RuntimeError(line.decode(errors="replace").strip())
                if not started:
                    raise RuntimeError("No lab portal start confirmation on USB")
                print(f"Lab portal running as {ssid}; listening for fixture submissions")
                end = time.monotonic() + duration if duration else None
                while end is None or time.monotonic() < end:
                    event = parse_frame(connection.readline())
                    if event:
                        write_record(artifact, event)
                        count += 1
                        print(f"Recorded {event['fixture']} fixture ({count} total)")
            except KeyboardInterrupt:
                pass
            finally:
                try:
                    if command_sent:
                        connection.write(b"stopportal\n")
                        time.sleep(0.4)
                finally:
                    connection.close()
            write_record(artifact, {"type": "session_ended", "host_time_utc": utc_now(),
                                    "submissions": count})
    except BaseException:
        # A partial artifact is useful for diagnosing a failed run. It contains
        # only fixed fixture names and board metadata, never entered secrets.
        raise
    print(f"Saved {count} fixture submissions to {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/tty.usbmodem101")
    parser.add_argument("--ssid", default="LAB-PerMyLastLogin")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--duration", type=int, default=120,
                        help="seconds to listen; 0 listens until Ctrl-C")
    args = parser.parse_args()
    if args.duration < 0:
        parser.error("duration must be nonnegative")
    run(args.port, args.ssid, args.output, args.duration)
