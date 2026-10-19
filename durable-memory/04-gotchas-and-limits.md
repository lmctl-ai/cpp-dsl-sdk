# Gotchas and limits

The loader accepts bytecode version 5 only. A v4 file is rejected before schema or code loading; regenerate all fixtures with the
current `lmscript` compiler. `LOOP_BACK` is structurally paired with `WHILE_BEGIN`; do not hand-author arbitrary back edges.
Ordinary jumps remain forward-only, with a verifier exception only for a break jump to the innermost loop exit. There is no
loop budget or runtime cap: a nonterminating script can run indefinitely by design (R34/R38).

- **Vendoring by hash, not by package.** Hosts copy `include/dslvm.hpp` to `cpp/dslvm.hpp`; `sdk.lock` records the SDK commit and
  the header's sha256; `scripts/sync-sdk.sh` (in each host) refreshes from a sibling checkout `../cpp-dsl-sdk` (override with
  `CPP_DSL_SDK=path`) and `scripts/sync-sdk.sh --verify` is the first check of every host `test.sh`. Editing a vendored header in
  place fails that host's test. Chosen over a git submodule because the team works with sibling checkouts and wants each host
  buildable offline; revisit when a packaged SDK exists.
- **Changing the header means touching every host:** rerun each host's `scripts/sync-sdk.sh`, rebuild, rerun its `test.sh`, commit
  the new `sdk.lock`. Hosts on 2026-10-18 all pin commit 2b28058.
- **The verifier is type-free.** It checks arity, names, bounds and jumps, not argument types; it trusts the compiler, which the
  schema hash makes reasonable. Untrusted bytecode with a valid hash could still pass wrong kinds to a host method; the VM's
  kind checks catch mismatches at run time, not at load.
- **Hosts must declare context names and types in the registry AND provide the objects per run** (`Context::set`); a declared but
  missing object is a run-time error (`context variable 'x' was not provided by the host for this run`).
- **Object handles are raw `void*` plus a type name.** Hosts keep objects alive for the run (arena pattern in lmnumpy and
  lmor-dsl); the DSL cannot free or return them.
- **Range loops are structurally bounded but have no runtime cap.** `FOR_RANGE` snapshots its two integer bounds, `FOR_NEXT`
  is the sole backward edge, and loop counters/end slots are compiler-owned hidden locals. Nesting has no VM-imposed depth
  limit; the host remains responsible for process resources. For measurements, `Program::RunStats` counts dispatches only.
- **Shared box:** load average 5-25 on this machine; the SDK tests are light, but run one heavy job at a time and `nice` builds.
