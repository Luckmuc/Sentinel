# Installation Guide

This guide provides step-by-step instructions for installing Sentinel Server on Ubuntu and Debian systems.

## Prerequisites

### System Requirements

**Supported Operating Systems:**
- Ubuntu 18.04 LTS or later
- Ubuntu 20.04 LTS (recommended)
- Ubuntu 22.04 LTS (recommended)
- Debian 10 (Buster) or later
- Debian 11 (Bullseye) or later

**Hardware Requirements:**
- **CPU:** 1 core minimum (2+ cores recommended)
- **RAM:** 512MB minimum (1GB+ recommended)
- **Storage:** 100MB free space for installation
- **Network:** Internet connection for package downloads

**Software Requirements:**
- **systemd:** Required for service management
- **sudo:** Required for installation and system operations
- **curl/wget:** Required for external IP detection
- **Python 3.6+:** Will be installed if not present

### Pre-Installation Checklist

Before installing, ensure you have:

- [ ] Root or sudo access to the target system
- [ ] Internet connection for downloading dependencies
- [ ] No conflicting services on ports 10000-65535
- [ ] Sufficient disk space (at least 500MB free)
- [ ] Backup of important data (recommended)

## Installation Methods

### Method 1: Automated Installation (Recommended)

This is the easiest and recommended method for most users.

#### Step 1: Download Sentinel Server

```bash
# Clone the repository
git clone https://github.com/Luckmuc/Sentinel-Server.git
cd Sentinel-Server

# Or download and extract the ZIP file
wget https://github.com/Luckmuc/Sentinel-Server/archive/main.zip
unzip main.zip
cd Sentinel-Server-main
```

#### Step 2: Run the Installer

```bash
# Make the installer executable (if needed)
chmod +x scripts/install.sh

# Run the installer with sudo
sudo ./scripts/install.sh
```

#### Step 3: Save Connection Information

The installer will display connection information like this:

```
=================================
   INSTALLATION COMPLETED
=================================
Connection Information:
  Local IP:     192.168.1.100
  External IP:  203.0.113.1
  Port:         12345
  Password:     abc12345
=================================
```

**Important:** Save this information securely - you'll need it to connect to your Sentinel Server.

### Method 2: Manual Installation

For advanced users who want more control over the installation process.

#### Step 1: Install Dependencies

```bash
# Update package list
sudo apt update

# Install required packages
sudo apt install -y python3 python3-pip python3-venv curl wget sudo

# Install Python dependencies
pip3 install flask==2.3.3 psutil==5.9.5 requests==2.31.0 cryptography==41.0.4 werkzeug==2.3.7
```

#### Step 2: Create Installation Directories

```bash
# Create installation directory
sudo mkdir -p /opt/sentinel-server/src
sudo mkdir -p /etc/sentinel-server

# Set permissions
sudo chmod 755 /opt/sentinel-server
sudo chmod 700 /etc/sentinel-server
```

#### Step 3: Copy Files

```bash
# Copy main application
sudo cp src/sentinel_server.py /opt/sentinel-server/src/
sudo chmod 755 /opt/sentinel-server/src/sentinel_server.py

# Copy service file
sudo cp config/sentinel-server.service /etc/systemd/system/
```

#### Step 4: Install Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable and start service
sudo systemctl enable sentinel-server.service
sudo systemctl start sentinel-server.service
```

#### Step 5: Verify Installation

```bash
# Check service status
sudo systemctl status sentinel-server.service

# Get connection information from logs
sudo journalctl -u sentinel-server.service | grep "SENTINEL SERVER STARTED" -A 10
```

## Post-Installation Setup

### Firewall Configuration

If you're using UFW firewall, allow the Sentinel Server port:

```bash
# Replace PORT_NUMBER with your actual port
sudo ufw allow PORT_NUMBER
sudo ufw reload

# Check firewall status
sudo ufw status
```

For iptables:
```bash
# Replace PORT_NUMBER with your actual port
sudo iptables -A INPUT -p tcp --dport PORT_NUMBER -j ACCEPT
sudo iptables-save > /etc/iptables/rules.v4
```

### Network Configuration

For remote access, ensure:

1. **Router Port Forwarding** (if accessing from outside your network):
   - Forward the Sentinel Server port to your server's internal IP
   - Consult your router's documentation for specific steps

2. **Cloud Provider Security Groups** (AWS, GCP, Azure, etc.):
   - Add inbound rule for the Sentinel Server port
   - Restrict source IPs for security

### SSL/TLS Setup (Optional but Recommended)

For production use, consider setting up SSL/TLS:

#### Option 1: Reverse Proxy with Nginx

```bash
# Install Nginx
sudo apt install nginx

# Create Nginx configuration
sudo tee /etc/nginx/sites-available/sentinel-server << EOF
server {
    listen 443 ssl;
    server_name your-domain.com;
    
    ssl_certificate /path/to/your/certificate.crt;
    ssl_certificate_key /path/to/your/private.key;
    
    location / {
        proxy_pass http://localhost:YOUR_PORT;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
    }
}
EOF

# Enable site
sudo ln -s /etc/nginx/sites-available/sentinel-server /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

#### Option 2: Let's Encrypt with Certbot

```bash
# Install Certbot
sudo apt install certbot python3-certbot-nginx

# Get certificate
sudo certbot --nginx -d your-domain.com

# Auto-renewal (already set up by certbot)
sudo certbot renew --dry-run
```

## Verification

### Verify Installation

Run the verification script:
```bash
./scripts/verify.sh
```

### Manual Verification

Check each component manually:

```bash
# 1. Check service status
sudo systemctl status sentinel-server.service

# 2. Check if port is listening
sudo netstat -tulpn | grep sentinel

# 3. Test health endpoint
curl http://localhost:YOUR_PORT/health

# 4. Test metrics endpoint (requires authentication)
curl -H "Authorization: Bearer YOUR_PASSWORD" http://localhost:YOUR_PORT/metrics
```

### Test Remote Access

From another machine:
```bash
# Replace with your server's IP and credentials
curl http://YOUR_SERVER_IP:YOUR_PORT/health
```

## Troubleshooting Installation

### Common Issues

#### Issue: "Permission denied" during installation

**Solution:**
```bash
# Ensure you're running with sudo
sudo ./scripts/install.sh

# Check if you have sudo privileges
sudo -v
```

#### Issue: "Port already in use"

**Solution:**
```bash
# Check what's using the port
sudo netstat -tulpn | grep :PORT_NUMBER

# Stop conflicting service or let Sentinel Server choose a new port
sudo systemctl restart sentinel-server.service
```

#### Issue: Service fails to start

**Solution:**
```bash
# Check service logs
sudo journalctl -u sentinel-server.service -e

# Check Python dependencies
python3 -c "import flask, psutil, requests, cryptography, werkzeug"

# Reinstall dependencies if needed
pip3 install -r requirements.txt
```

#### Issue: Cannot connect remotely

**Solution:**
```bash
# Check firewall
sudo ufw status

# Check if service is binding to all interfaces
sudo netstat -tulpn | grep :YOUR_PORT

# Check service configuration
sudo systemctl status sentinel-server.service
```

#### Issue: "Module not found" errors

**Solution:**
```bash
# Install missing Python modules
sudo apt install python3-pip
pip3 install -r requirements.txt

# Or use system package manager
sudo apt install python3-flask python3-psutil python3-requests
```

### Getting Help

If you encounter issues:

1. **Check the logs:**
   ```bash
   sudo journalctl -u sentinel-server.service -f
   ```

2. **Run the verification script:**
   ```bash
   ./scripts/verify.sh
   ```

3. **Check installation logs:**
   ```bash
   sudo cat /var/log/sentinel-server-install.log
   ```

4. **Review system requirements** and ensure compatibility

5. **Check GitHub issues** for similar problems and solutions

## Next Steps

After successful installation:

1. **Test the API endpoints** using the examples in the [API documentation](API.md)
2. **Set up monitoring clients** to connect to your Sentinel Server
3. **Configure automated monitoring** using the provided scripts
4. **Review security settings** and harden access as needed
5. **Set up regular backups** of your configuration

## Uninstallation

To completely remove Sentinel Server:

```bash
# Run the uninstaller
sudo ./scripts/uninstall.sh

# Verify removal
./scripts/verify.sh  # Should show errors indicating removal
```

The uninstaller will:
- Stop and disable the service
- Remove all installation files
- Remove configuration and log files
- Clean up systemd service registration

---

**Need more help?** Check the [API documentation](API.md) or [troubleshooting guide](../README.md#troubleshooting).