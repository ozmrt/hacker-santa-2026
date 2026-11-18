#!/usr/bin/env python3
"""Render the actual firmware kernels on the host; previews do not enter flash."""
import argparse
import ctypes
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--effect', choices=['crew', 'neon'], default='crew')
    parser.add_argument('--gif', action='store_true', help='Also make a six-second reference using ffmpeg')
    args = parser.parse_args()
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    sources = [ROOT / 'upstream/main/gui' / (name + '.c')
               for name in ['round_demo', 'round_crew', 'round_expand']]
    with tempfile.TemporaryDirectory(dir=build, prefix='demo-preview-') as directory:
        library = Path(directory) / 'raster.dylib'
        subprocess.run(['cc', '-O3', '-shared', '-fPIC', '-I', str(ROOT / 'upstream/include'),
                        *map(str, sources), '-o', str(library)], check=True)
        lib = ctypes.CDLL(str(library))
        render = getattr(lib, 'round_crew_render' if args.effect == 'crew' else 'round_demo_render')
        render.argtypes = [ctypes.POINTER(ctypes.c_uint16), ctypes.c_uint,
                           ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                           ctypes.c_uint32, ctypes.c_bool]
        pixels = (ctypes.c_uint16 * (240*240))()

        def frame(ms):
            render(pixels, 240, 0, 0, 239, 239, ms, False)
            data = bytearray()
            for i, c in enumerate(pixels):
                x, y = i % 240, i // 240
                inside = (2*x-239)**2 + (2*y-239)**2 <= 240**2
                data.extend((((c >> 11)*255//31, ((c >> 5) & 63)*255//63, (c & 31)*255//31)
                             if inside else (24, 24, 24)))
            return data

        def chunk(kind, data):
            return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind+data))

        stem = ROOT / 'docs' / ('copper-dream' if args.effect == 'crew' else 'neon-vortex')
        stem.parent.mkdir(exist_ok=True)
        data = frame(2100)
        rows = b''.join(b'\0'+data[y*720:(y+1)*720] for y in range(240))
        stem.with_suffix('.png').write_bytes(b'\x89PNG\r\n\x1a\n' +
            chunk(b'IHDR', struct.pack('>IIBBBBB', 240, 240, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
        if args.gif:
            raw = Path(directory) / 'frames.rgb'
            with raw.open('wb') as output:
                for i in range(120):
                    output.write(frame(i*50))
            subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 'rawvideo', '-pixel_format',
                'rgb24', '-video_size', '240x240', '-framerate', '20', '-i', str(raw), '-vf',
                'split[a][b];[a]palettegen=stats_mode=diff[p];[b][p]paletteuse',
                str(stem.with_suffix('.gif'))], check=True)
        print(stem)


if __name__ == '__main__':
    main()
