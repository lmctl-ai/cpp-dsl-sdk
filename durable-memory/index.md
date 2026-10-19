# cpp-dsl-sdk durable memory

What this repo is: the C++ runtime for lmscript `_dsl_` bytecode (one header, `include/dslvm.hpp`: loader, verifier, stack VM, host
registry), its unit tests and the language-neutral conformance vectors. Single source of truth for the header every host vendors.
Status (2026-10-18): EXPERIMENTAL, works; SDK version = bytecode version = 3; private repo `lmctlhq/cpp-dsl-sdk`.

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
