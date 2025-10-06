#!/bin/bash

# Sentinel Server Installation Verification Script
# Tests the installation and basic functionality

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
CONFIG_DIR="/etc/sentinel-server"
CONFIG_FILE="$CONFIG_DIR/config.json"
LOG_FILE="/var/log/sentinel-server.log"

print_header() {
    echo -e "\n${CYAN}=================================${NC}"
    echo -e "${CYAN}  SENTINEL SERVER VERIFICATION${NC}"
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

check_installation() {
    print_info "Checking installation files..."
    
    local errors=0
    
    # Check installation directory
    if [[ -d "/opt/sentinel-server" ]]; then
        print_success "Installation directory exists"
    else
        print_error "Installation directory not found"
        ((errors++))
    fi
    
    # Check main script
    if [[ -f "/opt/sentinel-server/src/sentinel_server.py" ]]; then
        print_success "Main script exists"
    else
        print_error "Main script not found"
        ((errors++))
    fi
    
    # Check configuration directory
    if [[ -d "$CONFIG_DIR" ]]; then
        print_success "Configuration directory exists"
    else
        print_error "Configuration directory not found"
        ((errors++))
    fi
    
    # Check service file
    if [[ -f "/etc/systemd/system/sentinel-server.service" ]]; then
        print_success "Service file exists"
    else
        print_error "Service file not found"
        ((errors++))
    fi
    
    return $errors
}

check_service_status() {
    print_info "Checking service status..."
    
    local errors=0
    
    # Check if service is enabled
    if systemctl is-enabled --quiet sentinel-server.service; then
        print_success "Service is enabled"
    else
        print_error "Service is not enabled"
        ((errors++))
    fi
    
    # Check if service is active
    if systemctl is-active --quiet sentinel-server.service; then
        print_success "Service is running"
    else
        print_error "Service is not running"
        print_info "Service status:"
        systemctl status sentinel-server.service --no-pager -l
        ((errors++))
    fi
    
    return $errors
}

check_configuration() {
    print_info "Checking configuration..."
    
    local errors=0
    
    # Check config file
    if [[ -f "$CONFIG_FILE" ]]; then
        print_success "Configuration file exists"
        
        # Check file permissions
        local perms=$(stat -c "%a" "$CONFIG_FILE")
        if [[ "$perms" == "600" ]]; then
            print_success "Configuration file has correct permissions"
        else
            print_warning "Configuration file permissions: $perms (expected: 600)"
        fi
        
        # Validate JSON
        if python3 -m json.tool "$CONFIG_FILE" >/dev/null 2>&1; then
            print_success "Configuration file is valid JSON"
        else
            print_error "Configuration file is not valid JSON"
            ((errors++))
        fi
        
    else
        print_error "Configuration file not found"
        ((errors++))
    fi
    
    return $errors
}

check_connectivity() {
    print_info "Checking network connectivity..."
    
    local errors=0
    
    if [[ -f "$CONFIG_FILE" ]]; then
        local port=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['port'])" 2>/dev/null)
        
        if [[ -n "$port" ]]; then
            print_info "Testing connection to localhost:$port"
            
            # Test health endpoint
            if curl -s --max-time 5 "http://localhost:$port/health" >/dev/null; then
                print_success "Health endpoint is accessible"
            else
                print_error "Health endpoint is not accessible"
                ((errors++))
            fi
            
            # Test info endpoint
            if curl -s --max-time 5 "http://localhost:$port/info" >/dev/null; then
                print_success "Info endpoint is accessible"
            else
                print_error "Info endpoint is not accessible"
                ((errors++))
            fi
            
        else
            print_error "Could not determine port from configuration"
            ((errors++))
        fi
    else
        print_error "Configuration file not available for connectivity test"
        ((errors++))
    fi
    
    return $errors
}

check_dependencies() {
    print_info "Checking Python dependencies..."
    
    local errors=0
    local required_packages=("flask" "psutil" "requests" "cryptography" "werkzeug")
    
    for package in "${required_packages[@]}"; do
        if python3 -c "import $package" 2>/dev/null; then
            print_success "$package is installed"
        else
            print_error "$package is not installed"
            ((errors++))
        fi
    done
    
    return $errors
}

check_logs() {
    print_info "Checking log files..."
    
    local errors=0
    
    # Check application log
    if [[ -f "$LOG_FILE" ]]; then
        print_success "Application log file exists"
        
        # Check for recent entries
        if [[ -s "$LOG_FILE" ]]; then
            print_success "Application log has content"
        else
            print_warning "Application log is empty"
        fi
    else
        print_warning "Application log file not found (may not have been created yet)"
    fi
    
    # Check systemd journal
    if journalctl -u sentinel-server.service --no-pager -q >/dev/null 2>&1; then
        print_success "Service logs are available in journal"
    else
        print_warning "No service logs found in journal"
    fi
    
    return $errors
}

show_connection_info() {
    print_info "Retrieving connection information..."
    
    if [[ -f "$CONFIG_FILE" ]]; then
        local port=$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['port'])" 2>/dev/null)
        local server_ip=$(hostname -I | awk '{print $1}')
        
        echo -e "\n${GREEN}Connection Information:${NC}"
        echo -e "${CYAN}  IP Address: ${YELLOW}$server_ip${NC}"
        echo -e "${CYAN}  Port:       ${YELLOW}$port${NC}"
        echo -e "${CYAN}  Password:   ${YELLOW}Check installation logs or service output${NC}"
        
        echo -e "\n${BLUE}Test Commands:${NC}"
        echo -e "${BLUE}  Health:     ${NC}curl http://$server_ip:$port/health"
        echo -e "${BLUE}  Info:       ${NC}curl http://$server_ip:$port/info"
        echo -e "${BLUE}  Metrics:    ${NC}curl -H \"Authorization: Bearer YOUR_PASSWORD\" http://$server_ip:$port/metrics"
    else
        print_error "Configuration file not available"
    fi
}

show_useful_commands() {
    echo -e "\n${CYAN}Useful Commands:${NC}"
    echo -e "${BLUE}  Service status: ${NC}systemctl status sentinel-server"
    echo -e "${BLUE}  Service logs:   ${NC}journalctl -u sentinel-server -f"
    echo -e "${BLUE}  Config file:    ${NC}cat $CONFIG_FILE"
    echo -e "${BLUE}  Stop service:   ${NC}systemctl stop sentinel-server"
    echo -e "${BLUE}  Start service:  ${NC}systemctl start sentinel-server"
    echo -e "${BLUE}  Restart:        ${NC}systemctl restart sentinel-server"
}

main() {
    print_header
    
    local total_errors=0
    
    # Run all checks
    check_installation
    total_errors=$((total_errors + $?))
    
    check_service_status
    total_errors=$((total_errors + $?))
    
    check_configuration
    total_errors=$((total_errors + $?))
    
    check_dependencies
    total_errors=$((total_errors + $?))
    
    check_connectivity
    total_errors=$((total_errors + $?))
    
    check_logs
    total_errors=$((total_errors + $?))
    
    # Show results
    echo -e "\n${CYAN}Verification Results:${NC}"
    
    if [[ $total_errors -eq 0 ]]; then
        echo -e "${GREEN}✓ All checks passed! Sentinel Server is properly installed and running.${NC}"
        show_connection_info
    else
        echo -e "${RED}✗ $total_errors issue(s) found. Please review the errors above.${NC}"
        print_info "Check the installation logs and service status for more details."
    fi
    
    show_useful_commands
    echo
}

# Run verification
main "$@"