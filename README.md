# cpp-dsl-sdk

The C++ runtime for lmscript `_dsl_` bytecode: loader, verifier, VM and host registry in one header, `include/dslvm.hpp`.
C++17, standard library only, no dependencies. This repository is the single source of truth for the header; hosts vendor it
by pinned hash (see "Consuming it"). Status: EXPERIMENTAL, part of the DSL proof-of-concept series (lmctl-mono
`lmscript/durable-memory/requirements/dsl-bytecode.md` R05 "provided once", R12-R22; living language spec
`lmscript/durable-memory/design/dsl-language.md`).

## What it is

lmscript compiles a `_dsl_` block (a small strongly typed configuration language whose names come from a JSON schema) into a
portable bytecode file (`savedsl`). A host program written in C++ embeds this header, implements the schema's objects and
functions in a `dslvm::Registry`, loads the file with the exact schema bytes, and runs the chunk once per event with a
`dslvm::Context` of live host objects. The host owns the process, the objects and their lifetime; the chunk only calls what
the registry offers. lmscript never executes the DSL; this SDK never reads the schema's content, only its bytes (for the
SHA-256 binding).

## Embedding it in twenty lines

```cpp
#include "dslvm.hpp"
struct Greeter { std::string last; };

int main(int argc, char** argv) {
  dslvm::Registry reg;
  reg.context("g", "Greeter");                                                       // the schema's context variable and its type
  reg.method("Greeter", "say", {"string"}, "void", [](void* self, dslvm::Args& a) { // Greeter.say(string) -> void, declared as in the schema
    static_cast<Greeter*>(self)->last = a[0].s; return dslvm::Value::none(); });
  reg.function("now", {}, "string", [](dslvm::Args&) { return dslvm::Value::string("2026-10-18"); });  // now() -> string
  if (argc > 3 && std::string(argv[3]) == "--dsl-signatures") { for (auto& s : reg.signatures()) std::cout << s << "\n"; return 0; }  // diffed against `lmscript --dsl-signatures`

  dslvm::Program program;
  try { program = dslvm::Program::load(dslvm::read_file(argv[1]), dslvm::read_file(argv[2]), reg); }  // bytecode, schema
  catch (const dslvm::Error& e) { std::cerr << "refused: " << e.what() << "\n"; return 3; }       // nothing ran

  Greeter greeter;
  dslvm::Context ctx; ctx.set("g", &greeter);
  try { program.run(ctx, reg); } catch (const dslvm::Error& e) { std::cerr << "dsl error: " << e.what() << "\n"; return 3; }
  std::cout << greeter.last << "\n";                                     // the host continues with what the chunk did
}
```

`Registry` is declared with the schema's types (parameter types and return type, as strings); `load()` resolves canonical import signatures (including parameter and return types), and refuses a file whose imports the registry does not provide, before anything runs. `reg.signatures()` lists the
sorted canonical strings (`context g: Greeter`, `fn now() -> string`, `method Greeter.say(string) -> void`), exactly what
`lmscript --dsl-signatures SCHEMA` prints, so a host's test diffs the two sources of truth (spec v4 S0, A02). That is a consistency
check, not a defense: per operator decisions R33/R34 the VM performs no call-time type checks; bytecode is trusted like the host's
own code. `Value` carries `INT` (int32), `FLOAT` (binary64), `STR`, `BOOL` and `OBJ` (a host pointer plus the schema type name); a
void method returns `Value::none()`. Host code throws `dslvm::Error` to report a run-time failure.

**Result line and exit codes (spec v4 D7, R26).** CLI hosts print exactly one JSON line on stdout through `dslvm::report_ok(json)`,
`dslvm::report_error("load"|"run", e)` (returns 3) or `dslvm::report_app_failure(message)` (returns 4) and exit with the returned
code; lmscript's own compile failures are exit 2. `Error::code` is `LOAD_BAD_BYTECODE`, `LOAD_SCHEMA_MISMATCH`, `LOAD_UNBOUND`,
`RUN_ERROR` (a VM error), `RUN_HOST` (thrown by a host closure) or `APP_FAIL`. Server hosts (poc-web) keep their own protocol.

## Versioning rule

`dslvm::VERSION` is the bytecode format version this SDK reads. The SDK version equals that number (currently 4) and the
loader refuses every other version: there is no backward compatibility. When the compiler's format changes, the compiler,
this header and every host move together; the history is in the living spec (v1 poc-web, v2 poc-numpy, v3 poc-curl, v4 spec
slice S1': int64 with wraparound, `MOD_I`, typed constants, an import table of canonical signatures, context/locals/lines
sections, the compiler-measured `max_stack`, no VM limits; S3 adds structured `range` loops with read-only hidden locals and
the optional `RunStats` dispatch counter for host measurements).

## What load() guarantees

Wrong magic, another version (v3 included), a schema-hash mismatch, truncation, trailing bytes, unknown constant tags,
undecodable instructions, out-of-range constant/local/import/context indexes, calls whose import kind or arity differ, ordinary
jumps that miss an instruction start, go backwards or cross a range-loop boundary, malformed or misnested range loops, writes to
their read-only hidden locals, a missing HALT, and
context declarations or import signatures the registry does not provide (the message names the host's own signature for that
name) are refused with a message that names the problem. `run()` checks value kinds defensively and imposes no limits
(operator decision 2026-10-18): no stack cap, no locals cap, no string cap; `max_stack` from the file is a reservation hint.
Integer arithmetic is 64-bit two's complement and wraps (R34); division and remainder by zero are the run-time errors.

## Tests

    make test           # tests/dslvm_test.cpp: 68 checks (range loops, int64 wraparound, sections, verifier rejection, run-time checks, no-limit pins, signatures, result line)
    make conformance    # conformance/run.cpp over conformance/vectors: 31 language-neutral vectors including S3 loops

`conformance/vectors` is generated by the lmscript compiler (lmctl-mono `lmscript/scripts/generate-dsl-conformance.ts OUT_DIR`)
from small DSL programs that cover every opcode, type and error path: `NAME.dsl` (source), `NAME.dslb` (bytecode),
`NAME.expect` (line 1 `trace`, `runtime_error` or `load_error`, then the expected trace lines or error substring) and
`manifest.json`, whose `host` entry describes the stub a runtime must implement (context `t` records calls; `root`, `thing`,
`twice`, `half`, `yes`, `fail`). A second runtime (Rust, R21) implements the same stub and reads the same files; nothing in the
vectors is C++-specific.

## Consuming it

Hosts keep a verbatim copy at `cpp/dslvm.hpp` plus `sdk.lock` (SDK commit and the header's sha256) and `scripts/sync-sdk.sh`,
which copies the header from a sibling checkout `../cpp-dsl-sdk` and rewrites the lock; each host's `test.sh` fails when the
vendored header's hash differs from the lock. Hosts: `lmctlhq/poc-web`, `lmctlhq/poc-numpy`, `lmctlhq/poc-curl`.
