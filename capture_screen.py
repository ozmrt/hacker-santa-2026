#!/usr/bin/env python3
"""Capture GhostESP's USB display mirror as a PNG, masked to the round aperture."""
import argparse
from pathlib import Path
import struct
import time
import zlib

import serial


def save_png(path, pixels, width, height, circular):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            inside = (2*x-width+1)**2 + (2*y-height+1)**2 <= min(width, height)**2
            index = (y*width+x)*3
            rows.extend(pixels[index:index+3] if inside or not circular else b"\x18\x18\x18")
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind+data))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def connect(port):
    connection = serial.Serial()
    connection.port, connection.baudrate, connection.timeout = port, 115200, 0.1
    connection.dtr = connection.rts = False
    connection.open()
    # Native USB may reset when macOS opens the tty. Wait until command
    # registration and display initialization have completed before use.
    end = time.monotonic() + 3
    while time.monotonic() < end:
        connection.read(4096)
    return connection


def capture(port, output, commands, connection=None):
    pixels = bytearray(240*240*3)
    covered = bytearray(240*240)
    buffer = bytearray()
    raw = bytearray()
    packets = 0
    owns_connection = connection is None
    if owns_connection:
        connection = connect(port)
    try:
        for command in commands:
            connection.write(command.encode() + b"\n")
            # Consume command output while the UI handles navigation.
            end = time.monotonic() + 0.5
            while time.monotonic() < end:
                connection.read(4096)
        connection.write(b"mirror on\n")
        deadline = time.monotonic() + 10
        complete = None
        while time.monotonic() < deadline:
            received = connection.read(8192)
            raw.extend(received)
            buffer.extend(received)
            while len(buffer) >= 17:
                start = buffer.find(struct.pack("<I", 0x47455350))
                if start < 0:
                    del buffer[:-3]
                    break
                del buffer[:start]
                if len(buffer) < 17:
                    break
                marker, cmd, x1, y1, x2, y2, count = struct.unpack_from("<IBHHHHI", buffer)
                if cmd == 1:
                    if (x1, y1) != (240, 240):
                        raise ValueError(f"Unexpected display: {x1}x{y1}")
                    del buffer[:17]
                    continue
                if cmd != 6 or x1 > x2 or y1 > y2 or x2 >= 240 or y2 >= 240 or count > 86400:
                    del buffer[0]
                    continue
                total = 17 + count + 4
                if len(buffer) < total:
                    break
                if struct.unpack_from("<I", buffer, 17+count)[0] != 0x444E4547:
                    del buffer[0]
                    continue
                data = buffer[17:17+count]
                values = []
                for i in range(0, len(data), 3):
                    values.append((data[i] << 4) | (data[i+1] >> 4))
                    if i+2 < len(data):
                        values.append(((data[i+1] & 15) << 8) | data[i+2])
                if len(values) != (x2-x1+1)*(y2-y1+1):
                    raise ValueError("Mirror tile size mismatch")
                for index, value in enumerate(values):
                    x = x1 + index % (x2-x1+1)
                    y = y1 + index // (x2-x1+1)
                    pixel = y*240+x
                    pixels[pixel*3:pixel*3+3] = bytes(((value>>8)*17, ((value>>4)&15)*17, (value&15)*17))
                    covered[pixel] = 1
                packets += 1
                del buffer[:total]
            if complete is None and all(covered):
                complete = time.monotonic()
            if complete is not None and time.monotonic() - complete > 0.5:
                break
    finally:
        connection.write(b"mirror off\n")
        end = time.monotonic() + 0.3
        while time.monotonic() < end:
            connection.read(8192)
        if owns_connection:
            connection.close()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.with_suffix(".serial.bin").write_bytes(raw)
    if not all(covered):
        print(raw[:2000].decode(errors="replace"))
        raise RuntimeError(f"Incomplete capture: {sum(covered)}/57600 pixels")
    save_png(output, pixels, 240, 240, True)
    save_png(output.with_name(output.stem + "-square.png"), pixels, 240, 240, False)
    print(f"Captured {packets} tiles: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/tty.usbmodem101")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--command", action="append", default=[])
    parser.add_argument("--interactive", action="store_true", help="Keep USB open; Enter captures, :command runs a console command, q quits")
    args = parser.parse_args()
    if args.interactive:
        connection = connect(args.port)
        try:
            index = 0
            while True:
                request = input("Enter captures; :command runs a command; q quits: ").strip()
                if request == "q":
                    break
                if request.startswith(":"):
                    connection.write(request[1:].encode() + b"\n")
                    end = time.monotonic() + 2
                    response = bytearray()
                    while time.monotonic() < end:
                        response.extend(connection.read(8192))
                    with args.output.with_suffix(".console.log").open("ab") as log:
                        log.write(response)
                    print(response.decode(errors="replace"))
                    continue
                destination = args.output.with_name(f"{args.output.stem}-{index}.png")
                try:
                    capture(args.port, destination, args.command, connection)
                except RuntimeError as error:
                    print(error)  # Keep the port open for diagnosis after a stall.
                index += 1
        finally:
            connection.close()
    else:
        capture(args.port, args.output, args.command)
