#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST="${1:-$ROOT/build/stdlib_string_test}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/dsl-string-signatures-XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
COMPILER_ROOT="${LMSCRIPT_ROOT:-${HOME}/lmctl-mono/lmscript}"
if [[ -n "${LMSCRIPT_BIN:-}" ]]; then
  COMPILER=("$LMSCRIPT_BIN")
elif [[ -f "$COMPILER_ROOT/runner.ts" ]]; then
  cat > "$WORK/runner.mjs" <<EOF
import {runCli} from "$(printf '%s' "$COMPILER_ROOT/runner.ts" | sed 's/\\/\\\\/g; s/"/\\"/g')";
process.exitCode = await runCli(process.argv.slice(2));
EOF
  COMPILER=(node --experimental-strip-types "$WORK/runner.mjs")
elif command -v lmscript >/dev/null 2>&1; then
  COMPILER=(lmscript)   # the released CLI on PATH (npm install -g @lmctl-ai/lmctl, >= 0.2.346)
else
  echo "signature parity requires lmscript: install @lmctl-ai/lmctl (>= 0.2.346), or set LMSCRIPT_BIN or LMSCRIPT_ROOT" >&2
  exit 2
fi
node - "$ROOT" <<'JS_CHECK'
const fs = require('fs'), assert = require('assert');
const root = process.argv[2];
const fixture = JSON.parse(fs.readFileSync(root + '/tests/string.schema.json'));
const fragment = JSON.parse(fs.readFileSync(root + '/stdlib/string.schema-fragment.json'));
assert.deepStrictEqual(fixture.functions, fragment.functions, 'test schema differs from shipped catalog fragment');
JS_CHECK
"${COMPILER[@]}" --dsl-signatures "$ROOT/tests/string.schema.json" > "$WORK/compiler-signatures"
"$HOST" --signatures > "$WORK/host-signatures"
diff -u "$WORK/compiler-signatures" "$WORK/host-signatures"
echo "string catalog compiler/registry parity passed"
