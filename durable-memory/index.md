# cpp-dsl-sdk notes

This repository contains a header-only C++ runtime, native tests, and language-neutral conformance fixtures for lmscript bytecode.
It is experimental. The current SDK and bytecode format version is 5; other versions are refused.

| File | Contents |
| --- | --- |
| [01-purpose-and-place.md](01-purpose-and-place.md) | Host integration and the role of the conformance fixtures |
| [02-architecture-and-decisions.md](02-architecture-and-decisions.md) | Public API, format overview, verifier and runtime decisions |
| [03-build-test-conformance.md](03-build-test-conformance.md) | Build commands, tests, fixtures, and optional string catalog |
| [04-gotchas-and-limits.md](04-gotchas-and-limits.md) | Versioning, trust model, vendoring, and host responsibilities |
| [05-open-items.md](05-open-items.md) | Possible future improvements |

The root README has the embedding example, command summary, catalog reference, and license.
