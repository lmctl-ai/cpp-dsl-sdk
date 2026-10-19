# Architecture and decisions (with reasons)

## Public API (one header, C++17, standard library only)

`dslvm::Error`, `dslvm::Value` (VOID, INT int64, FLOAT binary64, STR, BOOL, OBJ = host pointer + schema type name), `dslvm::Args`,
`dslvm::Registry` (`function(name, argc, fn)`, `method(type, name, argc, fn)`, `context(name, type)`), `dslvm::Context`
(`set(name, pointer)`), `dslvm::Program::load(bytes, schemaBytes, registry)`, `Program::run(context, registry)`, `dslvm::VERSION`,
helpers `sha256`, `hex`, `read_file`. Everything else in the header is private detail.

## Bytecode v4 (big-endian, fixed widths, IEEE-754 binary64)

The file is `DSLB`, u16 version 4, u8[32] SHA-256 of the schema bytes, then a u32-count constant pool. Each constant is tag 1
(string), u32 byte length and UTF-8 bytes. The import table is u16 count followed by u32 constant indexes for canonical function or
method signatures. The context table is u16 count followed by name/type constant indexes. Locals are a u16 count followed by a
type constant index and hidden flag for each slot. A u16 compiler-measured `max_stack` precedes the u32 code length and code;
the file ends with a u32-count line table of `(pc, line, column)` entries, which may be empty. Integer operands are i64 in
big-endian two's-complement representation; all other fixed-width numbers are big-endian. Opcodes retain v1-v3 numbers, with
`PUSH_STR` and calls using their v4 u32/u16 indexes, and add `MOD_I` at `0x33`. Full table: the mono spec
`design/dsl-language.md` section 7.

## Decisions

- **Canonical imports resolved once at load** (2026-10-18, S1'): signatures remain readable strings in the constant pool, while
  call operands carry compact import indexes. This preserves schema-bound diagnostics and avoids a registry lookup on every call.
  `Program` retains pointers to the resolved registry entries, so the same registry object must outlive the program and remain
  unchanged between `load()` and `run()`.
- **Typed opcodes (ADD_I versus ADD_F)** (2026-10-18, poc-numpy): the VM and verifier stay type-free; the compiler decides. The VM
  checks value kinds defensively anyway.
- **Forward-only jumps** (2026-10-18, SDK creation): the v4 S1' language still has no loops, so refusing backward jumps proves
  termination without a step counter. The loop slice changes this verifier rule together with the conformance vectors.
- **2026-10-18, v4 (spec slice S1' under R31-R34):** `Value::i` is `int64_t`; ADD/SUB/MUL/NEG wrap in two's complement (unsigned
  arithmetic, no UB), `INT64_MIN / -1` and `% -1` are handled explicitly (wrap / 0); `MOD_I` 0x33; constants carry a tag (1 =
  string, u32 count and length); an import table of canonical signature strings is resolved at load with `function_by_signature`
  / `method_by_signature` (a drift message names the host's own signature via `declared_for`); context and locals sections are
  checked/kept (`Program::locals()`, `max_stack()`, `lines()`, `line_of(pc)`); `LOAD_CTX` indexes the context table, `CALL_FN` /
  `CALL_METHOD` the import table. The limits below are GONE (operator: no memory or time limits in the language or the VM).
- **No VM resource limits in v4** (2026-10-18, R34): operand stack, locals and string concatenation grow as the script requires.
  `max_stack` is a compiler-computed reservation hint; the host remains responsible for any process-level resource policy.
- **Loader refuses every other version** (R-level "no backward compatibility"): SDK version = bytecode version; compiler, SDK and
  hosts move together. v1 -> v2 -> v3 -> v4 all happened on 2026-10-18 during the POCs.
- **Float to text is `%.15g`** (v3): the same text poc-numpy prints, deterministic across hosts; part of the conformance vectors
  (`float-formatting`).
- **Host run-time errors are `dslvm::Error` thrown from registry callbacks**; `run()` propagates them unchanged (vector
  `runtime-host-error`). Hosts map them to exit code 3 by convention.
- **2026-10-18, S0' (spec v4): registration declares the schema's types** (`function(name, {params}, returns, fn)`,
  `method(type, name, {params}, returns, fn)`) so `signatures()` can list the canonical strings that `lmscript --dsl-signatures`
  prints; hosts diff the two in their test.sh. Duplicates are refused. Deliberately NOT added (operator R33/R34): call-time
  type/arity/return checks, forged-file defenses, caps; the loader's v3 structural verification is unchanged.
- **2026-10-18, S0': `Error::code`** classifies the existing errors (`LOAD_BAD_BYTECODE`, `LOAD_SCHEMA_MISMATCH`, `LOAD_UNBOUND`,
  `RUN_ERROR`, `RUN_HOST`, `APP_FAIL`) and `report_ok/report_error/report_app_failure` print the one-line JSON result with exit
  codes 3 (load or run) and 4 (application); compile failures are lmscript's exit 2 (R26).
