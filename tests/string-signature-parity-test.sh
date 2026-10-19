#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/dsl-signature-eol-XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cat > "$WORK/compiler" <<'COMPILER'
#!/usr/bin/env bash
printf 'fn str_len(string) -> int\n'
COMPILER
cat > "$WORK/host" <<'HOST'
#!/usr/bin/env bash
case "${PARITY_CASE:-crlf}" in
  crlf) printf 'fn str_len(string) -> int\r\n' ;;
  mismatch) printf 'fn str_len(string) -> float\r\n' ;;
  lone-cr) printf 'fn str_len(string) -> in\rt\n' ;;
esac
HOST
chmod +x "$WORK/compiler" "$WORK/host"
LMSCRIPT_BIN="$WORK/compiler" bash "$ROOT/tests/string-signature-parity.sh" "$WORK/host" > "$WORK/result" 2>&1
for mode in mismatch lone-cr; do
  if PARITY_CASE="$mode" LMSCRIPT_BIN="$WORK/compiler" bash "$ROOT/tests/string-signature-parity.sh" "$WORK/host" > "$WORK/result" 2>&1; then
    echo "fail: signature parity accepted $mode" >&2
    exit 1
  fi
  # Ensure the negative test reached the comparison, rather than failing setup.
  grep -q '^@@ ' "$WORK/result"
done
echo 'signature parity accepts CRLF and rejects signature content changes'
