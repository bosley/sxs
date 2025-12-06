#!/usr/bin/env bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build-perf"

NCPUS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

usage() {
    echo "Usage: $0 [coverage|profile|both] [script.slp]"
    echo ""
    echo "Modes:"
    echo "  coverage  - Generate code coverage report"
    echo "  profile   - Generate performance profile with gprof"
    echo "  both      - Generate both coverage and profile (default)"
    echo ""
    echo "Examples:"
    echo "  $0 coverage min.sxs"
    echo "  $0 profile min.sxs"
    echo "  $0 both min.sxs"
    echo "  $0 min.sxs  (defaults to 'both')"
    exit 1
}

MODE="both"
SCRIPT=""

if [ $# -eq 0 ]; then
    usage
elif [ $# -eq 1 ]; then
    if [[ "$1" == *.slp ]] || [[ "$1" == *.sxs ]]; then
        SCRIPT="$1"
    elif [ "$1" == "coverage" ] || [ "$1" == "profile" ] || [ "$1" == "both" ]; then
        echo "Error: Script file required"
        usage
    else
        usage
    fi
elif [ $# -eq 2 ]; then
    MODE="$1"
    SCRIPT="$2"
    if [ "$MODE" != "coverage" ] && [ "$MODE" != "profile" ] && [ "$MODE" != "both" ]; then
        usage
    fi
else
    usage
fi

if [ ! -f "$SCRIPT" ]; then
    echo "Error: Script file '$SCRIPT' not found"
    exit 1
fi

SCRIPT_ABS="$(cd "$(dirname "$SCRIPT")" && pwd)/$(basename "$SCRIPT")"

echo "========================================"
echo "=== SXS Performance Analysis ==="
echo "========================================"
echo "Mode: $MODE"
echo "Script: $SCRIPT_ABS"
echo ""

if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

CFLAGS=""
LDFLAGS=""

if [ "$MODE" == "coverage" ] || [ "$MODE" == "both" ]; then
    CFLAGS="${CFLAGS} --coverage -fprofile-arcs -ftest-coverage"
    LDFLAGS="${LDFLAGS} --coverage"
fi

if [ "$MODE" == "profile" ] || [ "$MODE" == "both" ]; then
    CFLAGS="${CFLAGS} -pg"
    LDFLAGS="${LDFLAGS} -pg"
fi

echo "Building with instrumentation..."
cmake -DUSE_TCC=OFF \
      -DENABLE_ASAN=OFF \
      -DEXEC_TESTS=OFF \
      -DCMAKE_C_FLAGS="$CFLAGS" \
      -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" \
      -DCMAKE_SHARED_LINKER_FLAGS="$LDFLAGS" \
      .. > /dev/null

make -j${NCPUS} > /dev/null

echo "Build complete."
echo ""

SXS_BIN="${BUILD_DIR}/cmds/sxs/sxs"

if [ ! -f "$SXS_BIN" ]; then
    echo "Error: sxs binary not found at $SXS_BIN"
    exit 1
fi

echo "Running script: $SCRIPT_ABS"
echo "----------------------------------------"

if [ "$MODE" == "profile" ] || [ "$MODE" == "both" ]; then
    cd "$BUILD_DIR"
fi

"$SXS_BIN" "$SCRIPT_ABS"
EXIT_CODE=$?

echo "----------------------------------------"
echo "Script execution complete (exit code: $EXIT_CODE)"
echo ""

if [ "$MODE" == "coverage" ] || [ "$MODE" == "both" ]; then
    echo "========================================"
    echo "=== Generating Coverage Report ==="
    echo "========================================"
    
    if ! command -v lcov >/dev/null 2>&1; then
        echo "Warning: lcov not found. Install with: brew install lcov"
        echo "Falling back to gcov..."
        
        cd "${BUILD_DIR}/libs/sxs"
        gcov *.c
        echo ""
        echo "Coverage files generated in: ${BUILD_DIR}/libs/sxs/*.gcov"
    else
        lcov --capture --directory . --output-file coverage.info 2>/dev/null
        lcov --remove coverage.info '/usr/*' '/Library/*' --output-file coverage.info 2>/dev/null
        
        if command -v genhtml >/dev/null 2>&1; then
            genhtml coverage.info --output-directory coverage_html 2>/dev/null
            echo ""
            echo "Coverage report generated:"
            echo "  HTML: ${BUILD_DIR}/coverage_html/index.html"
            echo ""
            echo "Opening coverage report..."
            open "${BUILD_DIR}/coverage_html/index.html" 2>/dev/null || true
        fi
        
        echo ""
        echo "Coverage summary:"
        lcov --summary coverage.info 2>/dev/null
    fi
    echo ""
fi

if [ "$MODE" == "profile" ] || [ "$MODE" == "both" ]; then
    echo "========================================"
    echo "=== Generating Profile Report ==="
    echo "========================================"
    
    GMON_FILE="${BUILD_DIR}/gmon.out"
    if [ ! -f "$GMON_FILE" ]; then
        GMON_FILE=$(find "$BUILD_DIR" -name "gmon.out*" | head -n 1)
    fi
    
    if [ -z "$GMON_FILE" ] || [ ! -f "$GMON_FILE" ]; then
        echo "Warning: gmon.out not found. Profile data may not have been generated."
        echo "This can happen if the program exits too quickly."
    else
        if ! command -v gprof >/dev/null 2>&1; then
            echo "Warning: gprof not found. Cannot generate profile report."
        else
            PROFILE_TXT="${BUILD_DIR}/profile.txt"
            gprof "$SXS_BIN" "$GMON_FILE" > "$PROFILE_TXT" 2>/dev/null
            
            echo ""
            echo "Profile report generated: $PROFILE_TXT"
            echo ""
            echo "Top 20 functions by time:"
            echo "----------------------------------------"
            grep -A 20 "^  %   cumulative" "$PROFILE_TXT" | head -n 21 || echo "No profile data available"
            echo ""
            echo "Full report: $PROFILE_TXT"
        fi
    fi
    echo ""
fi

echo "========================================"
echo "=== Analysis Complete ==="
echo "========================================"
echo ""
echo "Results location: $BUILD_DIR"

if [ "$MODE" == "coverage" ] || [ "$MODE" == "both" ]; then
    if [ -d "${BUILD_DIR}/coverage_html" ]; then
        echo "  Coverage HTML: ${BUILD_DIR}/coverage_html/index.html"
    fi
fi

if [ "$MODE" == "profile" ] || [ "$MODE" == "both" ]; then
    if [ -f "${BUILD_DIR}/profile.txt" ]; then
        echo "  Profile report: ${BUILD_DIR}/profile.txt"
    fi
fi

echo ""
echo "Additional profiling options:"
echo ""
echo "For Instruments (requires Xcode):"
echo "  xcrun xctrace record --template 'Time Profiler' --launch -- $SXS_BIN $SCRIPT_ABS"
echo ""
echo "For Valgrind/Callgrind (install: brew install valgrind):"
echo "  valgrind --tool=callgrind $SXS_BIN $SCRIPT_ABS"
echo "  callgrind_annotate callgrind.out.*"
echo ""
echo "For sampling profiler (built-in macOS):"
echo "  sample $SXS_BIN 10 -file ${BUILD_DIR}/sample.txt &"
echo "  $SXS_BIN $SCRIPT_ABS"
echo ""

