# cpp-dsl-sdk durable memory

What this repo is: the C++ runtime for lmscript `_dsl_` bytecode (one header, `include/dslvm.hpp`: loader, verifier, stack VM, host
registry), its unit tests and the language-neutral conformance vectors. Single source of truth for the header every host vendors.
Status (2026-10-18): EXPERIMENTAL, works; SDK version = bytecode version = 4; private repo `lmctlhq/cpp-dsl-sdk`.

Team guide (start here if the framework is new to you): lmctl-mono `lmscript/durable-memory/dsl-framework.md`. Operator requirements:
lmctl-mono `lmscript/durable-memory/requirements/dsl-bytecode.md` (R05 "provided once" is this repo; R12/R13 model; R21 Rust).

| File | Fact |
| --- | --- |
| [01-purpose-and-place.md](01-purpose-and-place.md) | why a shared SDK exists, what it is not, who consumes it |
| [02-architecture-and-decisions.md](02-architecture-and-decisions.md) | public API, bytecode v4 layout (v3 as history), verifier guarantees, the removed limits, the dated decisions and their reasons |
| [03-build-test-conformance.md](03-build-test-conformance.md) | exact commands, toolchain facts, how the vectors are generated and what a second runtime must implement |
| [04-gotchas-and-limits.md](04-gotchas-and-limits.md) | vendoring by hash, version rule, shared-box load, what the verifier cannot know |
| [05-open-items.md](05-open-items.md) | what the next agent should do, in order |

README.md has the 20-line embedding example and the versioning rule; do not duplicate it here.

## S3 structured ranges (2026-10-19)

FOR_RANGE/FOR_NEXT snapshot int64 bounds, support nesting and empty ranges, and enforce loop shape at load.
No runtime budget, depth cap or new dynamic checks. Optional RunStats counts dispatches for measurements.
S3 verification: 68 unit checks and 31 shared vectors, including million iterations and i64 endpoints.

## S4 primitive math (2026-10-18)

The trusted VM decodes MATH (0x32) with shared function IDs 0-17. Float transcendental functions use the C++ standard library
(`std::sin`, `std::cos`, `std::pow`, and related libm operations); shared vectors mark those results with `~ ` and compare finite
numeric tokens at relative tolerance 1e-12. Integer abs wraps `INT64_MIN`; `int(float)` truncates toward zero, maps NaN to zero,
and saturates infinities/out-of-range values. Float min/max propagate NaN and choose the first equal operand, preserving signed zero.
Clamp is the direct `min(max(x, lo), hi)` formula, including reversed bounds.

## S2 canonical overload binding

Multiple signatures may share one function/method name. Registry keys are full canonical signatures;
duplicate parameter tuples are refused regardless of return type. Enums retain their schema type
names in signatures and use strings as runtime values. Self expands to the receiver type before
registration/import binding; no identity check or dynamic overload resolution is added.
Verified: 75 native checks and 37 shared vectors.
