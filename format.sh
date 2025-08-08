#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found" >&2
  exit 1
fi

detect_cpus() {
  case "$(uname -s)" in
    Linux) nproc ;;
    Darwin) sysctl -n hw.ncpu ;;
    *) echo 1 ;;
  esac
}

CPUS="$(detect_cpus)"

case "$(uname -s)" in
  Linux)
    find "$ROOT" \
      \( -path "*/.git/*" -o -path "*/build/*" -o -path "*/cmake-build-*/*" -o -path "*/third_party/*" -o -path "*/external/*" -o -path "*/extern/*" -o -path "*/Extern/*" -o -path "*/vendor/*" -o -path "*/node_modules/*" \) -prune -o \
      -type f -regex ".*\\.\\(cpp\\|cxx\\|cc\\|hpp\\|hxx\\|hh\\|h\\)" -print0 | \
      xargs -0 -n 50 -P "$CPUS" clang-format -style=file -i
    ;;
  Darwin)
    find "$ROOT" \
      \( -path "*/.git/*" -o -path "*/build/*" -o -path "*/cmake-build-*/*" -o -path "*/third_party/*" -o -path "*/external/*" -o -path "*/extern/*" -o -path "*/Extern/*" -o -path "*/vendor/*" -o -path "*/node_modules/*" \) -prune -o \
      \( -name "*.hpp" -o -name "*.hxx" -o -name "*.hh" -o -name "*.h" \) -type f -print0 | \
      xargs -0 -n 50 -P "$CPUS" clang-format -style=file -i
    find "$ROOT" \
      \( -path "*/.git/*" -o -path "*/build/*" -o -path "*/cmake-build-*/*" -o -path "*/third_party/*" -o -path "*/external/*" -o -path "*/extern/*" -o -path "*/Extern/*" -o -path "*/vendor/*" -o -path "*/node_modules/*" \) -prune -o \
      \( -name "*.cpp" -o -name "*.cxx" -o -name "*.cc" \) -type f -print0 | \
      xargs -0 -n 50 -P "$CPUS" clang-format -style=file -i
    ;;
  *)
    find "$ROOT" \
      \( -path "*/.git/*" -o -path "*/build/*" -o -path "*/cmake-build-*/*" -o -path "*/third_party/*" -o -path "*/external/*" -o -path "*/extern/*" -o -path "*/Extern/*" -o -path "*/vendor/*" -o -path "*/node_modules/*" \) -prune -o \
      \( -name "*.hpp" -o -name "*.hxx" -o -name "*.hh" -o -name "*.h" \) -type f -print0 | \
      xargs -0 -n 50 clang-format -style=file -i
    find "$ROOT" \
      \( -path "*/.git/*" -o -path "*/build/*" -o -path "*/cmake-build-*/*" -o -path "*/third_party/*" -o -path "*/external/*" -o -path "*/extern/*" -o -path "*/Extern/*" -o -path "*/vendor/*" -o -path "*/node_modules/*" \) -prune -o \
      \( -name "*.cpp" -o -name "*.cxx" -o -name "*.cc" \) -type f -print0 | \
      xargs -0 -n 50 clang-format -style=file -i
    ;;
esac


