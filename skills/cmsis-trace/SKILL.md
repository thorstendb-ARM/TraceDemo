---
name: cmsis-trace-memory-corruption
description: Automatically investigate CMSIS Cortex-M runtime failures that are observable through SWO trace, configure the appropriate trace capture with pyOCD, decode the resulting data, and resolve relevant PCs to source. Apply to trace-observable memory, timing, control-flow, exception, or instrumentation problems; not to static-only reviews or PDSC trace-sequence authoring.
---

# CMSIS trace analysis

Investigate a runtime symptom with a live CMSIS SWO trace and produce an
evidence-backed diagnosis. Preserve the user's firmware unless they explicitly
request a fix. Use trace only for behaviour it can observe; state clearly when
the symptom needs a different debugger or instrumentation method.

## Choose the trace evidence

Classify the symptom before configuring the profile:

- **Wrong value, corruption, or peripheral register change:** DWT data trace;
  watch reads, writes, or both at the symbol/register address and record
  `PC+value` when the writer matters.
- **Unexpected execution, hang, latency, or ordering:** PC sampling and/or
  timestamped ITM/RTOS events; add a focused data watch only where it tests a
  concrete hypothesis.
- **Fault or crash:** capture the lead-up with PC sampling/data trace, then use
  the live debugger's fault and stacked-frame inspection to confirm the faulting
  PC. Do not infer a fault cause from trace rows alone.
- **Application-defined ITM events:** enable only the required ITM stimulus
  channels and decode their payload format from the application or pack.

Choose the smallest trace configuration that can distinguish the hypotheses.
Do not enable every source by default: bandwidth, comparator count, and trace
buffer capacity are finite.

## Configure the trace

1. Inspect the active `*.csolution.yml`, target, debugger, build context, and
   any existing `.cmsis/*.ctrace.yml` profile. Start from the active
   `*.cbuild-idx.yml`/`*.cbuild-run.yml` when available so the device, core,
   ELF, and trace clock are not guessed.
2. Ensure the selected `target-set.debugger` contains this SWO file trace block.
   Keep the CMSIS schema's indentation exactly as shown:

   ```yaml
            trace:
              - swo-uart:
                mode: file
                input-clock: <input-clock-value>
                output-clock: <output-clock-value>
   ```

3. Generate or update the local `.cmsis/<solution>+<target>.ctrace.yml` profile.
   Prefer symbols and pack metadata over guessed addresses so the configuration
   follows the current ELF. For a DWT data-watch investigation, use:

   ```yaml
   ctrace:
     setup:
       - core: Cortex-M33       # use the active target core
         timestamps:
           clock: <input-clock-value>
           itm-prescaler: 1
         data:
           - location: <suspect-symbol-or-register>
             access: W
             size: <access-size>
             output: PC+value
         itm:
           enable: 0x00000000
         pcsampling:
           period: 0
         synchronization:
           DWT: 16M
           sync-on-run: true
   ```

   Adapt the source type, symbol/register, access mode, size, core, timestamp
   clock, ITM channels, and sampling period to the chosen hypothesis. Convert
   the profile to `.trace/*.ctrace-run.yml` through the CMSIS Trace Generation
   extension after every build or profile change.

## Capture

- Build the active context first and confirm the ELF exists. A generated
  `ctrace-run` file with `ELF file does not exist` is not ready to capture.
- Prefer the VS Code CMSIS `Load & Debug` workflow because it configures the
  target, trace pins, SWO, and pyOCD together. A standalone `pyocd list` can be
  unavailable to a sandbox even when the VS Code extension sees the probe.
- Start without a breakpoint, continue past the initial `main` breakpoint, and
  run long enough to cover the suspected workload. Use about 30 seconds as a
  default for periodic failures, but shorten or extend it when event rate,
  buffer capacity, or the failure period requires it.
- Pause or stop the session to flush and decode the raw trace. Always release
  the probe when finished. Do not add temporary `printf`, UART, LED, or trace
  instrumentation to the application for this diagnosis.

## Analyze and correlate the trace data

Read `.trace/*.SWO.csv` and inspect rows by `type` (`dwt`, `pcsample`, `itm`,
fault/decode notes, or tool-specific records). When present, run the bundled
helper for a repeatable first pass:

```sh
python3 skills/cmsis-trace-memory-corruption/scripts/analyze_trace_csv.py \
  --csv .trace/<capture>.SWO.csv \
  --elf out/<context>/<build>/<solution>.elf \
  --trace-clock <Hz>
```

The helper reports event counts, PC/value groupings, timing, and optional
`addr2line` source resolution. Treat its output as triage; inspect the raw CSV
and correlate with source, symbols, RTOS state, and debugger observations.

- Separate startup/BSS initialization and decoder synchronization rows from
  application events. An initial zero from `Reset_Handler` is normally not the
  corruption.
- For data trace, count accesses by PC, access kind, value, and timestamp.
  Unexpected values or writers identify candidates only after the watched
  address and access size are confirmed.
- For PC sampling, look for the time window and call-site concentration around
  the symptom; sampling shows where execution was observed, not necessarily
  the exact instruction that caused the failure.
- For ITM/RTOS records, decode channel/payload semantics before assigning
  meaning to values. Compare event order and timestamps with the failing
  behaviour.
- Resolve every relevant PC against the exact ELF with `addr2line -f -C -i`
  or GDB `info line *0x...`; never assume addresses from an older build.
- Convert cycles using the configured trace clock:
  `seconds = cycles / trace_clock_hz`. Compare nearby events to show ordering,
  latency, or transient overwrites.
- Cross-check the symbol address and neighboring objects with `nm -n` or the
  linker map when an out-of-bounds write is suspected.

Report the capture/profile paths, target and ELF identity, event counts, key
event sequence, relevant PC(s), resolved file/line when available, timing, and
confidence. Distinguish direct evidence from inference. If no CSV, only
initialization, decoder errors, overflow, dropped data, or an unsupported
symptom appears, report the concrete limitation and the next useful capture
change instead of inventing a diagnosis.
