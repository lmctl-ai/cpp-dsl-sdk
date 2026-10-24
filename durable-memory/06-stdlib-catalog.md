# Stdlib catalog

The catalog is a set of ordinary host functions declared in a schema fragment and registered with the SDK's `register_*` helper. It adds
no opcodes and no VM behavior; a host may ship all of it, part of it, or its own functions. The reference implementation is
`stdlib/string.hpp` with `stdlib/string.schema-fragment.json`.

## Byte-string rule

VM strings are byte sequences in every SDK: lengths and offsets are bytes, no decoding or validation happens, NUL bytes are legal,
and invalid UTF-8 survives constants, concatenation, equality and host callbacks. Catalog functions therefore operate on bytes.
`str_lower` / `str_upper` change ASCII letters only; `str_substr` can cut a multibyte character. Host-facing helpers that need text
convert explicitly (validated or lossy). Rust follows the same rule with a byte-backed string value.

## String functions (`str_*`)

| Function | Semantics |
| --- | --- |
| `str_len(s) -> int` | byte length |
| `str_starts_with(s, p)`, `str_ends_with(s, p)`, `str_contains(s, n) -> bool` | byte comparison; empty argument is always true |
| `str_find(s, n) -> int` | first byte offset, or -1 |
| `str_substr(s, start, count) -> string` | byte offsets; start < 0 clamps to 0; start >= len or count <= 0 gives empty; count clamps to the remaining bytes |
| `str_lower`, `str_upper` | ASCII `A-Z` / `a-z` only |
| `str_trim` | strips bytes ` ` and `\t`..`\r` from both ends |
| `str_repeat(s, n)` | error if n < 0 or the result exceeds 64 MiB (67108864 bytes); empty `s` with n >= 0 gives empty |
| `str_replace(s, from, to)` | all non-overlapping occurrences left to right; empty `from` is a no-op; error if the result exceeds 64 MiB |
| `str_is_int(s)`, `str_to_int(s)` | optional single leading `+` or `-`, decimal digits only, must fit int64; `str_to_int` returns 0 when not an int |
| `str_is_float(s)`, `str_to_float(s)` | decimal syntax `[+-]digits[.digits][e[+-]digits]` (digits before or after the dot), no whitespace/hex/nan/inf; C locale; overflow or underflow-to-zero is not a float; `str_to_float` returns 0.0 when invalid |
| `str_match(text, pattern) -> bool` | glob over bytes: `*`, `?`, `[abc]`, `[a-z]`, `[!x]`, backslash escape; unterminated `[` is literal, `]` first in a class is literal, descending ranges are empty; error after 2^27 work steps |

`stdlib/string.hpp` is the exact specification for edge cases; ports must reproduce it byte for byte.

## Adding a function in all four SDKs

1. Add the signature to the C++ fragment (`stdlib/string.schema-fragment.json`) and `tests/string.schema.json` (they must be identical).
2. Implement it in `stdlib/string.hpp` and register it in `register_string`.
3. Port it: `lmcode-rust/dslvm/src/stdlib.rs` + `lmcode-rust/stdlib/` fragment, `lmcode-go/stdlib`, `lmcode-csharp/stdlib` (fragment + binder).
4. Run each SDK's signature-parity check against `lmscript --dsl-signatures` (sorted registry signatures must equal the compiler's output).
5. Add a stdlib conformance vector (below) and make every SDK pass it.
6. Update each README's catalog line and this table.

## Stdlib conformance vectors

`conformance/stdlib-vectors/` holds language-neutral cases: `NAME.dsl` (source), `NAME.dslb` (bytecode compiled by `lmscript` against the
catalog schema), `NAME.expect` (expected output or error) and a manifest entry. The runner registers the full string catalog and compares
results, so every SDK that ships a catalog function must pass the vectors that use it. Failure errors use the same codes as the main vectors.
