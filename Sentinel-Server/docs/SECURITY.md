# Security Guide

This document outlines security considerations, best practices, and hardening steps for Sentinel Server deployments.

## Table of Contents

- [Security Overview](#security-overview)
- [Authentication Security](#authentication-security)
- [Network Security](#network-security)
- [System Security](#system-security)
- [Operational Security](#operational-security)
- [Monitoring Security](#monitoring-security)
- [Incident Response](#incident-response)

---

## Security Overview

Sentinel Server is designed with security in mind, but proper configuration and operational practices are essential for maintaining a secure deployment.

### Security Model

**Threat Model:**
- Remote attackers attempting unauthorized access
- Insider threats with network access
- Man-in-the-middle attacks
- Credential theft and misuse
- System compromise through vulnerabilities

**Security Boundaries:**
- Network perimeter (firewall, NAT)
- Application authentication (Bearer tokens)
- System privileges (service isolation)
- Data protection (encrypted storage)

---

## Authentication Security

### Password Management

#### Strong Password Generation

The installer generates 8-character passwords by default. For enhanced security:

```bash
# Generate longer passwords (modify installer or config manually)
# 16-character password example:
python3 -c "import random, string; print(''.join(random.choices(string.ascii_letters + string.digits + '!@#$%^&*', k=16)))"
```

#### Password Storage

- Passwords are hashed using Werkzeug's PBKDF2 implementation
- Configuration files are stored with restrictive permissions (600)
- Never store passwords in plain text

#### Password Rotation

```bash
#!/bin/bash
# Password rotation script

# Generate new password
NEW_PASSWORD=$(python3 -c "import random, string; print(''.join(random.choices(string.ascii_letters + string.digits, k=12)))")

# Hash the password
NEW_HASH=$(python3 -c "
from werkzeug.security import generate_password_hash
print(generate_password_hash('$NEW_PASSWORD'))
")

# Update configuration (requires manual editing)
echo "New password: $NEW_PASSWORD"
echo "New hash: $NEW_HASH"
echo "Update /etc/sentinel-server/config.json manually"
```

### API Authentication

#### Bearer Token Best Practices

```bash
# Good: Use environment variables
export SENTINEL_TOKEN="your_password_here"
curl -H "Authorization: Bearer $SENTINEL_TOKEN" http://server:port/metrics

# Bad: Password in command history
curl -H "Authorization: Bearer plaintext_password" http://server:port/metrics
```

#### Token Validation

```python
# Client-side token validation
import re

def validate_token(token):
    """Validate token format and strength"""
    if len(token) < 8:
        return False, "Token too short"
    
    if not re.match(r'^[a-zA-Z0-9]+$', token):
        return False, "Token contains invalid characters"
    
    # Add more validation as needed
    return True, "Token valid"

# Usage
token = "abc12345"
valid, message = validate_token(token)
print(f"Token validation: {message}")
```

---

## Network Security

### Firewall Configuration

#### UFW (Ubuntu/Debian)

```bash
# Basic firewall setup
sudo ufw enable

# Allow specific source IPs only
sudo ufw allow from 192.168.1.0/24 to any port YOUR_PORT
sudo ufw allow from 10.0.0.0/8 to any port YOUR_PORT

# Block all other access
sudo ufw deny YOUR_PORT

# Check rules
sudo ufw status numbered
```

#### iptables

```bash
# Allow specific networks
sudo iptables -A INPUT -s 192.168.1.0/24 -p tcp --dport YOUR_PORT -j ACCEPT
sudo iptables -A INPUT -s 10.0.0.0/8 -p tcp --dport YOUR_PORT -j ACCEPT

# Drop all other traffic to the port
sudo iptables -A INPUT -p tcp --dport YOUR_PORT -j DROP

# Save rules
sudo iptables-save > /etc/iptables/rules.v4
```

### Network Segmentation

#### VPN Access

```bash
# OpenVPN server configuration snippet
# Add to server.conf:
push "route 192.168.100.0 255.255.255.0"

# Client configuration for Sentinel access
# route 192.168.100.0 255.255.255.0 vpn_gateway
```

#### SSH Tunneling

```bash
# Create SSH tunnel for secure access
ssh -L 8080:localhost:YOUR_PORT user@your-server

# Use tunnel
curl http://localhost:8080/health
```

### TLS/SSL Configuration

#### Nginx Reverse Proxy with SSL

```nginx
# /etc/nginx/sites-available/sentinel-server
server {
    listen 443 ssl http2;
    server_name your-domain.com;
    
    # SSL Configuration
    ssl_certificate /etc/letsencrypt/live/your-domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your-domain.com/privkey.pem;
    
    # Security headers
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-XSS-Protection "1; mode=block" always;
    
    # Rate limiting
    limit_req zone=api burst=10 nodelay;
    
    # Proxy configuration
    location / {
        proxy_pass http://localhost:YOUR_PORT;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # Security
        proxy_hide_header X-Powered-By;
        
        # Timeouts
        proxy_connect_timeout 30s;
        proxy_send_timeout 30s;
        proxy_read_timeout 30s;
    }
    
    # Block sensitive endpoints from external access
    location ~ ^/(reboot|update) {
        deny all;
        return 403;
    }
}

# Rate limiting zone
http {
    limit_req_zone $binary_remote_addr zone=api:10m rate=1r/s;
}
```

#### Let's Encrypt Setup

```bash
# Install Certbot
sudo apt install certbot python3-certbot-nginx

# Get certificate
sudo certbot --nginx -d your-domain.com

# Auto-renewal
sudo systemctl enable certbot.timer
sudo systemctl start certbot.timer

# Test renewal
sudo certbot renew --dry-run
```

---

## System Security

### Service Hardening

#### Systemd Security Features

Enhanced service file with security options:

```ini
# /etc/systemd/system/sentinel-server.service
[Unit]
Description=Sentinel Server - System Monitoring and Management Service
After=network.target network-online.target
Wants=network-online.target

[Service]
Type=simple
User=sentinel
Group=sentinel
ExecStart=/usr/bin/python3 /opt/sentinel-server/src/sentinel_server.py
Restart=always
RestartSec=10

# Security settings
NoNewPrivileges=true
ProtectHome=true
ProtectSystem=strict
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true
PrivateTmp=true
PrivateDevices=true
RestrictSUIDSGID=true
RestrictRealtime=true
RestrictNamespaces=true
LockPersonality=true
MemoryDenyWriteExecute=true

# Network restrictions
IPAddressDeny=any
IPAddressAllow=localhost
IPAddressAllow=192.168.0.0/16
IPAddressAllow=10.0.0.0/8

# Resource limits
LimitNOFILE=1024
LimitNPROC=256
LimitMEMLOCK=0

# Read-write paths
ReadWritePaths=/etc/sentinel-server
ReadWritePaths=/var/log
ReadWritePaths=/tmp

[Install]
WantedBy=multi-user.target
```

#### Dedicated User Account

```bash
# Create dedicated user for Sentinel Server
sudo useradd --system --no-create-home --shell /bin/false sentinel

# Set file ownership
sudo chown -R sentinel:sentinel /opt/sentinel-server
sudo chown -R sentinel:sentinel /etc/sentinel-server

# Update service file to use sentinel user
sudo systemctl daemon-reload
sudo systemctl restart sentinel-server
```

### File System Security

#### Secure File Permissions

```bash
# Set secure permissions
sudo chmod 700 /etc/sentinel-server
sudo chmod 600 /etc/sentinel-server/config.json
sudo chmod 755 /opt/sentinel-server
sudo chmod 644 /opt/sentinel-server/src/sentinel_server.py

# Verify permissions
sudo find /opt/sentinel-server -ls
sudo find /etc/sentinel-server -ls
```

#### File Integrity Monitoring

```bash
# Install AIDE (Advanced Intrusion Detection Environment)
sudo apt install aide

# Configure AIDE for Sentinel Server
sudo tee -a /etc/aide/aide.conf << EOF
# Sentinel Server monitoring
/opt/sentinel-server R
/etc/sentinel-server R
/etc/systemd/system/sentinel-server.service R
EOF

# Initialize database
sudo aide --init
sudo mv /var/lib/aide/aide.db.new /var/lib/aide/aide.db

# Check for changes
sudo aide --check
```

### AppArmor Profile

```bash
# Create AppArmor profile for Sentinel Server
sudo tee /etc/apparmor.d/opt.sentinel-server.src.sentinel_server << EOF
#include <tunables/global>

/opt/sentinel-server/src/sentinel_server.py {
  #include <abstractions/base>
  #include <abstractions/python>
  
  capability setuid,
  capability setgid,
  
  /opt/sentinel-server/src/sentinel_server.py r,
  /etc/sentinel-server/ r,
  /etc/sentinel-server/** rw,
  /var/log/sentinel-server.log w,
  /proc/*/stat r,
  /proc/*/status r,
  /proc/meminfo r,
  /proc/loadavg r,
  /sys/class/net/ r,
  /sys/class/net/*/statistics/ r,
  /sys/class/net/*/statistics/* r,
  
  # Python libraries
  /usr/lib/python3*/** r,
  /usr/local/lib/python3*/** r,
  
  # Temporary files
  /tmp/ r,
  /tmp/** rw,
  
  # Network access
  network inet stream,
  network inet dgram,
}
EOF

# Load profile
sudo apparmor_parser -r /etc/apparmor.d/opt.sentinel-server.src.sentinel_server

# Check status
sudo aa-status | grep sentinel
```

---

## Operational Security

### Logging and Monitoring

#### Enhanced Logging

```python
# Enhanced logging configuration for sentinel_server.py
import logging
import logging.handlers
import os

def setup_security_logging():
    """Set up security-focused logging"""
    
    # Create security logger
    security_logger = logging.getLogger('security')
    security_logger.setLevel(logging.INFO)
    
    # Create handlers
    file_handler = logging.handlers.RotatingFileHandler(
        '/var/log/sentinel-security.log',
        maxBytes=10*1024*1024,  # 10MB
        backupCount=5
    )
    
    syslog_handler = logging.handlers.SysLogHandler(address='/dev/log')
    
    # Create formatter
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    file_handler.setFormatter(formatter)
    syslog_handler.setFormatter(formatter)
    
    # Add handlers
    security_logger.addHandler(file_handler)
    security_logger.addHandler(syslog_handler)
    
    return security_logger

# Usage in authentication decorator
def require_auth(self, f):
    def decorated(*args, **kwargs):
        auth_header = request.headers.get('Authorization')
        client_ip = request.environ.get('REMOTE_ADDR')
        
        if not auth_header or not auth_header.startswith('Bearer '):
            self.security_logger.warning(
                f"Authentication required - IP: {client_ip} - Endpoint: {request.endpoint}"
            )
            return jsonify({'error': 'Authentication required'}), 401
        
        password = auth_header.split(' ')[1]
        if not self.authenticate(password):
            self.security_logger.warning(
                f"Invalid authentication - IP: {client_ip} - Endpoint: {request.endpoint}"
            )
            return jsonify({'error': 'Invalid authentication'}), 401
        
        self.security_logger.info(
            f"Successful authentication - IP: {client_ip} - Endpoint: {request.endpoint}"
        )
        return f(*args, **kwargs)
    
    decorated.__name__ = f.__name__
    return decorated
```

#### Log Analysis

```bash
# Monitor failed authentication attempts
sudo tail -f /var/log/sentinel-security.log | grep "Invalid authentication"

# Count authentication failures by IP
sudo grep "Invalid authentication" /var/log/sentinel-security.log | \
    awk -F'IP: ' '{print $2}' | awk '{print $1}' | sort | uniq -c | sort -nr

# Monitor for suspicious activity
sudo grep -E "(reboot|update)" /var/log/sentinel-security.log | tail -20
```

### Intrusion Detection

#### fail2ban Configuration

```ini
# /etc/fail2ban/jail.local
[sentinel-auth]
enabled = true
port = YOUR_PORT
protocol = tcp
filter = sentinel-auth
logpath = /var/log/sentinel-security.log
maxretry = 3
bantime = 3600
findtime = 600
action = iptables-multiport[name=sentinel, port="YOUR_PORT", protocol=tcp]
```

```bash
# /etc/fail2ban/filter.d/sentinel-auth.conf
[Definition]
failregex = Invalid authentication - IP: <HOST>
ignoreregex =
```

### Backup and Recovery

#### Configuration Backup

```bash
#!/bin/bash
# Backup script for Sentinel Server configuration

BACKUP_DIR="/var/backups/sentinel-server"
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/sentinel-config-$DATE.tar.gz"

# Create backup directory
sudo mkdir -p "$BACKUP_DIR"

# Create backup
sudo tar czf "$BACKUP_FILE" \
    /etc/sentinel-server/ \
    /etc/systemd/system/sentinel-server.service \
    /opt/sentinel-server/src/

# Set permissions
sudo chmod 600 "$BACKUP_FILE"

# Keep only last 10 backups
sudo find "$BACKUP_DIR" -name "sentinel-config-*.tar.gz" -type f | \
    sort -r | tail -n +11 | sudo xargs rm -f

echo "Backup created: $BACKUP_FILE"
```

#### Disaster Recovery

```bash
#!/bin/bash
# Recovery script for Sentinel Server

BACKUP_FILE="$1"

if [ -z "$BACKUP_FILE" ] || [ ! -f "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup_file>"
    exit 1
fi

# Stop service
sudo systemctl stop sentinel-server

# Restore files
sudo tar xzf "$BACKUP_FILE" -C /

# Reload systemd
sudo systemctl daemon-reload

# Start service
sudo systemctl start sentinel-server

# Check status
sudo systemctl status sentinel-server
```

---

## Monitoring Security

### Security Metrics

```python
# Security monitoring script
import requests
import time
import json
from datetime import datetime

class SecurityMonitor:
    def __init__(self, log_file="/var/log/sentinel-security.log"):
        self.log_file = log_file
        self.failed_attempts = {}
        self.success_count = 0
    
    def analyze_logs(self):
        """Analyze security logs for threats"""
        try:
            with open(self.log_file, 'r') as f:
                lines = f.readlines()
            
            for line in lines:
                if "Invalid authentication" in line:
                    # Extract IP address
                    try:
                        ip = line.split("IP: ")[1].split(" -")[0]
                        self.failed_attempts[ip] = self.failed_attempts.get(ip, 0) + 1
                    except IndexError:
                        continue
                
                elif "Successful authentication" in line:
                    self.success_count += 1
            
            return True
        except Exception as e:
            print(f"Error analyzing logs: {e}")
            return False
    
    def get_security_report(self):
        """Generate security report"""
        report = {
            'timestamp': datetime.now().isoformat(),
            'successful_auth': self.success_count,
            'failed_attempts_by_ip': self.failed_attempts,
            'total_failed_attempts': sum(self.failed_attempts.values()),
            'unique_attackers': len(self.failed_attempts),
            'alerts': []
        }
        
        # Generate alerts
        for ip, count in self.failed_attempts.items():
            if count > 10:
                report['alerts'].append({
                    'type': 'brute_force',
                    'ip': ip,
                    'attempts': count,
                    'severity': 'high'
                })
            elif count > 5:
                report['alerts'].append({
                    'type': 'suspicious_activity',
                    'ip': ip,
                    'attempts': count,
                    'severity': 'medium'
                })
        
        return report
    
    def send_alert(self, report):
        """Send security alerts"""
        if report['alerts']:
            print(f"SECURITY ALERT: {len(report['alerts'])} threats detected")
            for alert in report['alerts']:
                print(f"- {alert['type']}: {alert['ip']} ({alert['attempts']} attempts)")

# Usage
monitor = SecurityMonitor()
if monitor.analyze_logs():
    report = monitor.get_security_report()
    monitor.send_alert(report)
    print(json.dumps(report, indent=2))
```

### Automated Security Checks

```bash
#!/bin/bash
# Automated security check script

echo "=== Sentinel Server Security Check ==="
echo "Timestamp: $(date)"
echo

# Check service status
echo "1. Service Status:"
if systemctl is-active --quiet sentinel-server; then
    echo "   ✓ Service is running"
else
    echo "   ✗ Service is not running"
fi

# Check file permissions
echo "2. File Permissions:"
CONFIG_PERMS=$(stat -c "%a" /etc/sentinel-server/config.json 2>/dev/null)
if [ "$CONFIG_PERMS" = "600" ]; then
    echo "   ✓ Config file permissions correct (600)"
else
    echo "   ✗ Config file permissions incorrect ($CONFIG_PERMS)"
fi

# Check for failed login attempts
echo "3. Security Log Analysis:"
FAILED_ATTEMPTS=$(grep -c "Invalid authentication" /var/log/sentinel-security.log 2>/dev/null || echo "0")
echo "   Failed authentication attempts: $FAILED_ATTEMPTS"

if [ "$FAILED_ATTEMPTS" -gt 100 ]; then
    echo "   ⚠ High number of failed attempts detected"
fi

# Check firewall status
echo "4. Firewall Status:"
if command -v ufw >/dev/null && ufw status | grep -q "Status: active"; then
    echo "   ✓ UFW firewall is active"
elif command -v iptables >/dev/null && iptables -L | grep -q "Chain INPUT"; then
    echo "   ✓ iptables rules present"
else
    echo "   ⚠ No active firewall detected"
fi

# Check for updates
echo "5. System Updates:"
UPDATES=$(apt list --upgradable 2>/dev/null | wc -l)
echo "   Available updates: $((UPDATES - 1))"

echo
echo "Security check completed."
```

---

## Incident Response

### Security Incident Playbook

#### 1. Unauthorized Access Detected

```bash
# Immediate response
sudo systemctl stop sentinel-server
sudo ufw deny YOUR_PORT

# Investigation
sudo grep "Invalid authentication" /var/log/sentinel-security.log | tail -50
sudo netstat -tulpn | grep YOUR_PORT
sudo lsof -i :YOUR_PORT

# Recovery
# 1. Change password
# 2. Review and update firewall rules
# 3. Check for system compromise
# 4. Restart service with new configuration
```

#### 2. Service Compromise

```bash
# Immediate isolation
sudo systemctl stop sentinel-server
sudo systemctl disable sentinel-server

# Evidence collection
sudo cp -r /opt/sentinel-server /tmp/evidence/
sudo cp -r /etc/sentinel-server /tmp/evidence/
sudo journalctl -u sentinel-server > /tmp/evidence/service.log

# System scan
sudo rkhunter --check
sudo chkrootkit

# Recovery
# 1. Reinstall from clean source
# 2. Restore configuration from secure backup
# 3. Update all credentials
# 4. Implement additional monitoring
```

### Contact Information

Maintain an incident response contact list:

```bash
# Emergency contacts
ADMIN_EMAIL="admin@company.com"
SECURITY_TEAM="security@company.com"
ON_CALL_PHONE="+1-555-0123"

# Automated alerting
echo "Security incident detected at $(date)" | \
    mail -s "Sentinel Server Security Alert" $SECURITY_TEAM
```

---

## Security Checklist

### Pre-Deployment

- [ ] Strong passwords generated and stored securely
- [ ] Firewall rules configured to restrict access
- [ ] TLS/SSL configured for encrypted communication
- [ ] Service user account created with minimal privileges
- [ ] File permissions set correctly
- [ ] Security logging enabled
- [ ] Backup procedures established

### Post-Deployment

- [ ] Regular security log review
- [ ] Failed authentication monitoring
- [ ] System update schedule established
- [ ] Incident response procedures documented
- [ ] Regular security assessments performed
- [ ] Access control reviewed quarterly

### Ongoing Maintenance

- [ ] Monitor security logs daily
- [ ] Update system packages monthly
- [ ] Rotate passwords quarterly
- [ ] Review firewall rules semi-annually
- [ ] Test backup/recovery procedures annually
- [ ] Conduct security audits annually

---

Remember: Security is an ongoing process, not a one-time configuration. Regular monitoring, updates, and assessments are essential for maintaining a secure Sentinel Server deployment.