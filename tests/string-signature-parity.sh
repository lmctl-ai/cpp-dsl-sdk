#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPILER_ROOT="${LMSCRIPT_ROOT:-${HOME}/lmctl-mono/lmscript}"
if [[ -n "${LMSCRIPT_BIN:-}" ]]; then
  COMPILER=("$LMSCRIPT_BIN")
elif [[ -f "$COMPILER_ROOT/runner.ts" ]]; then
  WORK="$(mktemp -d "${TMPDIR:-/tmp}/dsl-string-signatures-XXXXXX")"
  trap 'rm -rf "$WORK"' EXIT
  cat > "$WORK/runner.mjs" <<EOF
import {runCli} from "$(printf '%s' "$COMPILER_ROOT/runner.ts" | sed 's/\\/\\\\/g; s/"/\\"/g')";
process.exitCode = await runCli(process.argv.slice(2));
EOF
  COMPILER=(node --experimental-strip-types "$WORK/runner.mjs")
else
  echo "signature parity requires lmscript: set LMSCRIPT_BIN or LMSCRIPT_ROOT" >&2
  exit 2
fi
"${COMPILER[@]}" --dsl-signatures "$ROOT/tests/string.schema.json" > "$ROOT/build/string.compiler-signatures"
"$ROOT/build/stdlib_string_test" --signatures > "$ROOT/build/string.host-signatures"
diff -u "$ROOT/build/string.compiler-signatures" "$ROOT/build/string.host-signatures"
echo "string catalog compiler/registry parity passed"
