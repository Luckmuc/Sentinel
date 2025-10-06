#!/bin/bash

# Sentinel Server Installer Script
# Compatible with Ubuntu and Debian systems
# This script installs and configures the Sentinel Server monitoring service

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
LOG_FILE="/var/log/sentinel-server-install.log"

# Functions
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

print_header() {
    echo -e "\n${CYAN}=================================${NC}"
    echo -e "${CYAN}  SENTINEL SERVER INSTALLER${NC}"
    echo -e "${CYAN}=================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
    log "SUCCESS: $1"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
    log "ERROR: $1"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
    log "WARNING: $1"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
    log "INFO: $1"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

check_os() {
    print_info "Checking operating system compatibility..."
    
    if [[ -f /etc/os-release ]]; then
        source /etc/os-release
        
        case "$ID" in
            ubuntu|debian)
                print_success "Compatible OS detected: $PRETTY_NAME"
                ;;
            *)
                print_error "Unsupported operating system: $PRETTY_NAME"
                print_info "This installer is designed for Ubuntu and Debian systems only"
                exit 1
                ;;
        esac
    else
        print_error "Unable to determine operating system"
        exit 1
    fi
}

check_systemd() {
    print_info "Checking systemd availability..."
    
    if ! command -v systemctl &> /dev/null; then
        print_error "systemctl not found. This installer requires systemd."
        exit 1
    fi
    
    if ! systemctl is-system-running --quiet 2>/dev/null && ! systemctl is-system-running | grep -E "(running|degraded)" &> /dev/null; then
        print_warning "systemd may not be running properly"
    else
        print_success "systemd is available and running"
    fi
}

install_dependencies() {
    print_info "Installing system dependencies..."
    
    # Update package list
    print_info "Updating package list..."
    apt-get update -y >> "$LOG_FILE" 2>&1
    
    # Install required packages
    local packages=(
        "python3"
        "python3-pip"
        "python3-venv"
        "curl"
        "wget"
        "sudo"
    )
    
    for package in "${packages[@]}"; do
        print_info "Installing $package..."
        apt-get install -y "$package" >> "$LOG_FILE" 2>&1
        print_success "$package installed"
    done
}

install_python_dependencies() {
    print_info "Installing Python dependencies..."
    
    # Create virtual environment
    python3 -m venv "$INSTALL_DIR/venv"
    source "$INSTALL_DIR/venv/bin/activate"
    
    # Upgrade pip
    pip install --upgrade pip >> "$LOG_FILE" 2>&1
    
    # Install required Python packages
    pip install flask==2.3.3 psutil==5.9.5 requests==2.31.0 cryptography==41.0.4 werkzeug==2.3.7 >> "$LOG_FILE" 2>&1
    
    print_success "Python dependencies installed"
}

create_directories() {
    print_info "Creating required directories..."
    
    # Create installation directory
    mkdir -p "$INSTALL_DIR/src"
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$(dirname "$LOG_FILE")"
    
    # Set permissions
    chmod 755 "$INSTALL_DIR"
    chmod 700 "$CONFIG_DIR"
    
    print_success "Directories created"
}

install_files() {
    print_info "Installing Sentinel Server files..."
    
    # Copy source files (this assumes the script is run from the project directory)
    if [[ -f "src/sentinel_server.py" ]]; then
        cp "src/sentinel_server.py" "$INSTALL_DIR/src/"
        chmod 755 "$INSTALL_DIR/src/sentinel_server.py"
        print_success "Source files copied"
    else
        print_error "Source files not found. Please run this script from the Sentinel Server project directory."
        exit 1
    fi
    
    # Copy service file
    if [[ -f "config/sentinel-server.service" ]]; then
        cp "config/sentinel-server.service" "$SERVICE_FILE"
        print_success "Service file installed"
    else
        print_error "Service file not found."
        exit 1
    fi
}

configure_firewall() {
    print_info "Configuring firewall..."
    
    # Check if ufw is installed and active
    if command -v ufw &> /dev/null; then
        if ufw status | grep -q "Status: active"; then
            print_warning "UFW firewall is active. You may need to manually allow the Sentinel Server port."
            print_info "Use: ufw allow <port_number> where <port_number> is shown at the end of installation"
        else
            print_info "UFW firewall is installed but not active"
        fi
    else
        print_info "UFW firewall not found - skipping firewall configuration"
    fi
}

create_user() {
    print_info "Setting up service user..."
    
    # Sentinel Server runs as root to access system information and perform updates
    # This is necessary for the monitoring and management functionality
    print_warning "Sentinel Server will run as root for system management capabilities"
    print_info "Ensure you use strong authentication and restrict network access"
}

install_service() {
    print_info "Installing and configuring systemd service..."
    
    # Reload systemd
    systemctl daemon-reload
    
    # Enable service
    systemctl enable sentinel-server.service
    print_success "Service enabled for automatic startup"
    
    # Start service
    systemctl start sentinel-server.service
    print_success "Service started"
    
    # Check service status
    sleep 2
    if systemctl is-active --quiet sentinel-server.service; then
        print_success "Service is running successfully"
    else
        print_error "Service failed to start. Check logs with: journalctl -u sentinel-server.service"
        exit 1
    fi
}

get_connection_info() {
    print_info "Retrieving connection information..."
    
    # Wait a moment for the service to fully start and generate config
    sleep 3
    
    # Read configuration
    if [[ -f "$CONFIG_DIR/config.json" ]]; then
        local port=$(grep -o '"port": [0-9]*' "$CONFIG_DIR/config.json" | grep -o '[0-9]*')
        local server_ip=$(hostname -I | awk '{print $1}')
        
        # Try to get external IP as well
        local external_ip=$(curl -s ifconfig.me 2>/dev/null || echo "Unable to determine")
        
        echo -e "\n${GREEN}=================================${NC}"
        echo -e "${GREEN}   INSTALLATION COMPLETED${NC}"
        echo -e "${GREEN}=================================${NC}"
        echo -e "${CYAN}Connection Information:${NC}"
        echo -e "${CYAN}  Local IP:     ${YELLOW}$server_ip${NC}"
        echo -e "${CYAN}  External IP:  ${YELLOW}$external_ip${NC}"
        echo -e "${CYAN}  Port:         ${YELLOW}$port${NC}"
        echo -e "${CYAN}  Password:     ${YELLOW}$(get_password_from_logs)${NC}"
        echo -e "${GREEN}=================================${NC}"
        echo -e "${BLUE}API Endpoints:${NC}"
        echo -e "${BLUE}  Health:   GET  http://$server_ip:$port/health${NC}"
        echo -e "${BLUE}  Metrics:  GET  http://$server_ip:$port/metrics${NC}"
        echo -e "${BLUE}  Update:   POST http://$server_ip:$port/update${NC}"
        echo -e "${BLUE}  Reboot:   POST http://$server_ip:$port/reboot${NC}"
        echo -e "${GREEN}=================================${NC}"
        echo -e "${YELLOW}IMPORTANT SECURITY NOTES:${NC}"
        echo -e "${YELLOW}  • Save the password securely${NC}"
        echo -e "${YELLOW}  • Use HTTPS in production${NC}"
        echo -e "${YELLOW}  • Restrict network access${NC}"
        echo -e "${YELLOW}  • Monitor access logs${NC}"
        echo -e "${GREEN}=================================${NC}\n"
        
    else
        print_warning "Configuration file not found. Check service logs for connection details."
        print_info "Use: journalctl -u sentinel-server.service | grep 'SENTINEL SERVER STARTED' -A 10"
    fi
}

get_password_from_logs() {
    # Try to extract password from service logs
    local password=$(journalctl -u sentinel-server.service --no-pager -q | grep "Password:" | tail -1 | awk '{print $2}')
    if [[ -n "$password" ]]; then
        echo "$password"
    else
        echo "Check service logs: journalctl -u sentinel-server.service"
    fi
}

cleanup() {
    print_info "Cleaning up temporary files..."
    # Add any cleanup tasks here
    print_success "Cleanup completed"
}

show_next_steps() {
    echo -e "\n${CYAN}Next Steps:${NC}"
    echo -e "${BLUE}  1. Test the service: curl http://localhost:<port>/health${NC}"
    echo -e "${BLUE}  2. Check service status: systemctl status sentinel-server${NC}"
    echo -e "${BLUE}  3. View logs: journalctl -u sentinel-server.service -f${NC}"
    echo -e "${BLUE}  4. Configure firewall if needed${NC}"
    echo -e "${BLUE}  5. Set up monitoring client connection${NC}\n"
}

# Main installation process
main() {
    print_header
    
    # Pre-installation checks
    check_root
    check_os
    check_systemd
    
    # Create log file
    touch "$LOG_FILE"
    chmod 644 "$LOG_FILE"
    
    log "=== Sentinel Server Installation Started ==="
    
    # Installation steps
    create_directories
    install_dependencies
    install_files
    install_python_dependencies
    configure_firewall
    create_user
    install_service
    
    # Post-installation
    get_connection_info
    show_next_steps
    cleanup
    
    log "=== Sentinel Server Installation Completed ==="
    print_success "Installation completed successfully!"
}

# Handle script interruption
trap 'print_error "Installation interrupted"; exit 1' INT TERM

# Run main function
main "$@"