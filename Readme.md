# Floating-Point Unit & FMA

## Project overview
This project implements a configurable SystemC floating-point unit (FPU) that supports add, subtract, multiply, minimum, maximum, and fused multiply-add (FMA) operations with IEEE 754-style behavior. Each operation is selected via a 4-bit opcode and produces status flags for zero, sign, overflow, underflow, inexact, and NaN conditions as part of every cycle-driven computation. The module is designed as a set of cooperating submodules (e.g., FMA) to match the assignment’s specification and can be embedded into a TinyRISC-like CPU as an extension for floating-point arithmetic.

## Assignment highlights
- **Required operations:** FADD (8), FSUB (9), FMUL (10), FMIN (13), FMAX (14), and FMA (15), with operands `r1`, `r2`, `r3` and result `ro` sourced from module inputs each clock cycle. The FMA path performs multiplication and addition without intermediate rounding to maintain accuracy.
- **Status flags:** Operations must set zero, sign, overflow, underflow, inexact, and NaN flags based on the computed result range and validity.
- **Rounding modes:** Five rounding strategies are supported—nearest even (0), nearest away from zero (1), toward zero (2), toward +INF (3), and toward -INF (4). Results exceeding representable range round to INF; subnormal magnitudes round to zero.
- **Floating-point format:** The design follows IEEE 754 single-precision structure with configurable exponent and mantissa sizes, handling normalization, special values (±INF, NaN), and edge cases like INF±INF and NaN propagation.Optional helper methods expose constants and extrema for the configured format.
**under slides.pdf you can see exactly how each operation is implemented**

## Team contributions
- **Nourallah Gargouri:** Built the `AddSub` unit (including shift-align and normalize logic), the fused multiply-add data path with guard/sticky propagation and rounding, and CLI-based user input validation.
- **Saifeddine Guenanna:** Implemented CSV loading and conversion into requests honoring custom exponent/mantissa sizes and round modes, plus the SystemC `Max` and `Min` comparator modules handling NaN/INF edge cases with bitwise comparisons.
- **Ahmed Amine Loukil:** Developed command-line parsing with `getopt_long`, VCD trace generation, the top-level FPU clock-gating strategy to activate only needed submodules, and the floating-point multiplier pipeline covering exponent adjustment, sign handling, and rounding.

## Repository layout & build
The repository contains the expected artifacts for the assignment: a `Makefile` that builds an executable named `project`, a `build.sh` wrapper script, presentation slides (`slides.pdf`), source code under `src/`, headers under `include/`, and optional tests under `test/`.【F:assignment.txt†L61-L126】 Run either helper to build the SystemC design:

```bash
make project
# or
./build.sh
```

## Running simulations
Use the CLI to configure the FPU and feed a CSV of requests:

- `--size-exponent <uint8>` / `--size-mantissa <uint8>`: bit widths for exponent and mantissa (sum must fit in 32 bits).
- `--round-mode <uint8>`: rounding policy (0–4 as listed above).
- `--cycles <uint32>`: number of simulation cycles to process.
- `--tf <path>`: emit a trace file capturing key signals (omit to skip tracing).
- `<file>`: CSV input describing the queued requests (see below).
- `--help`: print option documentation and exit.

Each option is validated for range and file accessibility; invalid arguments halt execution with a descriptive error. Tracing, when enabled, records the important signals for inspection in tools like GTKWave.【F:assignment.txt†L769-L781】 The primary simulation entry point initializes the FPU with the selected parameters and returns per-flag statistics (cycles, sign events, overflows, underflows, inexact results, NaNs).【F:assignment.txt†L796-L865】

## CSV input format
Provide a headered CSV file where each row specifies an operation and operands to execute sequentially:

```
op,r1,r2,r3
9,0x00000011,0x10001011,
13,0x00000001,32949138,11112
15,0.001,2.395,0x13
```

- The first column selects the opcode (decimal). Remaining columns supply operands as floats, hexadecimal, or 32-bit integers (interpreted as float bit patterns). Unused operands may be left blank.
- Files **must** include the header row and use commas as separators; nonconforming files are rejected with an error.

