# Data Trace Demo

A program runs two worker threads concurrently. `varWorker2` is occasionally
overwritten with an unexpected value. Data trace records the writes and their
program counters (PCs) to help identify where that value comes from.

[Motivation diagram](doc/motivation.svg)

## Worker threads

- `worker1` writes data to a buffer.
- `worker2` writes `10, 20, 30, …, 200` to `varWorker2`.

Both threads wait 100 ms after each write. They each perform two passes, then
suspend themselves. Restart the application to repeat the demo.

## Prerequisites

Use the `NUCLEO-L552ZE-Q` with ULINKplus connected via SWD and SWO, and open
`TraceDemo.csolution.yml` in VS Code with the CMSIS Solution and CMSIS Debugger
extensions. Select the connected probe. Hardware TrustZone must be disabled.

Use the following tested version baseline. VS Code has a confirmed minimum
version requirement; earlier versions of the extensions and trace tools have
not been validated for this demo. Build-tool versions are pinned by the project.

| Component | Version | Requirement |
| --- | --- | --- |
| Visual Studio Code | 1.109.0 or later within 1.x | Minimum required by the tested debugger extension |
| Arm CMSIS Solution extension | 1.72.0 | Tested baseline |
| Arm CMSIS Debugger extension | 1.8.1-64-g28c1601 | Tested development build with Trace Generation and Show Captured Trace |
| ctrace | 0.4.0 | Bundled with the tested debugger |
| pyTS | 0.6.1 | Bundled with the tested debugger |
| Arm Tools Environment Manager | 1.26.0 | Tested baseline for activating the build environment |
| CMSIS-Toolbox (`csolution`, `cbuild`) | 2.15.0 | Pinned build-tool version |
| Arm GNU Toolchain | 14.3.1 | Pinned compiler version; the demo uses `-O0` |
| CMake | 3.31.5 | Pinned build-tool version |
| Ninja | 1.13.2 | Pinned build-tool version |

As of 2026-10-06, the public [CMSIS Debugger 1.8.0 release](https://github.com/Open-CMSIS-Pack/vscode-cmsis-debugger/releases/tag/v1.8.0)
does not include Trace Generation or Show Captured Trace. This workflow requires
the development build listed above or a newer build providing these features.
Use its bundled ctrace and pyTS; no separate installation is needed. pyOCD and
GDB are also supplied by the debugger extension.

Activate the tools defined in [vcpkg-configuration.json](vcpkg-configuration.json).
The [csolution](TraceDemo.csolution.yml) also pins the required packs:
`ARM::CMSIS@6.3.0`, `ARM::CMSIS-RTX@5.9.0`, and
`Keil::STM32L5xx_DFP@2.0.0`.

## Demo steps

1. **Enable trace in the csolution.**

   Add `trace:` to the existing `debugger:` node under
   `solution.target-types[].target-set[]`, then save `TraceDemo.csolution.yml`.
   The resulting node should look like this:

   ```yaml
   debugger:
     name: ULINKplus@pyOCD
     protocol: swd
     clock: 10000000
     gdbserver:
       - port: 45333
     trace:
       - swo-uart:
         mode: file
         input-clock: 4000000
         output-clock: 1000000
   ```

   This records SWO to a file, using a 4 MHz core/trace clock and 1 Mbaud SWO.

2. **Open Trace Generation.**

   In **Trace and Live View**, open **Trace Generation**.

3. **Add the variable.**

   Under **DWT Data Trace**, click **[+]** and set **Location** to `varWorker2`.
   Set **Access** to **Write** and **Output** to **PC+value**, then click **Save**.
   The trace configuration is stored locally and is ignored by Git.

4. **Build and enter debug mode.**

   Click **Build**, then **Debug Enter** to load the application and start the
   debug session.

5. **Record the writes.**

   Click **Run/Continue**, let the application run for about five seconds,
   then click **Pause**.

6. **Open the CSV recording.**

   Click **Show Captured Trace** on the debug toolbar to open the recorded CSV.

7. **Inspect the writes.**

   In the `dwt` rows, compare the `value` and `pc` columns. Find the unexpected
   value and compare its PC with those of the regular writes. Click the `pc`
   column header to sort and group writes from the same instruction. Use
   timestamp order to inspect when the unexpected value appears and is
   overwritten again.

After inspecting the trace, see the [explanation diagram](doc/memory-overrun.svg).

## How the test program works

The linker places the eight-element `volatile int32_t` array `arrWorker1`
directly before `varWorker2`. `worker1` uses `i <= 8` instead of `i < 8`, so
its ninth write stores `8` into the adjacent variable in this GCC `-O0` build.
This is an intentional out-of-bounds access and undefined C behavior.

`worker2` writes `10, 20, …, 200` using an independent local counter. Both
threads wait 100 ms after each write and perform two passes before suspending.
The next write by `worker2` can quickly hide the incorrect `8`. Data trace
records each write to `varWorker2` together with its PC, revealing which
thread wrote each value.

## License and sources

This project is licensed under [Apache-2.0](LICENSE). Retained vendor copyright
notices and the ST license are included with the third-party sources.
The [ST source manifest](ThirdParty/ST/sources.json) records the reused files.
CMSIS-Core and RTX are provided by the packs listed in the csolution.
