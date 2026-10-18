# Architecture and decisions (with reasons)

## Public API (one header, C++17, standard library only)

`dslvm::Error`, `dslvm::Value` (VOID, INT int32, FLOAT binary64, STR, BOOL, OBJ = host pointer + schema type name), `dslvm::Args`,
`dslvm::Registry` (`function(name, argc, fn)`, `method(type, name, argc, fn)`, `context(name, type)`), `dslvm::Context`
(`set(name, pointer)`), `dslvm::Program::load(bytes, schemaBytes, registry)`, `Program::run(context, registry)`, `dslvm::VERSION`,
helpers `sha256`, `hex`, `read_file`. Everything else in the header is private detail.

## Bytecode v3 (big-endian, fixed widths, IEEE-754 binary64)

magic `DSLB`, u16 version, u8[32] SHA-256 of the schema bytes, u16 string constants (u16 length + UTF-8), u16 locals, u32 code length,
code. Opcodes: v1 `01-11` (push int/str/bool, locals, load context, call fn/method by name, EQ/NE/NOT, forward jumps, DUP/POP/HALT),
v2 `12-25` (PUSH_FLOAT, typed int/float arithmetic and ordering, NEG, I2F), v3 `26-29` (CONCAT, I2S/F2S/B2S). Full table: the mono
spec `design/dsl-language.md` section 7.

## Decisions

- **Names resolved by string at load, not by index** (2026-10-18, poc-web): makes the file self-describing and the verifier
  trivial; cost is a map lookup per call. An import table resolved once at load is the known next step if lmor-scale programs
  need it.
- **Typed opcodes (ADD_I versus ADD_F)** (2026-10-18, poc-numpy): the VM and verifier stay type-free; the compiler decides. The VM
  checks value kinds defensively anyway.
- **Forward-only jumps** (2026-10-18, SDK creation): the language has no loops, so refusing backward jumps proves termination
  without a step counter. If loops arrive (Q12), this rule and the conformance vector `load-backward-jump` change together.
- **Resource limits in the VM** (2026-10-18): operand stack 1024 values, 4096 locals, CONCAT result 16 MiB. Chosen as generous
  for configuration chunks and small enough to stop a hand-crafted file from exhausting memory.
- **Loader refuses every other version** (R-level "no backward compatibility"): SDK version = bytecode version; compiler, SDK and
  hosts move together. v1 -> v2 -> v3 all happened on 2026-10-18 during the POCs.
- **Float to text is `%.15g`** (v3): the same text poc-numpy prints, deterministic across hosts; part of the conformance vectors
  (`float-formatting`).
- **Host run-time errors are `dslvm::Error` thrown from registry callbacks**; `run()` propagates them unchanged (vector
  `runtime-host-error`). Hosts map them to exit code 3 by convention.
