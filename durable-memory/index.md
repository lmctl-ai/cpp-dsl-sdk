# cpp-dsl-sdk durable memory

What this repo is: the C++ runtime for lmscript `_dsl_` bytecode (one header, `include/dslvm.hpp`: loader, verifier, stack VM, host
registry), its unit tests and the language-neutral conformance vectors. Single source of truth for the header every host vendors.
Status (2026-10-19): EXPERIMENTAL, works; SDK version = bytecode version = 5; repo `lmctl-ai/cpp-dsl-sdk`. v4 and all other versions are refused.

Team guide (start here if the framework is new to you): lmctl-mono `lmscript/durable-memory/dsl-framework.md`. Operator requirements:
lmctl-mono `lmscript/durable-memory/requirements/dsl-bytecode.md` (R05 "provided once" is this repo; R12/R13 model; R21 Rust).

| File | Fact |
| --- | --- |
| [01-purpose-and-place.md](01-purpose-and-place.md) | why a shared SDK exists, what it is not, who consumes it |
| [02-architecture-and-decisions.md](02-architecture-and-decisions.md) | public API, bytecode v5 layout, structured loop verifier, no VM caps, version refusal |
| [03-build-test-conformance.md](03-build-test-conformance.md) | exact commands, toolchain facts, conformance generation, and the optional string catalog checks |
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
Verified: 82 native checks and 42 shared vectors.

## R38 while and control transfer (bytecode v5)

`WHILE_BEGIN` opens a verified loop, `WHILE_TEST` consumes a bool and branches to the paired exit, and `LOOP_BACK` is the only
backward edge; it must return to the paired head. The verifier uses lexical loop owners to prohibit jumps into/out of loop bodies,
except a forward break jump to the innermost loop's exit. Dataflow checks stack depth at loop heads and all control-flow joins.
Continue targets the `FOR_NEXT` or `LOOP_BACK` instruction. No execution budget is added; non-termination is the author's
responsibility. v4 bytecode is refused. Validation: 82 native checks plus 42 compiler-generated conformance vectors.

Latest v5 integration: 86 native checks and 43 regenerated vectors pass, including caller-stack operands across inlined mixed loops. The verifier checks paired while-test stack depth and prevents bypassing the test; these are static structural rules, with no new runtime checks.

## S5: located existing runtime errors

Program::run attaches the line-table position of the faulting opcode, preserving the existing error code. Error::locate prefixes once; an already located nested error is preserved. report_error adds numeric line/column fields, omitting an unknown column. Column 0 is unknown, including long physical source lines; empty tables retain unlocated messages and JSON. No new runtime validation or resource limit. Native tests assert opcode PC rather than reader-next PC, exact JSON, empty tables and single attachment. Verified: 91 native checks and 50 shared vectors, including nested loops, inline calls, division/remainder and empty/line-only mappings.
