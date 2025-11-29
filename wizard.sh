#!/bin/bash

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SXS_STD_REPO="https://github.com/bosley/sxs-std.git"
SXS_HOME="${SXS_HOME:-$HOME/.sxs}"
SXS_REPO_DIR="$SXS_HOME/.repo"

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
    echo "  --install      Install sxs-std to ~/.sxs"
    echo "  --reinstall    Force reinstall (removes existing installation)"
    echo "  --uninstall    Remove sxs-std installation"
    echo "  --check        Check installation status and show directory tree"
    echo "  --build        Build the main sxs project"
    echo "  --test         Build and run all tests (unit + kernel)"
    echo
    exit 1
}

check_installation() {
    print_header
    
    if [ ! -d "$SXS_HOME" ]; then
        echo -e "${RED}✗ sxs-std is not installed${NC}"
        echo -e "  Directory ~/.sxs does not exist"
        echo
        echo -e "${YELLOW}Run: $0 --install${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ sxs-std installation found${NC}"
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
        if [ -d "$SXS_REPO_DIR" ]; then
            echo -e "${YELLOW}Source Repository:${NC}"
            echo "  $SXS_REPO_DIR"
            if [ -d "$SXS_REPO_DIR/.git" ]; then
                cd "$SXS_REPO_DIR"
                COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
                BRANCH=$(git branch --show-current 2>/dev/null || echo "unknown")
                echo "  Branch: $BRANCH"
                echo "  Commit: $COMMIT"
            fi
        fi
    fi
    echo
}

install_sxs_std() {
    local force_reinstall=$1
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    
    print_header
    
    if [ -d "$SXS_HOME" ] && [ "$force_reinstall" != "true" ]; then
        echo -e "${YELLOW}⚠ sxs-std appears to be already installed at ~/.sxs${NC}"
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
    mkdir -p "$SXS_HOME"
    
    if [ -d "$SXS_REPO_DIR" ]; then
        echo -e "${YELLOW}Updating existing sxs-std repository...${NC}"
        cd "$SXS_REPO_DIR"
        git pull
    else
        echo -e "${YELLOW}Cloning sxs-std to ~/.sxs/.repo...${NC}"
        git clone "$SXS_STD_REPO" "$SXS_REPO_DIR"
        cd "$SXS_REPO_DIR"
    fi
    
    if [ -d "build" ]; then
        echo -e "${YELLOW}Cleaning previous build...${NC}"
        rm -rf build
    fi
    
    mkdir build
    cd build
    
    echo -e "${YELLOW}Configuring sxs-std...${NC}"
    cmake ..
    
    echo -e "${YELLOW}Building sxs-std...${NC}"
    make -j8
    
    echo -e "${YELLOW}Installing sxs-std to ~/.sxs...${NC}"
    make install
    
    echo
    echo -e "${GREEN}✓ sxs-std installed successfully!${NC}"
    echo
    
    echo -e "${YELLOW}Building main sxs project...${NC}"
    echo
    
    BUILD_DIR="$SCRIPT_DIR/build"
    
    cd "$SCRIPT_DIR"
    
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    cd "$BUILD_DIR"
    
    echo -e "${YELLOW}Configuring with CMake...${NC}"
    if ! cmake ..; then
        echo -e "${RED}✗ CMake configuration failed${NC}"
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
    
    SXS_BINARY="$BUILD_DIR/cmd/sxs/sxs"
    if [ -f "$SXS_BINARY" ]; then
        echo -e "${YELLOW}Installing sxs binary to $SXS_HOME/bin...${NC}"
        mkdir -p "$SXS_HOME/bin"
        cp "$SXS_BINARY" "$SXS_HOME/bin/sxs"
        chmod +x "$SXS_HOME/bin/sxs"
        echo -e "  ${GREEN}✓${NC} sxs binary installed"
    fi
    
    echo
    echo -e "${GREEN}✓ Installation complete!${NC}"
    echo
    echo -e "${BLUE}Installation Details:${NC}"
    echo -e "  Binary:  $SXS_HOME/bin/sxs"
    echo -e "  Source:  $SXS_HOME/.repo/"
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
    BUILD_DIR="$SCRIPT_DIR/build"
    
    if [ ! -d "$SXS_HOME" ]; then
        echo -e "${RED}✗ sxs-std is not installed${NC}"
        echo -e "${YELLOW}Run: $0 --install${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}Building sxs project...${NC}"
    echo
    
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${YELLOW}Creating build directory...${NC}"
        mkdir -p "$BUILD_DIR"
    fi
    
    cd "$BUILD_DIR"
    
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
        echo -e "${YELLOW}⚠ sxs-std is not installed${NC}"
        echo -e "  Directory $SXS_HOME does not exist"
        exit 0
    fi
    
    echo -e "${YELLOW}⚠ This will remove the sxs-std installation at:${NC}"
    echo -e "  $SXS_HOME"
    echo
    
    if [ -d "$SXS_REPO_DIR" ]; then
        echo -e "${BLUE}Contents to be removed:${NC}"
        echo -e "  - Binary (bin/)"
        echo -e "  - Source repository (.repo/)"
        echo -e "  - Libraries (lib/)"
        echo -e "  - Headers (include/)"
        echo -e "  - Kernels (lib/kernels/)"
    fi
    
    echo
    read -p "Are you sure you want to uninstall? (y/N): " -n 1 -r
    echo
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${BLUE}Uninstall cancelled${NC}"
        exit 0
    fi
    
    echo -e "${YELLOW}Removing sxs-std installation...${NC}"
    rm -rf "$SXS_HOME"
    
    echo
    echo -e "${GREEN}✓ sxs-std uninstalled successfully${NC}"
    echo -e "  Removed: $SXS_HOME"
    echo
}

run_tests() {
    print_header
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    BUILD_DIR="$SCRIPT_DIR/build"
    KERNEL_DIR="$SCRIPT_DIR/kernels"
    
    echo -e "${YELLOW}Building project first...${NC}"
    echo
    build_project
    
    echo -e "${YELLOW}Running unit tests...${NC}"
    echo
    cd "$BUILD_DIR"
    if ! ctest --output-on-failure; then
        echo -e "${RED}✗ Unit tests failed${NC}"
        exit 1
    fi
    
    echo
    echo -e "${GREEN}✓ Unit tests passed!${NC}"
    echo
    
    echo -e "${YELLOW}Running kernel tests...${NC}"
    echo
    cd "$KERNEL_DIR"
    if ! ./test.sh; then
        echo -e "${RED}✗ Kernel tests failed${NC}"
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

