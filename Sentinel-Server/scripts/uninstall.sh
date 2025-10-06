#!/bin/bash

# Sentinel Server Uninstaller Script
# Removes Sentinel Server and all associated files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
INSTALL_DIR="/opt/sentinel-server"
CONFIG_DIR="/etc/sentinel-server"
SERVICE_FILE="/etc/systemd/system/sentinel-server.service"
LOG_FILE="/var/log/sentinel-server.log"

print_header() {
    echo -e "\n${CYAN}=================================${NC}"
    echo -e "${CYAN}  SENTINEL SERVER UNINSTALLER${NC}"
    echo -e "${CYAN}=================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

confirm_uninstall() {
    echo -e "${YELLOW}This will completely remove Sentinel Server and all its data.${NC}"
    echo -e "${YELLOW}This action cannot be undone.${NC}\n"
    
    read -p "Are you sure you want to continue? (yes/no): " confirm
    
    if [[ "$confirm" != "yes" ]]; then
        print_info "Uninstallation cancelled."
        exit 0
    fi
}

stop_service() {
    print_info "Stopping Sentinel Server service..."
    
    if systemctl is-active --quiet sentinel-server.service; then
        systemctl stop sentinel-server.service
        print_success "Service stopped"
    else
        print_info "Service was not running"
    fi
}

disable_service() {
    print_info "Disabling Sentinel Server service..."
    
    if systemctl is-enabled --quiet sentinel-server.service 2>/dev/null; then
        systemctl disable sentinel-server.service
        print_success "Service disabled"
    else
        print_info "Service was not enabled"
    fi
}

remove_service_file() {
    print_info "Removing service file..."
    
    if [[ -f "$SERVICE_FILE" ]]; then
        rm -f "$SERVICE_FILE"
        systemctl daemon-reload
        print_success "Service file removed"
    else
        print_info "Service file not found"
    fi
}

remove_files() {
    print_info "Removing installation files..."
    
    # Remove installation directory
    if [[ -d "$INSTALL_DIR" ]]; then
        rm -rf "$INSTALL_DIR"
        print_success "Installation directory removed"
    else
        print_info "Installation directory not found"
    fi
    
    # Remove configuration directory
    if [[ -d "$CONFIG_DIR" ]]; then
        rm -rf "$CONFIG_DIR"
        print_success "Configuration directory removed"
    else
        print_info "Configuration directory not found"
    fi
    
    # Remove log file
    if [[ -f "$LOG_FILE" ]]; then
        rm -f "$LOG_FILE"
        print_success "Log file removed"
    else
        print_info "Log file not found"
    fi
}

cleanup_user() {
    print_info "Cleaning up user configuration..."
    # Since Sentinel Server runs as root, no specific user cleanup needed
    print_success "User cleanup completed"
}

main() {
    print_header
    
    check_root
    confirm_uninstall
    
    print_info "Beginning uninstallation process..."
    
    stop_service
    disable_service
    remove_service_file
    remove_files
    cleanup_user
    
    echo -e "\n${GREEN}=================================${NC}"
    echo -e "${GREEN}  UNINSTALLATION COMPLETED${NC}"
    echo -e "${GREEN}=================================${NC}"
    print_success "Sentinel Server has been completely removed from your system"
    echo -e "${BLUE}Thank you for using Sentinel Server!${NC}\n"
}

# Handle script interruption
trap 'print_error "Uninstallation interrupted"; exit 1' INT TERM

# Run main function
main "$@"