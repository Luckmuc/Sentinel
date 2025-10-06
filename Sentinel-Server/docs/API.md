# Sentinel Server API Documentation

Welcome to the Sentinel Server API documentation. This guide provides comprehensive information about all available endpoints, authentication methods, request/response formats, and usage examples.

## Table of Contents

- [Authentication](#authentication)
- [Base URL](#base-url)
- [API Endpoints](#api-endpoints)
  - [Health Check](#health-check)
  - [System Metrics](#system-metrics)
  - [System Update](#system-update)
  - [System Reboot](#system-reboot)
  - [Service Information](#service-information)
- [Response Formats](#response-formats)
- [Error Handling](#error-handling)
- [Rate Limiting](#rate-limiting)
- [Examples](#examples)

---

## Authentication

Sentinel Server uses **Bearer Token Authentication** for protected endpoints. The authentication token is the password generated during installation.

### How to Authenticate

Include the authentication token in the `Authorization` header:

```http
Authorization: Bearer YOUR_PASSWORD
```

### Example
```bash
curl -H "Authorization: Bearer abc12345" http://192.168.1.100:12345/metrics
```

### Public vs Protected Endpoints

- **Public Endpoints**: No authentication required
  - `/health`
  - `/info`

- **Protected Endpoints**: Authentication required
  - `/metrics`
  - `/update`
  - `/reboot`

---

## Base URL

The base URL format is:
```
http://YOUR_SERVER_IP:PORT
```

Where:
- `YOUR_SERVER_IP` is the IP address of your server
- `PORT` is the randomly assigned port (displayed during installation)

**Example**: `http://192.168.1.100:12345`

---

## API Endpoints

### Health Check

Check if the Sentinel Server is running and responsive.

#### Endpoint
```http
GET /health
```

#### Authentication
🔓 **Public** - No authentication required

#### Request
```bash
curl http://192.168.1.100:12345/health
```

#### Response
```json
{
  "status": "healthy",
  "service": "sentinel-server",
  "timestamp": "2025-10-05T12:00:00.123456"
}
```

#### Response Fields
| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Service health status (`healthy` or `unhealthy`) |
| `service` | string | Service name identifier |
| `timestamp` | string | ISO 8601 timestamp of the response |

#### Status Codes
- `200 OK` - Service is healthy
- `500 Internal Server Error` - Service is experiencing issues

---

### System Metrics

Retrieve comprehensive system monitoring data including CPU, memory, disk, network, and uptime information.

#### Endpoint
```http
GET /metrics
```

#### Authentication
🔒 **Protected** - Bearer token required

#### Request
```bash
curl -H "Authorization: Bearer abc12345" \
     http://192.168.1.100:12345/metrics
```

#### Response
```json
{
  "timestamp": "2025-10-05T12:00:00.123456",
  "cpu": 15.2,
  "memory": {
    "total_gb": 8.0,
    "used_gb": 4.2,
    "available_gb": 3.8,
    "percentage": 52.5
  },
  "disk": {
    "total_gb": 250.0,
    "used_gb": 125.5,
    "free_gb": 124.5,
    "percentage": 50.2
  },
  "network": {
    "outbound_kbits_per_sec": 150.75,
    "total_sent_gb": 12.5,
    "total_received_gb": 45.2
  },
  "uptime": {
    "uptime_seconds": 86400,
    "uptime_formatted": "1 day, 0:00:00",
    "boot_time": "2025-10-04T12:00:00.000000"
  }
}
```

#### Response Fields

**Root Level:**
| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | string | ISO 8601 timestamp when metrics were collected |
| `cpu` | number | Current CPU usage percentage (0-100) |

**Memory Object:**
| Field | Type | Description |
|-------|------|-------------|
| `total_gb` | number | Total RAM in gigabytes |
| `used_gb` | number | Used RAM in gigabytes |
| `available_gb` | number | Available RAM in gigabytes |
| `percentage` | number | Memory usage percentage (0-100) |

**Disk Object:**
| Field | Type | Description |
|-------|------|-------------|
| `total_gb` | number | Total disk space in gigabytes |
| `used_gb` | number | Used disk space in gigabytes |
| `free_gb` | number | Free disk space in gigabytes |
| `percentage` | number | Disk usage percentage (0-100) |

**Network Object:**
| Field | Type | Description |
|-------|------|-------------|
| `outbound_kbits_per_sec` | number | Current outbound network speed in kilobits per second |
| `total_sent_gb` | number | Total data sent since boot in gigabytes |
| `total_received_gb` | number | Total data received since boot in gigabytes |

**Uptime Object:**
| Field | Type | Description |
|-------|------|-------------|
| `uptime_seconds` | number | System uptime in seconds |
| `uptime_formatted` | string | Human-readable uptime format |
| `boot_time` | string | ISO 8601 timestamp of last system boot |

#### Status Codes
- `200 OK` - Metrics retrieved successfully
- `401 Unauthorized` - Invalid or missing authentication
- `500 Internal Server Error` - Error collecting metrics

---

### System Update

Execute system package updates (`apt update` and `apt upgrade`).

#### Endpoint
```http
POST /update
```

#### Authentication
🔒 **Protected** - Bearer token required

#### Request
```bash
curl -X POST \
     -H "Authorization: Bearer abc12345" \
     http://192.168.1.100:12345/update
```

#### Response (Success)
```json
{
  "success": true,
  "update_output": "Hit:1 http://archive.ubuntu.com/ubuntu jammy InRelease\n...",
  "upgrade_output": "Reading package lists...\nBuilding dependency tree...\n...",
  "errors": ""
}
```

#### Response (Failure)
```json
{
  "success": false,
  "error": "Update operation timed out",
  "update_output": "...",
  "upgrade_output": "...",
  "errors": "E: Could not get lock /var/lib/dpkg/lock-frontend"
}
```

#### Response Fields
| Field | Type | Description |
|-------|------|-------------|
| `success` | boolean | Whether the update operation completed successfully |
| `update_output` | string | Output from `apt update` command |
| `upgrade_output` | string | Output from `apt upgrade` command |
| `errors` | string | Any error messages from the update process |
| `error` | string | General error description (only present on failure) |

#### Status Codes
- `200 OK` - Update completed successfully
- `401 Unauthorized` - Invalid or missing authentication
- `500 Internal Server Error` - Update failed

#### Important Notes
- ⚠️ **Timeout**: Update operations have a 30-minute timeout
- ⚠️ **Permissions**: Requires sudo privileges (automatically handled)
- ⚠️ **System Impact**: May temporarily affect system performance
- ⚠️ **Package Locks**: May fail if another package manager is running

---

### System Reboot

Initiate a system reboot.

#### Endpoint
```http
POST /reboot
```

#### Authentication
🔒 **Protected** - Bearer token required

#### Request
```bash
curl -X POST \
     -H "Authorization: Bearer abc12345" \
     http://192.168.1.100:12345/reboot
```

#### Response (Success)
```json
{
  "success": true,
  "message": "Reboot initiated"
}
```

#### Response (Failure)
```json
{
  "success": false,
  "error": "Permission denied"
}
```

#### Response Fields
| Field | Type | Description |
|-------|------|-------------|
| `success` | boolean | Whether the reboot was initiated successfully |
| `message` | string | Success message (only present on success) |
| `error` | string | Error description (only present on failure) |

#### Status Codes
- `200 OK` - Reboot initiated successfully
- `401 Unauthorized` - Invalid or missing authentication
- `500 Internal Server Error` - Reboot failed

#### Important Notes
- ⚠️ **Immediate Effect**: System will reboot immediately upon successful request
- ⚠️ **Connection Loss**: You will lose connection to the server
- ⚠️ **Service Restart**: Sentinel Server will restart automatically after boot
- ⚠️ **No Confirmation**: There is no additional confirmation prompt

---

### Service Information

Get information about the Sentinel Server service and available endpoints.

#### Endpoint
```http
GET /info
```

#### Authentication
🔓 **Public** - No authentication required

#### Request
```bash
curl http://192.168.1.100:12345/info
```

#### Response
```json
{
  "service": "Sentinel Server",
  "version": "1.0.0",
  "endpoints": [
    "/health",
    "/metrics",
    "/update",
    "/reboot",
    "/info"
  ],
  "authentication": "Bearer token required for protected endpoints"
}
```

#### Response Fields
| Field | Type | Description |
|-------|------|-------------|
| `service` | string | Service name |
| `version` | string | Current version number |
| `endpoints` | array | List of available API endpoints |
| `authentication` | string | Authentication method description |

#### Status Codes
- `200 OK` - Information retrieved successfully
- `500 Internal Server Error` - Error retrieving information

---

## Response Formats

### Content Type
All responses use `application/json` content type.

### Standard Response Structure
```json
{
  "field1": "value1",
  "field2": "value2",
  "timestamp": "2025-10-05T12:00:00.123456"
}
```

### Timestamp Format
All timestamps use ISO 8601 format: `YYYY-MM-DDTHH:MM:SS.ffffff`

---

## Error Handling

### Authentication Errors

#### Missing Authentication
```http
HTTP/1.1 401 Unauthorized
Content-Type: application/json

{
  "error": "Authentication required"
}
```

#### Invalid Authentication
```http
HTTP/1.1 401 Unauthorized
Content-Type: application/json

{
  "error": "Invalid authentication"
}
```

### Server Errors

#### Internal Server Error
```http
HTTP/1.1 500 Internal Server Error
Content-Type: application/json

{
  "error": "Internal server error description"
}
```

### Common HTTP Status Codes

| Code | Description | When it occurs |
|------|-------------|----------------|
| `200` | OK | Request successful |
| `401` | Unauthorized | Missing or invalid authentication |
| `404` | Not Found | Endpoint doesn't exist |
| `405` | Method Not Allowed | Wrong HTTP method |
| `500` | Internal Server Error | Server-side error |

---

## Rate Limiting

Currently, Sentinel Server does not implement rate limiting. However, consider these best practices:

- **Metrics**: Don't poll more frequently than every 10-30 seconds
- **Updates**: Don't run concurrent update operations
- **Reboots**: Allow adequate time between reboot requests

---

## Examples

### Complete Monitoring Script

```python
#!/usr/bin/env python3
import requests
import time
import json

class SentinelMonitor:
    def __init__(self, host, port, password):
        self.base_url = f"http://{host}:{port}"
        self.headers = {"Authorization": f"Bearer {password}"}
    
    def get_metrics(self):
        response = requests.get(f"{self.base_url}/metrics", headers=self.headers)
        return response.json() if response.status_code == 200 else None
    
    def check_health(self):
        response = requests.get(f"{self.base_url}/health")
        return response.json() if response.status_code == 200 else None

# Usage
monitor = SentinelMonitor("192.168.1.100", 12345, "abc12345")
metrics = monitor.get_metrics()
print(json.dumps(metrics, indent=2))
```

### Bash Monitoring Script

```bash
#!/bin/bash

# Configuration
SERVER_IP="192.168.1.100"
SERVER_PORT="12345"
PASSWORD="abc12345"
BASE_URL="http://$SERVER_IP:$SERVER_PORT"

# Function to get metrics
get_metrics() {
    curl -s -H "Authorization: Bearer $PASSWORD" \
         "$BASE_URL/metrics" | jq '.'
}

# Function to check health
check_health() {
    curl -s "$BASE_URL/health" | jq '.'
}

# Function to update system
update_system() {
    curl -s -X POST \
         -H "Authorization: Bearer $PASSWORD" \
         "$BASE_URL/update" | jq '.'
}

# Usage examples
echo "=== Health Check ==="
check_health

echo -e "\n=== System Metrics ==="
get_metrics

echo -e "\n=== System Update ==="
read -p "Run system update? (y/N): " confirm
if [[ $confirm == [yY] ]]; then
    update_system
fi
```

### PowerShell Example

```powershell
# Configuration
$ServerIP = "192.168.1.100"
$ServerPort = "12345"
$Password = "abc12345"
$BaseURL = "http://$ServerIP`:$ServerPort"

# Headers
$Headers = @{
    "Authorization" = "Bearer $Password"
    "Content-Type" = "application/json"
}

# Get metrics
$Metrics = Invoke-RestMethod -Uri "$BaseURL/metrics" -Headers $Headers -Method Get
$Metrics | ConvertTo-Json -Depth 10

# Check health
$Health = Invoke-RestMethod -Uri "$BaseURL/health" -Method Get
Write-Output "Server Status: $($Health.status)"
```

### Continuous Monitoring

```python
#!/usr/bin/env python3
import requests
import time
import json
from datetime import datetime

def monitor_system(host, port, password, interval=30):
    """Continuously monitor system metrics"""
    base_url = f"http://{host}:{port}"
    headers = {"Authorization": f"Bearer {password}"}
    
    print(f"Starting monitoring (interval: {interval}s)")
    print("Press Ctrl+C to stop\n")
    
    try:
        while True:
            try:
                # Get metrics
                response = requests.get(f"{base_url}/metrics", headers=headers, timeout=10)
                
                if response.status_code == 200:
                    metrics = response.json()
                    
                    # Display key metrics
                    print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                          f"CPU: {metrics['cpu']:.1f}% | "
                          f"RAM: {metrics['memory']['percentage']:.1f}% | "
                          f"Disk: {metrics['disk']['percentage']:.1f}% | "
                          f"Net: {metrics['network']['outbound_kbits_per_sec']:.1f} kbit/s")
                else:
                    print(f"Error: HTTP {response.status_code}")
                    
            except requests.RequestException as e:
                print(f"Connection error: {e}")
            
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped")

# Usage
if __name__ == "__main__":
    monitor_system("192.168.1.100", 12345, "abc12345")
```

---

## Getting Started

1. **Install Sentinel Server** on your target system
2. **Note the connection details** displayed after installation:
   - IP Address
   - Port Number
   - Password
3. **Test the connection** with a simple health check:
   ```bash
   curl http://YOUR_IP:YOUR_PORT/health
   ```
4. **Authenticate and get metrics**:
   ```bash
   curl -H "Authorization: Bearer YOUR_PASSWORD" http://YOUR_IP:YOUR_PORT/metrics
   ```

## Need Help?

- Check the [main README](../README.md) for installation instructions
- Review the [troubleshooting section](../README.md#troubleshooting)
- Examine the [example client](../scripts/client_example.py) for implementation details