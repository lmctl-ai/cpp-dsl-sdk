# Purpose and place in the DSL framework

Current prepared public names: [lmweb](https://github.com/lmctl-ai/lmweb), [lmnumpy](https://github.com/lmctl-ai/lmnumpy), and [lmcurl](https://github.com/lmctl-ai/lmcurl). Dated facts below retain the original repository names.

- **Fact (2026-10-18):** `cpp-dsl-sdk` is the runtime side of the lmscript `_dsl_` framework. lmscript compiles a `_dsl_` block
  into a bytecode file; a host program embeds `include/dslvm.hpp`, implements its schema's objects in a `dslvm::Registry`, loads
  the file with the exact schema bytes and runs the chunk as a callback (operator model R12, R13). The compiler lives in
  lmctl-mono `lmscript/dsl.ts`; the language spec is lmctl-mono `lmscript/durable-memory/design/dsl-language.md`.
- **Fact:** the repo was created on 2026-10-18 after three proof-of-concept hosts (poc-web, poc-numpy, poc-curl) each carried a
  verbatim copy of the header; requirement R05 says the runtime is "provided once". The copies were replaced by vendoring from this
  repo with a pinned hash (see 04).
- **Fact:** this repo never reads the schema's content. It hashes the schema bytes (SHA-256) to bind bytecode and schema; the host
  registry is the implementation and the loader checks the bytecode's names and arities against it.
- **Consumers (2026-10-18):** `lmctlhq/poc-web`, `poc-numpy`, `poc-curl`, `lmor-dsl` (all vendor `cpp/dslvm.hpp` + `sdk.lock`).
  A future Rust runtime (R21) consumes only `conformance/vectors`, not the header.
