# Open items (what the next agent should do)

1. Regenerate and land the shared S2 conformance vectors once the compiler schema 2 emitter is complete; both SDKs already bind
   canonical overload imports and execute them without runtime overload dispatch.
2. A packaged distribution (header-only tarball or CMake package) to replace copy-by-hash once more than one team consumes it.
3. Typed verification (argument kinds at load) if untrusted bytecode ever becomes a use case; needs the schema's content in the host.
4. Expand the optional host-side `stdlib/` catalog only when a concrete host needs additional ordinary functions. `str_join` is
   intentionally absent because there is no list type in this catalog; list-backed helpers belong to a host schema that defines
   its collection type.
