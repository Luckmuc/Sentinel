# Sentinel Server

A comprehensive system monitoring and management service for Ubuntu and Debian systems. Sentinel Server provides real-time system metrics monitoring and remote management capabilities through a secure REST API.

## Features

### System Monitoring
- **CPU Usage**: Real-time CPU utilization percentage
- **Memory Usage**: RAM usage in GB with detailed statistics
- **Storage Usage**: Disk usage in GB for root filesystem
- **Network Monitoring**: Outbound network traffic in kbit/s
- **System Uptime**: System uptime with detailed boot information

### System Management
- **System Updates**: Remote execution of `apt update` and `apt upgrade`
- **System Reboot**: Secure remote system reboot functionality
- **Health Monitoring**: Service health checks and status reporting

### Security Features
- **Random Port Selection**: Automatically selects an available non-system port
- **Authentication**: Bearer token authentication with generated passwords
- **Secure Configuration**: Protected configuration files with restricted permissions
- **Service Isolation**: Runs as a systemd service with security restrictions

## Quick Start

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Luckmuc/Sentinel-Server.git
   cd Sentinel-Server
   ```

2. **Run the installer** (requires root privileges):
   ```bash
   sudo ./scripts/install.sh
   ```

3. **Note the connection details** displayed after installation:
   - IP Address
   - Port Number
   - Generated Password

📖 **[Detailed Installation Guide →](docs/INSTALLATION.md)**

### Usage

After installation, the service will automatically start and run in the background. Use the provided connection details to connect from your monitoring client.

💡 **[Usage Examples & Integrations →](docs/EXAMPLES.md)**

## API Documentation

For complete API documentation with detailed examples, authentication guides, and integration patterns, see the **[comprehensive API documentation](docs/API.md)**.

### Quick API Reference

**Available Endpoints:**
- `GET /health` - Service health check (public)
- `GET /metrics` - System metrics (protected)
- `POST /update` - System updates (protected) 
- `POST /reboot` - System reboot (protected)
- `GET /info` - Service information (public)

**Authentication:**
```bash
curl -H "Authorization: Bearer YOUR_PASSWORD" http://YOUR_IP:PORT/metrics
```

**Sample Response:**
```json
{
  "timestamp": "2025-10-05T12:00:00.000000",
  "cpu": 15.2,
  "memory": {"total_gb": 8.0, "used_gb": 4.2, "percentage": 52.5},
  "disk": {"total_gb": 250.0, "used_gb": 125.5, "percentage": 50.2},
  "network": {"outbound_kbits_per_sec": 150.75},
  "uptime": {"uptime_formatted": "1 day, 0:00:00"}
}
```

📖 **[Full API Documentation →](docs/API.md)**

## Configuration

Sentinel Server automatically generates its configuration during installation. The configuration is stored in `/etc/sentinel-server/config.json` with restricted permissions.

### Configuration Structure
```json
{
  "port": 12345,
  "password_hash": "$pbkdf2-sha256$...",
  "created_at": "2025-10-05T12:00:00.000000"
}
```

### Port Selection

The installer automatically selects an available port avoiding:
- System ports (1-1023)
- Common development ports (3000-3010, 5000-5010, 8000-8010, etc.)
- Commonly used service ports (8080-8090, 9000-9010)

Selected ports range from 10000-65535, excluding forbidden ranges.

## System Requirements

### Supported Operating Systems
- Ubuntu 18.04 LTS or later
- Debian 10 (Buster) or later

### Dependencies
- Python 3.6 or later
- systemd
- sudo privileges for installation

### Required Python Packages
- Flask 2.3.3
- psutil 5.9.5
- requests 2.31.0
- cryptography 41.0.4
- werkzeug 2.3.7

## Security Considerations

### Important Security Notes

1. **Password Security**: The generated password is displayed only once during installation. Store it securely.

2. **Network Security**: 
   - Configure firewall rules to restrict access
   - Consider using VPN or SSH tunneling for remote access
   - Use HTTPS in production environments

3. **System Privileges**: 
   - Service runs with root privileges for system management
   - Monitor access logs regularly
   - Restrict physical and network access

4. **Regular Updates**: 
   - Keep the system updated using the API or manual updates
   - Monitor security advisories

🔒 **[Complete Security Guide →](docs/SECURITY.md)**

### Firewall Configuration

If using UFW firewall, allow the Sentinel Server port:

```bash
sudo ufw allow PORT_NUMBER
sudo ufw reload
```

Replace `PORT_NUMBER` with the port displayed during installation.

## Service Management

### Service Control Commands

```bash
# Check service status
sudo systemctl status sentinel-server

# Start the service
sudo systemctl start sentinel-server

# Stop the service
sudo systemctl stop sentinel-server

# Restart the service
sudo systemctl restart sentinel-server

# View logs
sudo journalctl -u sentinel-server -f

# Disable auto-start
sudo systemctl disable sentinel-server

# Enable auto-start
sudo systemctl enable sentinel-server
```

### Log Files

- **Service logs**: `journalctl -u sentinel-server`
- **Application logs**: `/var/log/sentinel-server.log`
- **Installation logs**: `/var/log/sentinel-server-install.log`

## Uninstallation

To completely remove Sentinel Server:

```bash
sudo ./scripts/uninstall.sh
```

This will:
- Stop and disable the service
- Remove all installation files
- Remove configuration and log files
- Clean up systemd service registration

## Troubleshooting

### Common Issues

#### Service Won't Start
```bash
# Check service status and logs
sudo systemctl status sentinel-server
sudo journalctl -u sentinel-server -e

# Common causes:
# - Port already in use
# - Missing dependencies
# - Permission issues
```

#### Can't Connect to API
```bash
# Verify service is running
sudo systemctl is-active sentinel-server

# Check firewall settings
sudo ufw status

# Verify port and IP
sudo netstat -tulpn | grep sentinel
```

#### Authentication Failures
```bash
# Check if password is correct
# Password is displayed during installation
# Or check installation logs:
sudo cat /var/log/sentinel-server-install.log | grep "Password:"
```

### Getting Help

1. Check the service logs: `sudo journalctl -u sentinel-server`
2. Review installation logs: `sudo cat /var/log/sentinel-server-install.log`
3. Verify system requirements and dependencies
4. Check network connectivity and firewall settings

## Development

### Project Structure

```
Sentinel-Server/
├── src/
│   └── sentinel_server.py      # Main application
├── config/
│   └── sentinel-server.service # Systemd service file
├── scripts/
│   ├── install.sh              # Installation script
│   └── uninstall.sh           # Uninstallation script
├── requirements.txt            # Python dependencies
└── README.md                  # This file
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on Ubuntu/Debian systems
5. Submit a pull request

### Testing

To test the server locally during development:

```bash
# Install dependencies
pip3 install -r requirements.txt

# Run the server
python3 src/sentinel_server.py
```

Note: Some monitoring features require root privileges to access system information.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Changelog

### Version 1.0.0
- Initial release
- System monitoring (CPU, RAM, Storage, Network, Uptime)
- System management (Updates, Reboot)
- REST API with authentication
- Automatic installation and service setup
- Ubuntu and Debian support

## Support

For support, feature requests, or bug reports, please create an issue on GitHub.

## Documentation

📚 **[Complete Documentation →](docs/README.md)**

- **[Installation Guide](docs/INSTALLATION.md)** - Detailed setup instructions
- **[API Documentation](docs/API.md)** - Complete API reference
- **[Usage Examples](docs/EXAMPLES.md)** - Practical integration examples
- **[Security Guide](docs/SECURITY.md)** - Security best practices

---

**⚠️ Security Warning**: This service provides system management capabilities. Always use strong authentication, restrict network access, and monitor usage in production environments.