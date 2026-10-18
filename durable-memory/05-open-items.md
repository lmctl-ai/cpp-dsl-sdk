# Open items (what the next agent should do)

1. If the language gains a list type or loops (v4 design round in lmctl-mono `lmscript/durable-memory/proposals/dsl-v4-design-*.md`),
   bump `VERSION`, add the opcodes to the VM and verifier (loops mean revisiting forward-only jumps and adding a step limit),
   regenerate the vectors from the mono generator, update every host's `sdk.lock`.
2. A Rust runtime (R21): implement the manifest's host stub and run `conformance/vectors`; nothing in the vectors is C++-specific.
3. A packaged distribution (header-only tarball or CMake package) to replace copy-by-hash once more than one team consumes it.
4. Typed verification (argument kinds at load) if untrusted bytecode ever becomes a use case; needs the schema's content in the host.
5. An import table resolved once at load if method-call overhead shows up in lmor-scale programs (it did not at 2M variables).
