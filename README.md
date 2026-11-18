# kvm

A minimal macOS command-line tool (for **Apple Silicon Macs**) that switches a
monitor's built-in KVM to the next connected PC via the DDC/CI protocol.

This is a deliberately tiny tool: it does **one thing** — switch the KVM to the
next PC — and has **zero third-party dependencies**. All DDC/CI communication is
done directly through Apple's IOAVService interface.

## Features

- Switch a monitor's built-in KVM (USB upstream) to the next connected PC

That's it. No input-source switching, no volume, no list — just the KVM toggle.

## Requirements

- An **Apple Silicon** Mac (uses the IOAVService DDC/CI path; not for Intel)
- macOS 11 (Big Sur) or later
- Swift toolchain (Command Line Tools is enough; Xcode not required)
- A monitor that supports DDC/CI and has a built-in KVM (e.g. Dell U/P series)

> **Intel note:** this build targets Apple Silicon. On Intel the DDC/CI
> mechanism is different (IOKit I2C), so this tool will not work there.

## Installation

### Building from Source

```bash
git clone <this-repo-url>
cd kvm
swift build -c release
# binary at .build/release/kvm
cp .build/release/kvm /usr/local/bin/
```

## Usage

```bash
kvm next       # switch the KVM to the next connected PC
kvm            # same as 'kvm next'
kvm --version
kvm --help
```

The command sends DDC/CI **VCP `0xE7` = `0xFF00`** to every external display.
Exit status is `0` on success, non-zero if no external display was found or the
command was not accepted by any display.

## How it works

1. Walk the IO registry for per-display `DCPAVServiceProxy` nodes.
2. Keep only the ones whose `Location` property is `External` (skipping the
   built-in display).
3. For each, create an `IOAVService` and send a standard DDC/CI "set VCP
   feature" packet (the same technique used by `m1ddc` / AppleSiliconDDC).

## Security / supply chain

- **No external dependencies.** Only Apple system frameworks (IOKit,
  CoreFoundation, CoreDisplay) are linked — nothing is fetched from the network
  at build time.
- Apple Silicon has no *public* DDC/CI API, so the private `IOAVService` symbols
  are declared directly in the C source. This is the same mechanism every
  Apple-Silicon DDC tool relies on; no third-party code is compiled in.
- The tool performs no file, network, or process-spawning operations. Its only
  side effect is a DDC/CI write to the monitor.
