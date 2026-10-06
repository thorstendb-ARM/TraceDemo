# Code Snippets for Trace Demo

Enable trace communication in `csolution.yml`:

```yaml
            trace:
              - swo-uart:
                mode: file
                input-clock: 4000000
                output-clock: 1000000
```

Resolve traced `PC` to code location:

```gdb
>info line *0x080002a4
```
