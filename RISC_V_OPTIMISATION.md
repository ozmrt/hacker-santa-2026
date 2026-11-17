# C3 instruction, memory and DMA experiments

These are measurements from this ESP32-2424S012, using the pinned IDF/compiler.
The aim is to improve the whole application, not maximize assembly use. The
first handwritten palette kernel is smaller but slower than optimized C and
is retained only for the explicit `saver kernels` diagnostic.

## Checking the proposed techniques

| Proposal | Finding and decision |
|---|---|
| Fixed point everywhere | Use it in the render/simulation loops. Q7/Q8/Q15 fit these ranges better than automatically choosing 16.16. Integer arithmetic can still overflow or require multiple instructions. Float elsewhere need not be rewritten without profiling. |
| `-march=rv32imc_zba_zbb` | **Unsupported on C3.** It is RV32IMC; do not enable Zba/Zbb. `sh1add`, `sh2add`, `clz` and `ror` are not C3 hardware instructions. The linked ELF architecture attributes were checked. |
| Global `-O3` | Keep `-Os` elsewhere. Compare only the scene/expander. Higher optimization can increase code/cache/IRAM use substantially. |
| Zero-overhead branches | C3 has ordinary branches, not zero-overhead hardware loops. Branchless code may add work. Move decisions to row setup, pack multiple pixels per iteration, and measure. |
| Pin every hot function/table | Pin only the winning 122-byte C expander. Tables remain in flash, except the 16-color working palette on the stack. Blind pinning would consume the RAM recovered from the unused peer code and checks. |
| 2/4-bit packed tiles save SPI bandwidth | They save source asset/scratch memory. This panel still receives RGB565. Copper Dream packs 120 logical pixels into 60 bytes, expands to a 240-pixel scanline, then reuses a second row. It does not send 4-bit pixels to the LCD. |
| GDMA ping-pong | SPI2 DMA was already enabled. Added a second five-line internal DMA buffer and a completion semaphore, keeping the existing IDF SPI driver. CPU rendering can overlap the previous payload transfer; it still prepares commands and handles completion. |

The [C3 datasheet](https://documentation.espressif.com/esp32-c3_datasheet_en.html)
specifies RV32IMC, SPI2 DMA and a 16 KB cache within the 400 KB SRAM total.
The [IDF SPI master documentation](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32c3/api-reference/peripherals/spi_master.html)
explains DMA-capable alignment, queued transfers, and coordination with polling
transactions. The shipped ELF's architecture string was:
`rv32i2p1_m2p0_c2p0_zicsr2p0_zifencei2p0_zmmul1p0_zca1p0`.
Zmmul/Zca are subsets related to the multiply/compressed capabilities, not Zba/Zbb.

## Actual assembly candidate: packed palette expansion

`main/gui/round_expand.c` contains reference C and an RV32IMC inline-assembly
prototype. One packed source byte contains two indices. Each index becomes a
pair of RGB565 pixels: four output pixels per iteration. Aligned C uses word
stores through `memcpy` with an explicit alignment assumption; an unaligned
fallback avoids unsafe word stores. The ASM path also checks alignment.

The explicit diagnostic checks all 256 packed-byte patterns, compares output,
and times 32 calls expanding 60 bytes each: **32 scanlines of 240 pixels**.
It repeats the measurement 64 times, yielding outside the timed interval.
Microsecond timing includes call overhead and interrupt scheduling. These are
warm-kernel measurements, not cache-disabled worst cases.

| Kernel | Code bytes | Mean / 32 scanlines | Best |
|---|---:|---:|---:|
| C `-Os`, flash | 126 | 269 us | 269 us |
| C `-O2`, flash | 122 | 232 us | 232 us |
| C `-O3`, flash | 122 | 232 us | 232 us |
| C `-O3`, IRAM | 122 | 220 us | 220 us |
| Handwritten RV32IMC, IRAM | 80 | 304 us | 304 us |

All compared outputs matched. The initial ASM is about **38% slower** than C
at the same IRAM placement. It serializes dependent palette loads/expansion;
its smaller instruction count is not evidence of a shorter pipeline schedule.
The C compiler schedules independent work and emits compact instructions.
The game and saver therefore do not call the ASM kernel. It remains callable
for experiments; its 80 IRAM bytes are diagnostic overhead, not a speed gain.
The C IRAM improvement over flash is about 5% for this kernel, not the whole frame.
Neither kernel uses Zba/Zbb, floating point or per-pixel division.

A future ASM experiment should overlap both palette loads, preserve alignment,
unroll only after checking code growth, and compare against equally optimized
C. Apply a replacement only after pixel equivalence, ABI/alignment tests and
whole-frame timing. The present result is evidence against replacing this loop
with the first handwritten version.

## Scene compiler comparison

A development-only build linked three implementations of the same Copper Dream
scene. Each rendered the same 32 phases in five-line strips; their checksums
matched. Times include a one-tick yield after each frame and exclude LCD/SPI.

| Scene compiler setting | Function bytes | Stack bytes | Mean / frame |
|---|---:|---:|---:|
| `-Os` | 918 | 224 | 8,012 us |
| `-O2` | 1,036 | 240 | 6,136 us |
| `-O3` | 1,424 | 208 | 5,988 us |

Selected local `-O3`: approximately 25% less elapsed time than `-Os`, about 2.4%
less than `-O2` for 388 more code bytes. The static stack-use report includes the
92-byte packed-row/palette scratch. After profiling, the extra `-Os`/`-O2` scene
copies are compiled out (`ROUND_CREW_PROFILE=0`); the release verifier checks it.
The final scene also uses the faster C IRAM expander, so its function size/time
may differ slightly from this profiling build. No global optimization flag changed.

## Which other code is worth replacing?

| Code | Recommendation |
|---|---|
| `round_demo_render` signed constant divides | Already replaced with exact shift/rounding correction. Golden-frame hashes match; actual ELF contains no DIV in that kernel. Source-level tuning was sufficient. |
| `round_crew_render` | Keep measured local `-O3`. Its two hardware DIV operations are row reciprocals, not per-pixel operations. Per-pixel stepping uses integer additions, shifts, comparisons and lookup loads. |
| `orbital_game_step`, `orbital_point`, touch angle | Q8/Q15 and 1,024 angle units; interpolate the shared sine table. Touch atan uses a 65-byte octant table and one integer ratio calculation per input event. Collision tests use radial/angular bounds, avoiding square roots and trig. Profile before replacing short C helpers. |
| `lv_color_fill` | Previous disassembly: 76 IRAM bytes, already fills sixteen pixels with eight word stores. Little obvious ASM opportunity. |
| RGB565 byte swapping | Swap the 16-entry palette once per strip instead of swapping each pixel. Eliminating work is more useful than an ASM byte-swap loop. |
| `memcpy` and line blending | Existing library/LVGL kernels are already tuned. Compare actual call sizes, alignment and instruction placement before replacing. |
| Circular layout geometry | Already integer-only. Cache widths or use a small table if a profile makes this material; a handwritten integer square root is not the first priority. |
| Touch sampling, SPI flush and UI transitions | Transaction/wait/scheduling latency dominates arithmetic in many cases. Fix work structure and blocking before assembly. |

## Bus and responsiveness changes

Two internal five-line buffers consume **4,800 bytes**, versus 2,400 previously.
LVGL renders B while SPI DMA owns A. If B cannot be allocated, this board falls
back to one buffer. The transaction pool remains four descriptors.
A binary completion semaphore lets the UI task sleep during `wait_cb`, instead
of spinning while the DMA transfer completes. The ISR signals LVGL completion
and wakes the waiter. Queue-submit failure signals completion from task context.
The flushing flag is authoritative, so stale semaphore tokens are harmless.
Allocation failure falls back to a one-tick wait. No task is added.

The semaphore does not remove the CPU cost of sending address-window commands,
per-strip setup or callbacks. Panel SPI stays at 20 MHz and flash stays at DIO
80 MHz. Full-square RGB565 still has a 46.08 ms pixel-wire lower bound. A target
of 20 fps is not a claim of achieved screen rate.

Orbital Vector uses sparse vector bounds rather than full-screen refreshes.
It unions each object's previous/new rectangle into one invalidation. With
12 enemies, 8 shots, 4 bursts and the ship, this stays below LVGL's 32-region
limit in the normal update path. Pause/restart/scene entry can repaint more.
The native test asserts steady gameplay averages under half-square pixel traffic;
that test is about transfer demand, not physical LCD latency or guaranteed fps.

Raw profiling logs/disassembly are local in `build/copper-profile.console.log`
and `build/binary-audit/copper-experiment/`. Run `saver kernels`,
`saver bench crew`, and `saver status` to repeat the appropriate measurements.
Keep mirroring off for timing. Baseline and final firmware storage/heap results
are recorded separately in [BINARY_OPTIMISATION.md](BINARY_OPTIMISATION.md).
