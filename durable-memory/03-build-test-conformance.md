# Build, test, and conformance

The SDK requires a C++17 compiler and has no external build dependencies. Run:

```sh
make test
make conformance
```

The unit tests build an in-test assembler for malformed programs and cover valid execution, arithmetic, loops, imports, source
locations, host errors, and loader refusals. Conformance runs the language-neutral fixtures under `conformance/vectors`.

The vectors contain source, compiler-emitted bytecode, expected traces or errors, and a manifest describing the stub host. A
runtime implementation supplies that host and runs the same fixtures; the vector format does not depend on C++.

The version in `dslvm::VERSION` must match the compiler's emitted format. The loader rejects every other bytecode version.

`Program::RunStats` can optionally count instruction dispatches for measurements. It is observational and does not impose a
budget or limit.

## Standard library catalog

`stdlib/` contains optional schema declarations and C++ binders for ordinary host functions. `make test` includes the string
catalog unit tests and `tests/string-signature-parity.sh`, which compares the test host's canonical registry signatures with the
published compiler's output for the fixture schema. Install `@lmctl-ai/lmctl` so `lmscript` is on `PATH`, or set `LMSCRIPT_BIN`
to an explicit compiler executable.
