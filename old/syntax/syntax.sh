#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTENSION_NAME="sxs"
TARGET_EDITOR="vscode"

show_usage() {
    echo "Usage: $0 [--install|--uninstall|--reinstall] [--cursor]"
    echo ""
    echo "Options:"
    echo "  --install      Install the SXS syntax extension (default)"
    echo "  --uninstall    Uninstall the SXS syntax extension"
    echo "  --reinstall    Reinstall the SXS syntax extension"
    echo "  --cursor       Target Cursor instead of VS Code"
    echo ""
    echo "Examples:"
    echo "  $0                    # Install to VS Code"
    echo "  $0 --cursor           # Install to Cursor"
    echo "  $0 --uninstall        # Uninstall from VS Code"
    echo "  $0 --reinstall --cursor  # Reinstall to Cursor"
}

get_extension_dir() {
    if [ "$TARGET_EDITOR" = "cursor" ]; then
        echo "$HOME/.cursor/extensions"
    else
        echo "$HOME/.vscode/extensions"
    fi
}

get_editor_name() {
    if [ "$TARGET_EDITOR" = "cursor" ]; then
        echo "Cursor"
    else
        echo "VS Code"
    fi
}

install_extension() {
    local ext_dir=$(get_extension_dir)
    local editor_name=$(get_editor_name)
    local link_path="$ext_dir/$EXTENSION_NAME"
    
    echo "Installing SXS syntax extension to $editor_name..."
    
    if [ ! -d "$ext_dir" ]; then
        echo "Error: $editor_name extensions directory not found at: $ext_dir"
        echo "Please ensure $editor_name is installed."
        exit 1
    fi
    
    if [ -L "$link_path" ]; then
        echo "Extension symlink already exists at: $link_path"
        local current_target=$(readlink "$link_path")
        if [ "$current_target" = "$SCRIPT_DIR" ]; then
            echo "Symlink is already correctly configured."
            return 0
        else
            echo "Warning: Symlink points to different location: $current_target"
            echo "Removing old symlink..."
            rm "$link_path"
        fi
    elif [ -e "$link_path" ]; then
        echo "Error: A file or directory already exists at: $link_path"
        echo "Please remove it manually and try again."
        exit 1
    fi
    
    echo "Creating symlink: $link_path -> $SCRIPT_DIR"
    ln -s "$SCRIPT_DIR" "$link_path"
    
    echo ""
    echo "✓ Successfully installed SXS syntax extension to $editor_name"
    echo ""
    echo "Next steps:"
    echo "  1. Restart $editor_name or reload the window"
    echo "  2. Open any .sxs file to see syntax highlighting"
}

uninstall_extension() {
    local ext_dir=$(get_extension_dir)
    local editor_name=$(get_editor_name)
    local link_path="$ext_dir/$EXTENSION_NAME"
    
    echo "Uninstalling SXS syntax extension from $editor_name..."
    
    if [ ! -e "$link_path" ]; then
        echo "Extension is not installed at: $link_path"
        return 0
    fi
    
    if [ -L "$link_path" ]; then
        echo "Removing symlink: $link_path"
        rm "$link_path"
        echo "✓ Successfully uninstalled SXS syntax extension from $editor_name"
    else
        echo "Warning: $link_path exists but is not a symlink"
        echo "Please remove it manually if needed."
        exit 1
    fi
}

reinstall_extension() {
    echo "Reinstalling SXS syntax extension..."
    uninstall_extension
    echo ""
    install_extension
}

ACTION="install"

while [ $# -gt 0 ]; do
    case "$1" in
        --install)
            ACTION="install"
            shift
            ;;
        --uninstall)
            ACTION="uninstall"
            shift
            ;;
        --reinstall)
            ACTION="reinstall"
            shift
            ;;
        --cursor)
            TARGET_EDITOR="cursor"
            shift
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        *)
            echo "Error: Unknown option: $1"
            echo ""
            show_usage
            exit 1
            ;;
    esac
done

case "$ACTION" in
    install)
        install_extension
        ;;
    uninstall)
        uninstall_extension
        ;;
    reinstall)
        reinstall_extension
        ;;
esac

