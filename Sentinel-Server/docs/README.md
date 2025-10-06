# Sentinel Server Documentation

Welcome to the comprehensive documentation for Sentinel Server - a system monitoring and management service for Ubuntu and Debian systems.

## 📚 Documentation Overview

This documentation is organized into focused guides to help you get the most out of Sentinel Server:

### Quick Start
- **[Main README](../README.md)** - Project overview and quick start guide
- **[Installation Guide](INSTALLATION.md)** - Detailed installation instructions
- **[API Documentation](API.md)** - Complete API reference with examples

### Usage & Examples
- **[Usage Examples](EXAMPLES.md)** - Practical examples in multiple programming languages
- **[Security Guide](SECURITY.md)** - Security best practices and hardening

### Reference
- **[Troubleshooting](#troubleshooting)** - Common issues and solutions
- **[FAQ](#frequently-asked-questions)** - Frequently asked questions

---

## 🚀 Getting Started

New to Sentinel Server? Start here:

1. **[Read the overview](../README.md#features)** - Understand what Sentinel Server does
2. **[Install the service](INSTALLATION.md)** - Get Sentinel Server running on your system
3. **[Explore the API](API.md)** - Learn about available endpoints
4. **[Try the examples](EXAMPLES.md)** - See practical usage scenarios
5. **[Secure your deployment](SECURITY.md)** - Implement security best practices

---

## 📖 Documentation Sections

### 🔧 [Installation Guide](INSTALLATION.md)

Complete installation instructions covering:
- **Prerequisites** - System requirements and dependencies
- **Automated Installation** - One-command setup (recommended)
- **Manual Installation** - Step-by-step manual setup
- **Post-Installation** - Firewall, network, and SSL configuration
- **Verification** - Testing your installation
- **Troubleshooting** - Common installation issues

### 🌐 [API Documentation](API.md)

Comprehensive API reference including:
- **Authentication** - Bearer token setup and usage
- **Endpoints** - All available API endpoints with examples
- **Request/Response Formats** - Data structures and formats
- **Error Handling** - HTTP status codes and error responses
- **Rate Limiting** - Usage guidelines and best practices

### 💡 [Usage Examples](EXAMPLES.md)

Practical examples for different scenarios:
- **Quick Start Examples** - Basic health checks and metrics
- **Monitoring Examples** - Continuous monitoring and alerting
- **System Management** - Updates and reboots
- **Programming Languages** - Python, Node.js, PowerShell examples
- **Advanced Use Cases** - Multi-server monitoring, Grafana integration
- **Integration Examples** - Prometheus, load balancers

### 🔒 [Security Guide](SECURITY.md)

Security best practices and hardening:
- **Authentication Security** - Password management and rotation
- **Network Security** - Firewall, VPN, and SSL configuration
- **System Security** - Service hardening and isolation
- **Operational Security** - Logging, monitoring, and intrusion detection
- **Incident Response** - Security incident handling procedures

---

## 🔍 Quick Reference

### Essential Commands

```bash
# Install Sentinel Server
sudo ./scripts/install.sh

# Check service status
sudo systemctl status sentinel-server

# View logs
sudo journalctl -u sentinel-server -f

# Verify installation
./scripts/verify.sh

# Uninstall completely
sudo ./scripts/uninstall.sh
```

### Essential API Calls

```bash
# Health check (public)
curl http://YOUR_IP:PORT/health

# Get system metrics (requires auth)
curl -H "Authorization: Bearer YOUR_PASSWORD" \
     http://YOUR_IP:PORT/metrics

# Update system (requires auth)
curl -X POST \
     -H "Authorization: Bearer YOUR_PASSWORD" \
     http://YOUR_IP:PORT/update

# Reboot system (requires auth)
curl -X POST \
     -H "Authorization: Bearer YOUR_PASSWORD" \
     http://YOUR_IP:PORT/reboot
```

### Key Files and Locations

```
/opt/sentinel-server/          # Installation directory
├── src/
│   └── sentinel_server.py     # Main application
└── venv/                      # Python virtual environment

/etc/sentinel-server/          # Configuration directory
└── config.json               # Main configuration file

/var/log/                     # Log files
├── sentinel-server.log       # Application logs
└── sentinel-server-install.log  # Installation logs

/etc/systemd/system/          # Service files
└── sentinel-server.service  # Systemd service file
```

---

## 🆘 Troubleshooting

### Common Issues

#### Service Won't Start

```bash
# Check service status and logs
sudo systemctl status sentinel-server
sudo journalctl -u sentinel-server -e

# Common solutions:
# 1. Check if port is already in use
sudo netstat -tulpn | grep :YOUR_PORT

# 2. Verify Python dependencies
python3 -c "import flask, psutil, requests, cryptography, werkzeug"

# 3. Check file permissions
ls -la /etc/sentinel-server/config.json
```

#### Cannot Connect to API

```bash
# Verify service is running
sudo systemctl is-active sentinel-server

# Check firewall
sudo ufw status
sudo iptables -L | grep YOUR_PORT

# Test local connection
curl http://localhost:YOUR_PORT/health
```

#### Authentication Failures

```bash
# Check password in logs (during first install)
sudo journalctl -u sentinel-server | grep "Password:"

# Or check installation logs
sudo grep "Password:" /var/log/sentinel-server-install.log

# Verify config file exists and is readable
sudo cat /etc/sentinel-server/config.json
```

### Getting Help

1. **Check the logs**: `sudo journalctl -u sentinel-server -f`
2. **Run verification**: `./scripts/verify.sh`
3. **Review installation logs**: `sudo cat /var/log/sentinel-server-install.log`
4. **Search documentation**: Use Ctrl+F in your browser
5. **Check GitHub issues**: Look for similar problems and solutions

---

## ❓ Frequently Asked Questions

### General Questions

**Q: What operating systems are supported?**
A: Ubuntu 18.04+ and Debian 10+ are officially supported.

**Q: Does Sentinel Server require root privileges?**
A: Yes, root privileges are required for system monitoring and management functions (updates, reboot, system metrics).

**Q: Can I run multiple Sentinel Server instances?**
A: Yes, each instance will automatically select a different port during installation.

**Q: How much system resources does Sentinel Server use?**
A: Minimal - typically <50MB RAM and <1% CPU under normal operation.

### Security Questions

**Q: Is the communication encrypted?**
A: By default, no. For production use, set up SSL/TLS using a reverse proxy like Nginx.

**Q: How secure is the password?**
A: Passwords are 8 characters by default and use PBKDF2 hashing. For enhanced security, use longer passwords and enable SSL.

**Q: Can I change the password after installation?**
A: Yes, generate a new password hash and update the configuration file manually.

### Technical Questions

**Q: What happens if the network is disconnected?**
A: Sentinel Server continues to run and collect metrics locally. Network monitoring will show zero activity.

**Q: How accurate are the metrics?**
A: Metrics are collected using the psutil library, which provides accurate system statistics directly from the OS.

**Q: Can I customize the monitoring interval?**
A: The server provides real-time data on request. Monitoring intervals are controlled by your client applications.

### Installation Questions

**Q: What if the installation fails?**
A: Check the installation logs at `/var/log/sentinel-server-install.log` and ensure you have sudo privileges and internet connectivity.

**Q: Can I install without internet access?**
A: No, the installer requires internet access to download Python dependencies. For air-gapped environments, pre-download the dependencies.

**Q: How do I upgrade to a newer version?**
A: Currently, uninstall the old version and install the new version. Backup your configuration first.

---

## 📄 Document Information

### Last Updated
This documentation was last updated on **October 5, 2025**.

### Version
Documentation version **1.0.0** for Sentinel Server **1.0.0**.

### Contributing
Found an error or want to improve the documentation? 
- Create an issue on GitHub
- Submit a pull request with improvements
- Contact the maintainers

### License
This documentation is provided under the same MIT license as Sentinel Server.

---

## 🔗 Quick Links

- **[GitHub Repository](https://github.com/Luckmuc/Sentinel-Server)**
- **[Installation Guide](INSTALLATION.md)**
- **[API Documentation](API.md)**
- **[Usage Examples](EXAMPLES.md)**
- **[Security Guide](SECURITY.md)**
- **[Main README](../README.md)**

---

**Need immediate help?** Check the [troubleshooting section](#troubleshooting) or review the [installation verification](INSTALLATION.md#verification) steps.