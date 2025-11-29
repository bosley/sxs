#!/bin/bash

set -e
set -o pipefail

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SXS_HOME="${SXS_HOME:-$HOME/.sxs}"

print_header() {
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    echo -e "${BLUE}  SXS Wizard${NC}"
    echo -e "${BLUE}════════════════════════════════════════${NC}"
    echo
}

print_usage() {
    echo "Usage: $0 [COMMAND]"
    echo
    echo "Commands:"
    echo "  --install      Build and install libs and apps to ~/.sxs"
    echo "  --reinstall    Force reinstall (removes existing installation)"
    echo "  --uninstall    Remove sxs installation"
    echo "  --check        Check installation status and show directory tree"
    echo "  --build        Build the apps project"
    echo "  --test         Build and run all tests (libs + apps)"
    echo
    exit 1
}

check_installation() {
    print_header
    
    if [ ! -d "$SXS_HOME" ]; then
        echo -e "${RED}✗ sxs is not installed${NC}"
        echo -e "  Directory ~/.sxs does not exist"
        echo
        echo -e "${YELLOW}Run: $0 --install${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ sxs installation found${NC}"
    echo
    
    if command -v tree &> /dev/null; then
        echo -e "${BLUE}Installation Structure:${NC}"
        tree -L 3 -C "$SXS_HOME" 2>/dev/null || ls -laR "$SXS_HOME"
    else
        echo -e "${BLUE}Installation Contents:${NC}"
        echo
        if [ -d "$SXS_HOME/bin" ]; then
            echo -e "${YELLOW}Binary:${NC}"
            ls -lh "$SXS_HOME/bin" 2>/dev/null || echo "  (none)"
        fi
        if [ -d "$SXS_HOME/lib" ]; then
            echo -e "${YELLOW}Libraries:${NC}"
            ls -lh "$SXS_HOME/lib" 2>/dev/null || echo "  (none)"
        fi
        if [ -d "$SXS_HOME/include" ]; then
            echo -e "${YELLOW}Headers:${NC}"
            ls -lR "$SXS_HOME/include" 2>/dev/null || echo "  (none)"
        fi
        if [ -d "$SXS_HOME/lib/kernels" ]; then
            echo -e "${YELLOW}Kernels:${NC}"
            ls -l "$SXS_HOME/lib/kernels" 2>/dev/null || echo "  (none)"
        fi
    fi
    echo
}

install_sxs_std() {
    local force_reinstall=$1
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    
    print_header
    
    if [ -d "$SXS_HOME" ] && [ "$force_reinstall" != "true" ]; then
        echo -e "${YELLOW}⚠ sxs installation appears to exist at ~/.sxs${NC}"
        echo
        read -p "Do you want to remove and reinstall? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo -e "${BLUE}Installation cancelled${NC}"
            exit 0
        fi
        force_reinstall=true
    fi
    
    if [ "$force_reinstall" = "true" ] && [ -d "$SXS_HOME" ]; then
        echo -e "${YELLOW}Removing existing installation...${NC}"
        rm -rf "$SXS_HOME"
    fi
    
    echo -e "${YELLOW}Creating ~/.sxs directory...${NC}"
    if ! mkdir -p "$SXS_HOME"; then
        echo -e "${RED}✗ Failed to create $SXS_HOME${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building libs...${NC}"
    echo
    
    LIBS_BUILD_DIR="$SCRIPT_DIR/libs/build"
    
    if ! cd "$SCRIPT_DIR/libs"; then
        echo -e "${RED}✗ Failed to change to libs directory${NC}"
        exit 1
    fi
    
    if [ -d "build" ]; then
        echo -e "${YELLOW}Cleaning previous libs build...${NC}"
        if ! rm -rf build; then
            echo -e "${RED}✗ Failed to clean libs build directory${NC}"
            exit 1
        fi
    fi
    
    if ! mkdir build; then
        echo -e "${RED}✗ Failed to create libs build directory${NC}"
        exit 1
    fi
    if ! cd build; then
        echo -e "${RED}✗ Failed to change to libs build directory${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Configuring libs with CMake...${NC}"
    if ! cmake ..; then
        echo -e "${RED}✗ Libs CMake configuration failed${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building libs with make -j8...${NC}"
    if ! make -j8; then
        echo -e "${RED}✗ Libs build failed${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Installing libs to ~/.sxs...${NC}"
    if ! make install; then
        echo -e "${RED}✗ Libs installation failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Libs installed successfully!${NC}"
    echo
    
    echo -e "${YELLOW}Building apps...${NC}"
    echo
    
    APPS_BUILD_DIR="$SCRIPT_DIR/apps/build"
    
    if ! cd "$SCRIPT_DIR/apps"; then
        echo -e "${RED}✗ Failed to change to apps directory${NC}"
        exit 1
    fi
    
    if [ -d "build" ]; then
        echo -e "${YELLOW}Cleaning previous apps build...${NC}"
        if ! rm -rf build; then
            echo -e "${RED}✗ Failed to clean apps build directory${NC}"
            exit 1
        fi
    fi
    
    if ! mkdir build; then
        echo -e "${RED}✗ Failed to create apps build directory${NC}"
        exit 1
    fi
    if ! cd build; then
        echo -e "${RED}✗ Failed to change to apps build directory${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Configuring apps with CMake...${NC}"
    if ! cmake ..; then
        echo -e "${RED}✗ Apps CMake configuration failed${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building apps with make -j8...${NC}"
    if ! make -j8; then
        echo -e "${RED}✗ Apps build failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Apps build completed successfully!${NC}"
    echo
    
    SXS_BINARY="$APPS_BUILD_DIR/cmd/sxs/sxs"
    if [ ! -f "$SXS_BINARY" ]; then
        echo -e "${RED}✗ SXS binary not found at $SXS_BINARY${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Installing sxs binary to $SXS_HOME/bin...${NC}"
    mkdir -p "$SXS_HOME/bin"
    if ! cp "$SXS_BINARY" "$SXS_HOME/bin/sxs"; then
        echo -e "${RED}✗ Failed to copy sxs binary${NC}"
        exit 1
    fi
    chmod +x "$SXS_HOME/bin/sxs"
    echo -e "  ${GREEN}✓${NC} sxs binary installed"
    
    echo
    echo -e "${GREEN}✓ Installation complete!${NC}"
    echo
    echo -e "${BLUE}Installation Details:${NC}"
    echo -e "  Binary:  $SXS_HOME/bin/sxs"
    echo -e "  Libs:    $SXS_HOME/lib/"
    echo -e "  Headers: $SXS_HOME/include/sxs/"
    echo -e "  Kernels: $SXS_HOME/lib/kernels/"
    echo
    echo -e "${YELLOW}Tip: Add these to your shell rc file:${NC}"
    echo -e "  export SXS_HOME=$SXS_HOME"
    echo -e "  export PATH=\$SXS_HOME/bin:\$PATH"
    echo
}

build_project() {
    print_header
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/apps/build"
    
    if [ ! -d "$SXS_HOME" ]; then
        echo -e "${RED}✗ libs not installed to ~/.sxs${NC}"
        echo -e "${YELLOW}Run: $0 --install${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building apps...${NC}"
    echo
    
    if ! cd "$SCRIPT_DIR/apps"; then
        echo -e "${RED}✗ Failed to change to apps directory${NC}"
        exit 1
    fi
    
    if [ ! -d "build" ]; then
        echo -e "${YELLOW}Creating build directory...${NC}"
        if ! mkdir -p build; then
            echo -e "${RED}✗ Failed to create build directory${NC}"
            exit 1
        fi
    fi
    
    if ! cd build; then
        echo -e "${RED}✗ Failed to change to build directory${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Configuring with CMake...${NC}"
    if ! cmake ..; then
        echo -e "${RED}✗ CMake configuration failed${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    if ! make clean; then
        echo -e "${RED}✗ Clean failed${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building with make -j8...${NC}"
    if ! make -j8; then
        echo -e "${RED}✗ Build failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Build completed successfully!${NC}"
    echo
}

uninstall_sxs_std() {
    print_header
    
    if [ ! -d "$SXS_HOME" ]; then
        echo -e "${YELLOW}⚠ sxs is not installed${NC}"
        echo -e "  Directory $SXS_HOME does not exist"
        exit 0
    fi
    
    echo -e "${YELLOW}⚠ This will remove the sxs installation at:${NC}"
    echo -e "  $SXS_HOME"
    echo
    
    echo -e "${BLUE}Contents to be removed:${NC}"
    echo -e "  - Binary (bin/)"
    echo -e "  - Libraries (lib/)"
    echo -e "  - Headers (include/)"
    echo -e "  - Kernels (lib/kernels/)"
    
    echo
    read -p "Are you sure you want to uninstall? (y/N): " -n 1 -r
    echo
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${BLUE}Uninstall cancelled${NC}"
        exit 0
    fi
    
    echo -e "${YELLOW}Removing sxs installation...${NC}"
    rm -rf "$SXS_HOME"
    
    echo
    echo -e "${GREEN}✓ sxs uninstalled successfully${NC}"
    echo -e "  Removed: $SXS_HOME"
    echo
}

run_tests() {
    print_header
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    LIBS_BUILD_DIR="$SCRIPT_DIR/libs/build"
    APPS_BUILD_DIR="$SCRIPT_DIR/apps/build"
    
    echo -e "${YELLOW}Building libs first...${NC}"
    echo
    
    if ! cd "$SCRIPT_DIR/libs"; then
        echo -e "${RED}✗ Failed to change to libs directory${NC}"
        exit 1
    fi
    
    if [ ! -d "build" ]; then
        if ! mkdir -p build; then
            echo -e "${RED}✗ Failed to create libs build directory${NC}"
            exit 1
        fi
    fi
    
    if ! cd build; then
        echo -e "${RED}✗ Failed to change to libs build directory${NC}"
        exit 1
    fi
    
    if ! cmake ..; then
        echo -e "${RED}✗ Libs CMake configuration failed${NC}"
        exit 1
    fi
    
    if ! make -j8; then
        echo -e "${RED}✗ Libs build failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Libs built successfully!${NC}"
    echo
    
    echo -e "${YELLOW}Running libs unit tests...${NC}"
    echo
    if ! cd "$LIBS_BUILD_DIR"; then
        echo -e "${RED}✗ Failed to change to libs build directory${NC}"
        exit 1
    fi
    if ! ctest --output-on-failure; then
        echo -e "${RED}✗ Libs unit tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Libs unit tests passed!${NC}"
    echo
    
    echo -e "${YELLOW}Running libs kernel tests...${NC}"
    echo
    if ! cd "$SCRIPT_DIR/libs/tests/kernel"; then
        echo -e "${RED}✗ Failed to change to libs kernel tests directory${NC}"
        exit 1
    fi
    if ! ./run.sh; then
        echo -e "${RED}✗ Libs kernel tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Libs kernel tests passed!${NC}"
    echo
    
    echo -e "${YELLOW}Building apps...${NC}"
    echo
    
    if ! cd "$SCRIPT_DIR/apps"; then
        echo -e "${RED}✗ Failed to change to apps directory${NC}"
        exit 1
    fi
    
    if [ ! -d "build" ]; then
        if ! mkdir -p build; then
            echo -e "${RED}✗ Failed to create apps build directory${NC}"
            exit 1
        fi
    fi
    
    if ! cd build; then
        echo -e "${RED}✗ Failed to change to apps build directory${NC}"
        exit 1
    fi
    
    if ! cmake ..; then
        echo -e "${RED}✗ Apps CMake configuration failed${NC}"
        exit 1
    fi
    
    if ! make -j8; then
        echo -e "${RED}✗ Apps build failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Apps built successfully!${NC}"
    echo
    
    echo -e "${YELLOW}Running apps unit tests...${NC}"
    echo
    if ! cd "$APPS_BUILD_DIR"; then
        echo -e "${RED}✗ Failed to change to apps build directory${NC}"
        exit 1
    fi
    if ! ctest --output-on-failure; then
        echo -e "${RED}✗ Apps unit tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Apps unit tests passed!${NC}"
    echo
    
    echo -e "${YELLOW}Running apps kernel tests...${NC}"
    echo
    if ! cd "$SCRIPT_DIR/apps/tests/integration/kernel"; then
        echo -e "${RED}✗ Failed to change to apps kernel tests directory${NC}"
        exit 1
    fi
    if ! ./test.sh; then
        echo -e "${RED}✗ Apps kernel tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Apps kernel tests passed!${NC}"
    echo
    
    echo -e "${YELLOW}Running apps integration tests...${NC}"
    echo
    if ! cd "$SCRIPT_DIR/apps/tests/integration/app"; then
        echo -e "${RED}✗ Failed to change to apps integration tests directory${NC}"
        exit 1
    fi
    if ! ./test.sh; then
        echo -e "${RED}✗ Apps integration tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo
}

if [ $# -eq 0 ]; then
    print_usage
fi

case "$1" in
    --install)
        install_sxs_std false
        ;;
    --reinstall)
        install_sxs_std true
        ;;
    --uninstall)
        uninstall_sxs_std
        ;;
    --check)
        check_installation
        ;;
    --build)
        build_project
        ;;
    --test)
        run_tests
        ;;
    *)
        echo -e "${RED}Error: Unknown command '$1'${NC}"
        echo
        print_usage
        ;;
esac

