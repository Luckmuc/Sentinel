#!/bin/bash

# Sentinel Server Installer Script (quiet by default)
# Compatible with Ubuntu and Debian systems
set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Flags
VERBOSE=0
for arg in "$@"; do
    case "$arg" in
        -v|--verbose) VERBOSE=1 ;;
    esac
done

# Paths
INSTALL_DIR="/opt/sentinel-server"
CONFIG_DIR="/etc/sentinel-server"
SERVICE_FILE="/etc/systemd/system/sentinel-server.service"
LOG_FILE="/var/log/sentinel-server-install.log"

# Logging helpers
log() { echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> "$LOG_FILE"; }
info() { [[ $VERBOSE -eq 1 ]] && echo -e "${BLUE}ℹ $1${NC}"; log "INFO: $1"; }
success() { echo -e "${GREEN}✓ $1${NC}"; log "SUCCESS: $1"; }
warn() { [[ $VERBOSE -eq 1 ]] && echo -e "${YELLOW}⚠ $1${NC}"; log "WARN: $1"; }
err() { echo -e "${RED}✗ $1${NC}"; log "ERROR: $1"; }

print_header() {
    echo -e "\n${CYAN}=================================${NC}"
    echo -e "${CYAN}  SENTINEL SERVER INSTALLER${NC}"
    echo -e "${CYAN}=================================${NC}\n"
}

check_root() {
    if [[ ${EUID:-1} -ne 0 ]]; then
        err "This script must be run as root (use sudo)"; exit 1
    fi
}

check_os() {
    info "Checking OS compatibility..."
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian) success "Detected: $PRETTY_NAME" ;;
            *) err "Unsupported OS: $PRETTY_NAME"; exit 1 ;;
        esac
    else
        err "Cannot determine OS"; exit 1
    fi
}

check_systemd() {
    info "Checking systemd..."
    if ! command -v systemctl >/dev/null 2>&1; then
        err "systemctl not found; systemd required."; exit 1
    fi
}

create_directories() {
    info "Creating directories..."
    mkdir -p "$INSTALL_DIR/src" "$CONFIG_DIR" "$(dirname "$LOG_FILE")"
    chmod 755 "$INSTALL_DIR"
    chmod 700 "$CONFIG_DIR"
    success "Directories ready"
}

install_dependencies() {
    info "Installing system dependencies..."
    apt-get update -y >> "$LOG_FILE" 2>&1
    apt-get install -y python3 python3-pip python3-venv curl wget >> "$LOG_FILE" 2>&1
    success "System dependencies installed"
}

install_files() {
    info "Installing files..."
    if [[ -f "src/sentinel_server.py" ]]; then
        cp "src/sentinel_server.py" "$INSTALL_DIR/src/"
        chmod 755 "$INSTALL_DIR/src/sentinel_server.py"
    else
        err "src/sentinel_server.py not found; run from project root"; exit 1
    fi
    if [[ -f "config/sentinel-server.service" ]]; then
        cp "config/sentinel-server.service" "$SERVICE_FILE"
        # Ensure service runs with the venv Python (has Flask/psutil installed)
        sed -i "s|^ExecStart=.*|ExecStart=$INSTALL_DIR/venv/bin/python $INSTALL_DIR/src/sentinel_server.py|" "$SERVICE_FILE"
    else
        err "config/sentinel-server.service not found"; exit 1
    fi
    success "Files installed"
}

install_python_dependencies() {
    info "Setting up Python venv and deps..."
    python3 -m venv "$INSTALL_DIR/venv"
    # shellcheck disable=SC1091
    source "$INSTALL_DIR/venv/bin/activate"
    pip install --upgrade pip >> "$LOG_FILE" 2>&1
    pip install flask==2.3.3 psutil==5.9.5 requests==2.31.0 cryptography==41.0.4 werkzeug==2.3.7 >> "$LOG_FILE" 2>&1
    success "Python dependencies installed"
}

configure_firewall() {
    if [[ $VERBOSE -eq 1 ]]; then
        if command -v ufw >/dev/null 2>&1; then
            if ufw status | grep -q "Status: active"; then
                warn "UFW active: allow server port after install (ufw allow <port>)"
            else
                info "UFW installed but not active"
            fi
        else
            info "UFW not found"
        fi
    fi
}

create_user() { :; }

install_service() {
    info "Installing systemd service..."
    systemctl daemon-reload
    # Ensure env variables are appropriate; optional drop-in
    mkdir -p /etc/systemd/system/sentinel-server.service.d
    cat > /etc/systemd/system/sentinel-server.service.d/override.conf <<EOF
[Service]
Environment=PYTHONPATH=$INSTALL_DIR
Environment=PYTHONUNBUFFERED=1
EOF
    systemctl daemon-reload
    systemctl enable sentinel-server.service >> "$LOG_FILE" 2>&1 || true
    systemctl restart sentinel-server.service || systemctl start sentinel-server.service
    sleep 2
    if systemctl is-active --quiet sentinel-server.service; then
        success "Service running"
    else
        err "Service failed to start; check: journalctl -u sentinel-server.service"; exit 1
    fi
}

get_password_from_logs() {
    journalctl -u sentinel-server.service --no-pager -q | grep "Password:" | tail -1 | awk '{print $2}'
}

get_connection_info() {
    info "Gathering connection info..."
    sleep 2
    if [[ -f "$CONFIG_DIR/config.json" ]]; then
        local port; port=$(grep -o '"port": [0-9]*' "$CONFIG_DIR/config.json" | grep -o '[0-9]*')
        local server_ip; server_ip=$(hostname -I | awk '{print $1}')
        local external_ip; external_ip=$(curl -s ifconfig.me 2>/dev/null || echo "n/a")
        local password; password=$(get_password_from_logs)

        echo -e "\n${GREEN}=================================${NC}"
        echo -e "${GREEN}   INSTALLATION COMPLETED${NC}"
        echo -e "${GREEN}=================================${NC}"
        echo -e "${CYAN}Local IP:${NC}     ${YELLOW}${server_ip:-n/a}${NC}"
        echo -e "${CYAN}External IP:${NC}  ${YELLOW}${external_ip}${NC}"
        echo -e "${CYAN}Port:${NC}         ${YELLOW}${port:-n/a}${NC}"
        echo -e "${CYAN}Password:${NC}     ${YELLOW}${password:-check journalctl}${NC}"
        echo -e "${GREEN}=================================${NC}\n"

        if [[ $VERBOSE -eq 1 ]]; then
            echo -e "${BLUE}Health:   GET  http://${server_ip}:${port}/health${NC}"
            echo -e "${BLUE}Metrics:  GET  http://${server_ip}:${port}/metrics  (Authorization: Bearer <password>)${NC}"
            echo -e "${BLUE}Update:   POST http://${server_ip}:${port}/update${NC}"
            echo -e "${BLUE}Reboot:   POST http://${server_ip}:${port}/reboot${NC}\n"
        fi
    else
        warn "${CONFIG_DIR}/config.json not found yet; service may still be initializing."
    fi
}

show_next_steps() {
    if [[ $VERBOSE -eq 1 ]]; then
        echo -e "\n${CYAN}Next Steps:${NC}"
        echo -e "${BLUE}  systemctl status sentinel-server${NC}"
        echo -e "${BLUE}  journalctl -u sentinel-server.service -f${NC}"
    fi
}

main() {
    print_header
    check_root
    check_os
    check_systemd

    # Prepare log file
    mkdir -p "$(dirname "$LOG_FILE")"; : > "$LOG_FILE"; chmod 644 "$LOG_FILE"
    log "=== Sentinel Server Installation Started ==="

    echo "- Creating directories ..."; create_directories
    echo "- Installing dependencies ..."; install_dependencies
    echo "- Installing files ..."; install_files
    echo "- Installing Python deps ..."; install_python_dependencies
    echo "- Configuring service ..."; install_service
    configure_firewall
    create_user

    get_connection_info
    show_next_steps

    log "=== Sentinel Server Installation Completed ==="
    success "Installation completed successfully!"
}

trap 'err "Installation interrupted"; exit 1' INT TERM

main "$@"