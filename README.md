# RTX5: Array overrun with the writing PC in the trace

One project, one `NUCLEO-L552ZE-Q` target, and one `Debug` build using GCC and `-O0`.
Board/device: [NUCLEO-L552ZE-Q / STM32L552ZETxQ](https://www.keil.com/boards2/stmicroelectronics/nucleo_l552ze_q/).

[Memory layout diagram](doc/memory-overrun.svg): an editable SVG overview for presentations.

## Behavior

`main.c` starts exactly two application threads, plus the RTX idle thread.
The unused RTX timer thread is disabled.

- `worker2` writes the sequence `10, 20, …, 200` twice, with one write every 100 ms.
  Its counter is local, so a write to `varWorker2` by another thread does not
  affect its next correct assignment.
- `worker1` executes the deliberately incorrect loop with `i <= 8` and
  `arrWorker1[i] = cnt++` twice, resetting `cnt` to 0 for each pass.
  The thread waits 100 ms after each assignment.
  The ninth assignment in each pass writes **8** to `varWorker2`.

Both threads start without an artificial time offset. There is no additional
delay between outer loop iterations. `worker1` takes about 900 ms for its nine
steps; `worker2` takes about 2000 ms for its 20 steps. Execution remains
deterministic; the different loop lengths place the overrun at different values
in the `worker2` sequence. Because both threads use the same 100 ms interval,
`worker2` can immediately overwrite the incorrect value 8. The event trace
captures even this brief error, which occasional variable polling could miss.

After two outer loop iterations, both threads suspend themselves indefinitely
with `osThreadSuspend(osThreadGetId())`. They remain visible in the RTOS debugger
and perform no further writes. Restart the application to repeat the demo.

Both objects are separate global `volatile int32_t` symbols.
The [linker](Board/gcc_linker_script.ld.src) places the array immediately before
the variable and verifies the 32-byte offset with `ASSERT`.
C declaration order alone would not guarantee this layout.
The out-of-bounds access deliberately invokes undefined C behavior;
the demo is intended for this verified GCC debug build.

## Build and trace

1. Open `TraceDemo.csolution.yml` in the CMSIS Solution view and run **Build**.
   Tool versions are specified in `vcpkg-configuration.json`; the three pack
   versions are specified in the csolution.
2. Connect ULINKplus with SWD and SWO. Hardware TrustZone must already be
   disabled; `trustzone: off` does not change option bytes.
   Select the connected probe in CMSIS Solution and verify the project-local
   `cmsis-csolution.probe-id`, especially when changing probes.
3. Enable `vscode-cmsis-debugger.enableTraceGenerationView` in workspace settings.
   Open the [trace profile](.cmsis/TraceDemo+NUCLEO-L552ZE-Q.ctrace.yml) in
   **Trace Generation** and run **CMSIS Debugger: Convert *.ctrace.yml to
   *.ctrace-run.yml** after every build or profile change. The watched address
   is resolved from the current ELF.
4. Select **Load & Debug**, run from `main` without breakpoints for at least
   five seconds, then pause. The debugger extension generates the CSV file at
   `.trace/TraceDemo+NUCLEO-L552ZE-Q.SWO.csv`.
5. Compare `value` and `pc` in the `dwt` rows: the increments of ten come from
   `worker2`; the inserted **8** comes from `worker1`.

SWO is enabled directly in the [csolution](TraceDemo.csolution.yml):
4 MHz core/trace clock, 1 Mbaud SWO, recording to a file. The profile watches
only writes to the four bytes of `varWorker2`, using `output: PC+value`.
Timestamps are enabled; periodic PC sampling and ITM software channels are
disabled. Each reported PC therefore belongs to the specific write access.
The debugger configures the pins and trace using the DFP sequences; the
application contains no trace output code. GDB port 45333 avoids conflicts
with debug ports used by other projects running in parallel.

## Verified on hardware — 2026-10-06

Build and Load & Debug succeeded. After both iterations, both workers appear
as `Blocked` in `__svcThreadSuspend` in the RTX debugger; only the idle thread
is runnable. No fault flags are set.

| Object | Address | Size |
| --- | --- | --- |
| `arrWorker1` | `0x200000B0` | 32 bytes |
| `varWorker2` | `0x200000D0` | 4 bytes |

The uninterrupted recording contains 42 DWT events:

| Source | Value | Writing PC | Count |
| --- | --- | --- | --- |
| `worker2 + 32` | `10 … 200`, twice | `0x08000300` | 40 |
| `worker1 + 32` | **8** | **`0x080002A4`** | 2 |

The overruns occur after approximately 0.8 and 1.7 seconds in trace rows 8 and
18 (zero-based, excluding the CSV header). Each time, 537 trace clock cycles
later, or about 134 µs at 4 MHz, `worker2` writes the correct value 90 or 180,
respectively, to the same address.
BSS initialization is not included in this recording.

All expected values were checked in order; every write by the other thread
contained 8. The CSV contains no trace error or overflow events and no decoder
notes. Addresses and PCs were checked against the ELF symbols; the 32-byte
spacing between the globals was also confirmed through live inspection.
The debug session was then stopped and the probe released.

These addresses apply to this build and may change when relinking.
ELF SHA256: `23299790f49a903eb5ff4240ab51603ae77d7dde98c919be37cbde4595a8f6ee`.
Trace was verified with CMSIS Debugger `1.8.1-64-g28c1601`, pyTS `0.6.1`, and
ctrace `0.4.0`; these trace features require a debugger version that provides
them. The extension also logged DAP evaluation errors during debugging; the
inspection calls used here and the trace verification succeeded.

For a before/after comparison, change `i <= 8` to `i < 8`, rebuild, regenerate
the trace profile, and record again: writes from `worker1` to `varWorker2`
should disappear. The supplied version retains the bug for the demonstration.

## Reused files

Only the required STM32L5 headers, startup/system files and license, memory
regions, GCC linker template, RTX configuration files, and tool manifest were
copied from `../cortex-m-trace-examples`.
The L5 source manifest is in `ThirdParty/ST/sources.json`.
Other boards, workloads, HAL, CubeMX, and board-layer abstractions are not
included. RTX and CMSIS-Core are provided by the packs.

The portable `.clangd` removes the GCC-specific `-masm-syntax-unified` option
for the editor only. Set `cmsis-csolution.generateClangSetup` to `false` in
workspace settings to preserve this configuration.

## License

This project is licensed under [Apache-2.0](LICENSE). Retained vendor copyright
notices and the ST license are included with the third-party sources.
