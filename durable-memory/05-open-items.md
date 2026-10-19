# Open items (what the next agent should do)

1. Regenerate and land the shared S2 conformance vectors once the compiler schema 2 emitter is complete; both SDKs already bind
   canonical overload imports and execute them without runtime overload dispatch.
2. A packaged distribution (header-only tarball or CMake package) to replace copy-by-hash once more than one team consumes it.
3. Typed verification (argument kinds at load) if untrusted bytecode ever becomes a use case; needs the schema's content in the host.
