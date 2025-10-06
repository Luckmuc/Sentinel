# Usage Examples

This document provides practical examples of how to use Sentinel Server in various scenarios and with different programming languages.

## Table of Contents

- [Quick Start Examples](#quick-start-examples)
- [Authentication Examples](#authentication-examples)
- [Monitoring Examples](#monitoring-examples)
- [System Management Examples](#system-management-examples)
- [Programming Language Examples](#programming-language-examples)
- [Advanced Use Cases](#advanced-use-cases)
- [Integration Examples](#integration-examples)

---

## Quick Start Examples

### Basic Health Check

Test if your Sentinel Server is responding:

```bash
# Simple health check
curl http://192.168.1.100:12345/health

# Pretty formatted output
curl -s http://192.168.1.100:12345/health | jq '.'
```

Expected output:
```json
{
  "status": "healthy",
  "service": "sentinel-server",
  "timestamp": "2025-10-05T12:00:00.123456"
}
```

### Get Server Information

```bash
# Get available endpoints and service info
curl -s http://192.168.1.100:12345/info | jq '.'
```

### First Metrics Request

```bash
# Get system metrics (replace with your password)
curl -H "Authorization: Bearer abc12345" \
     -s http://192.168.1.100:12345/metrics | jq '.'
```

---

## Authentication Examples

### Using cURL

```bash
# Method 1: Header format
curl -H "Authorization: Bearer your_password" \
     http://your_server:port/metrics

# Method 2: With verbose output for debugging
curl -v -H "Authorization: Bearer your_password" \
     http://your_server:port/metrics

# Method 3: Store token in variable
TOKEN="your_password"
curl -H "Authorization: Bearer $TOKEN" \
     http://your_server:port/metrics
```

### Testing Authentication

```bash
# Test with wrong password (should return 401)
curl -H "Authorization: Bearer wrongpassword" \
     http://your_server:port/metrics

# Test without authentication (should return 401)
curl http://your_server:port/metrics

# Test with correct password (should return 200)
curl -H "Authorization: Bearer correct_password" \
     http://your_server:port/metrics
```

---

## Monitoring Examples

### Continuous System Monitoring

#### Basic Monitoring Loop

```bash
#!/bin/bash
# Simple monitoring script

SERVER="192.168.1.100:12345"
TOKEN="abc12345"

while true; do
    echo "=== $(date) ==="
    
    # Get metrics
    METRICS=$(curl -s -H "Authorization: Bearer $TOKEN" \
                  "http://$SERVER/metrics")
    
    # Extract key values
    CPU=$(echo "$METRICS" | jq -r '.cpu')
    MEM=$(echo "$METRICS" | jq -r '.memory.percentage')
    DISK=$(echo "$METRICS" | jq -r '.disk.percentage')
    
    echo "CPU: ${CPU}%, Memory: ${MEM}%, Disk: ${DISK}%"
    
    sleep 30
done
```

#### Advanced Monitoring with Alerts

```bash
#!/bin/bash
# Monitoring with threshold alerts

SERVER="192.168.1.100:12345"
TOKEN="abc12345"

# Thresholds
CPU_THRESHOLD=80
MEM_THRESHOLD=85
DISK_THRESHOLD=90

check_thresholds() {
    local cpu=$1
    local mem=$2
    local disk=$3
    
    if (( $(echo "$cpu > $CPU_THRESHOLD" | bc -l) )); then
        echo "ALERT: High CPU usage: ${cpu}%"
        # Send alert (email, webhook, etc.)
    fi
    
    if (( $(echo "$mem > $MEM_THRESHOLD" | bc -l) )); then
        echo "ALERT: High memory usage: ${mem}%"
    fi
    
    if (( $(echo "$disk > $DISK_THRESHOLD" | bc -l) )); then
        echo "ALERT: High disk usage: ${disk}%"
    fi
}

while true; do
    METRICS=$(curl -s -H "Authorization: Bearer $TOKEN" \
                  "http://$SERVER/metrics")
    
    if [ $? -eq 0 ] && [ -n "$METRICS" ]; then
        CPU=$(echo "$METRICS" | jq -r '.cpu')
        MEM=$(echo "$METRICS" | jq -r '.memory.percentage')
        DISK=$(echo "$METRICS" | jq -r '.disk.percentage')
        
        echo "[$(date)] CPU: ${CPU}%, MEM: ${MEM}%, DISK: ${DISK}%"
        check_thresholds "$CPU" "$MEM" "$DISK"
    else
        echo "ERROR: Failed to get metrics"
    fi
    
    sleep 60
done
```

### Data Logging

#### CSV Logging

```bash
#!/bin/bash
# Log metrics to CSV file

SERVER="192.168.1.100:12345"
TOKEN="abc12345"
LOGFILE="metrics_$(date +%Y%m%d).csv"

# Create header if file doesn't exist
if [ ! -f "$LOGFILE" ]; then
    echo "timestamp,cpu,memory_total,memory_used,memory_percentage,disk_total,disk_used,disk_percentage,network_outbound" > "$LOGFILE"
fi

while true; do
    METRICS=$(curl -s -H "Authorization: Bearer $TOKEN" \
                  "http://$SERVER/metrics")
    
    if [ $? -eq 0 ] && [ -n "$METRICS" ]; then
        TIMESTAMP=$(echo "$METRICS" | jq -r '.timestamp')
        CPU=$(echo "$METRICS" | jq -r '.cpu')
        MEM_TOTAL=$(echo "$METRICS" | jq -r '.memory.total_gb')
        MEM_USED=$(echo "$METRICS" | jq -r '.memory.used_gb')
        MEM_PCT=$(echo "$METRICS" | jq -r '.memory.percentage')
        DISK_TOTAL=$(echo "$METRICS" | jq -r '.disk.total_gb')
        DISK_USED=$(echo "$METRICS" | jq -r '.disk.used_gb')
        DISK_PCT=$(echo "$METRICS" | jq -r '.disk.percentage')
        NET_OUT=$(echo "$METRICS" | jq -r '.network.outbound_kbits_per_sec')
        
        echo "$TIMESTAMP,$CPU,$MEM_TOTAL,$MEM_USED,$MEM_PCT,$DISK_TOTAL,$DISK_USED,$DISK_PCT,$NET_OUT" >> "$LOGFILE"
        echo "Logged: $TIMESTAMP"
    fi
    
    sleep 300  # Log every 5 minutes
done
```

---

## System Management Examples

### System Updates

#### Manual Update

```bash
# Trigger system update
curl -X POST \
     -H "Authorization: Bearer abc12345" \
     http://192.168.1.100:12345/update

# Check update result with formatted output
curl -X POST \
     -H "Authorization: Bearer abc12345" \
     -s http://192.168.1.100:12345/update | jq '.'
```

#### Automated Weekly Updates

```bash
#!/bin/bash
# Weekly update script for crontab
# Add to crontab: 0 2 * * 0 /path/to/this/script

SERVER="192.168.1.100:12345"
TOKEN="abc12345"
LOGFILE="/var/log/sentinel-auto-update.log"

echo "=== Automated Update Started: $(date) ===" >> "$LOGFILE"

RESULT=$(curl -X POST \
              -H "Authorization: Bearer $TOKEN" \
              -s "http://$SERVER/update")

SUCCESS=$(echo "$RESULT" | jq -r '.success')

if [ "$SUCCESS" = "true" ]; then
    echo "Update completed successfully" >> "$LOGFILE"
    echo "$RESULT" | jq -r '.upgrade_output' >> "$LOGFILE"
else
    echo "Update failed!" >> "$LOGFILE"
    echo "$RESULT" | jq -r '.error' >> "$LOGFILE"
fi

echo "=== Automated Update Finished: $(date) ===" >> "$LOGFILE"
```

### System Reboot

#### Scheduled Reboot

```bash
#!/bin/bash
# Schedule a reboot with confirmation

SERVER="192.168.1.100:12345"
TOKEN="abc12345"

echo "This will reboot the server immediately!"
read -p "Are you sure? (yes/no): " confirm

if [ "$confirm" = "yes" ]; then
    echo "Initiating reboot..."
    
    RESULT=$(curl -X POST \
                  -H "Authorization: Bearer $TOKEN" \
                  -s "http://$SERVER/reboot")
    
    SUCCESS=$(echo "$RESULT" | jq -r '.success')
    
    if [ "$SUCCESS" = "true" ]; then
        echo "Reboot initiated successfully"
        echo "Server will be unavailable during reboot"
    else
        echo "Reboot failed:"
        echo "$RESULT" | jq -r '.error'
    fi
else
    echo "Reboot cancelled"
fi
```

---

## Programming Language Examples

### Python Examples

#### Simple Client Class

```python
#!/usr/bin/env python3
import requests
import json
from datetime import datetime

class SentinelClient:
    def __init__(self, host, port, password):
        self.base_url = f"http://{host}:{port}"
        self.headers = {"Authorization": f"Bearer {password}"}
        self.timeout = 30
    
    def health_check(self):
        """Check server health"""
        try:
            response = requests.get(f"{self.base_url}/health", timeout=10)
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Health check failed: {e}")
            return None
    
    def get_metrics(self):
        """Get system metrics"""
        try:
            response = requests.get(
                f"{self.base_url}/metrics", 
                headers=self.headers, 
                timeout=self.timeout
            )
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Failed to get metrics: {e}")
            return None
    
    def update_system(self):
        """Update the system"""
        try:
            response = requests.post(
                f"{self.base_url}/update", 
                headers=self.headers, 
                timeout=1800  # 30 minutes
            )
            return response.json()
        except requests.RequestException as e:
            print(f"Update failed: {e}")
            return {"success": False, "error": str(e)}
    
    def reboot_system(self):
        """Reboot the system"""
        try:
            response = requests.post(
                f"{self.base_url}/reboot", 
                headers=self.headers, 
                timeout=10
            )
            return response.json()
        except requests.RequestException as e:
            print(f"Reboot failed: {e}")
            return {"success": False, "error": str(e)}

# Usage example
if __name__ == "__main__":
    client = SentinelClient("192.168.1.100", 12345, "abc12345")
    
    # Health check
    health = client.health_check()
    print(f"Server health: {health['status'] if health else 'Unknown'}")
    
    # Get metrics
    metrics = client.get_metrics()
    if metrics:
        print(f"CPU: {metrics['cpu']}%")
        print(f"Memory: {metrics['memory']['percentage']}%")
        print(f"Disk: {metrics['disk']['percentage']}%")
```

#### Advanced Monitoring with Data Collection

```python
#!/usr/bin/env python3
import requests
import time
import csv
import sqlite3
from datetime import datetime
import threading
import queue

class SentinelDataCollector:
    def __init__(self, host, port, password, db_file="metrics.db"):
        self.client = SentinelClient(host, port, password)
        self.db_file = db_file
        self.running = False
        self.data_queue = queue.Queue()
        self.setup_database()
    
    def setup_database(self):
        """Set up SQLite database for metrics storage"""
        conn = sqlite3.connect(self.db_file)
        cursor = conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS metrics (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp TEXT,
                cpu REAL,
                memory_total REAL,
                memory_used REAL,
                memory_percentage REAL,
                disk_total REAL,
                disk_used REAL,
                disk_percentage REAL,
                network_outbound REAL,
                uptime_seconds INTEGER
            )
        ''')
        
        conn.commit()
        conn.close()
    
    def collect_metrics(self, interval=60):
        """Collect metrics at specified interval"""
        self.running = True
        
        while self.running:
            try:
                metrics = self.client.get_metrics()
                if metrics:
                    self.data_queue.put(metrics)
                
                time.sleep(interval)
            except Exception as e:
                print(f"Error collecting metrics: {e}")
                time.sleep(interval)
    
    def store_metrics(self):
        """Store metrics from queue to database"""
        conn = sqlite3.connect(self.db_file)
        cursor = conn.cursor()
        
        while self.running or not self.data_queue.empty():
            try:
                metrics = self.data_queue.get(timeout=1)
                
                cursor.execute('''
                    INSERT INTO metrics (
                        timestamp, cpu, memory_total, memory_used, memory_percentage,
                        disk_total, disk_used, disk_percentage, network_outbound, uptime_seconds
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                ''', (
                    metrics['timestamp'],
                    metrics['cpu'],
                    metrics['memory']['total_gb'],
                    metrics['memory']['used_gb'],
                    metrics['memory']['percentage'],
                    metrics['disk']['total_gb'],
                    metrics['disk']['used_gb'],
                    metrics['disk']['percentage'],
                    metrics['network']['outbound_kbits_per_sec'],
                    metrics['uptime']['uptime_seconds']
                ))
                
                conn.commit()
                print(f"Stored metrics for {metrics['timestamp']}")
                
            except queue.Empty:
                continue
            except Exception as e:
                print(f"Error storing metrics: {e}")
        
        conn.close()
    
    def start_monitoring(self, interval=60):
        """Start monitoring in background threads"""
        collector_thread = threading.Thread(
            target=self.collect_metrics, 
            args=(interval,)
        )
        storage_thread = threading.Thread(target=self.store_metrics)
        
        collector_thread.start()
        storage_thread.start()
        
        return collector_thread, storage_thread
    
    def stop_monitoring(self):
        """Stop monitoring"""
        self.running = False

# Usage
collector = SentinelDataCollector("192.168.1.100", 12345, "abc12345")
threads = collector.start_monitoring(interval=30)

try:
    # Keep running until interrupted
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("Stopping monitoring...")
    collector.stop_monitoring()
    
    # Wait for threads to finish
    for thread in threads:
        thread.join()
```

### Node.js Example

```javascript
// sentinel-client.js
const axios = require('axios');

class SentinelClient {
    constructor(host, port, password) {
        this.baseURL = `http://${host}:${port}`;
        this.headers = {
            'Authorization': `Bearer ${password}`,
            'Content-Type': 'application/json'
        };
        this.timeout = 30000; // 30 seconds
    }

    async healthCheck() {
        try {
            const response = await axios.get(`${this.baseURL}/health`, {
                timeout: 10000
            });
            return response.data;
        } catch (error) {
            console.error('Health check failed:', error.message);
            return null;
        }
    }

    async getMetrics() {
        try {
            const response = await axios.get(`${this.baseURL}/metrics`, {
                headers: this.headers,
                timeout: this.timeout
            });
            return response.data;
        } catch (error) {
            console.error('Failed to get metrics:', error.message);
            return null;
        }
    }

    async updateSystem() {
        try {
            const response = await axios.post(`${this.baseURL}/update`, {}, {
                headers: this.headers,
                timeout: 1800000 // 30 minutes
            });
            return response.data;
        } catch (error) {
            console.error('Update failed:', error.message);
            return { success: false, error: error.message };
        }
    }

    async rebootSystem() {
        try {
            const response = await axios.post(`${this.baseURL}/reboot`, {}, {
                headers: this.headers,
                timeout: 10000
            });
            return response.data;
        } catch (error) {
            console.error('Reboot failed:', error.message);
            return { success: false, error: error.message };
        }
    }
}

// Usage example
async function main() {
    const client = new SentinelClient('192.168.1.100', 12345, 'abc12345');
    
    // Health check
    const health = await client.healthCheck();
    console.log('Server health:', health ? health.status : 'Unknown');
    
    // Get metrics
    const metrics = await client.getMetrics();
    if (metrics) {
        console.log(`CPU: ${metrics.cpu}%`);
        console.log(`Memory: ${metrics.memory.percentage}%`);
        console.log(`Disk: ${metrics.disk.percentage}%`);
    }
}

// Export for use as module
module.exports = SentinelClient;

// Run if called directly
if (require.main === module) {
    main();
}
```

### PowerShell Example

```powershell
# SentinelClient.ps1
class SentinelClient {
    [string]$BaseUrl
    [hashtable]$Headers
    [int]$Timeout

    SentinelClient([string]$Host, [int]$Port, [string]$Password) {
        $this.BaseUrl = "http://$Host`:$Port"
        $this.Headers = @{
            "Authorization" = "Bearer $Password"
            "Content-Type" = "application/json"
        }
        $this.Timeout = 30
    }

    [PSCustomObject] HealthCheck() {
        try {
            $Response = Invoke-RestMethod -Uri "$($this.BaseUrl)/health" -Method Get -TimeoutSec 10
            return $Response
        }
        catch {
            Write-Error "Health check failed: $($_.Exception.Message)"
            return $null
        }
    }

    [PSCustomObject] GetMetrics() {
        try {
            $Response = Invoke-RestMethod -Uri "$($this.BaseUrl)/metrics" -Headers $this.Headers -Method Get -TimeoutSec $this.Timeout
            return $Response
        }
        catch {
            Write-Error "Failed to get metrics: $($_.Exception.Message)"
            return $null
        }
    }

    [PSCustomObject] UpdateSystem() {
        try {
            $Response = Invoke-RestMethod -Uri "$($this.BaseUrl)/update" -Headers $this.Headers -Method Post -TimeoutSec 1800
            return $Response
        }
        catch {
            Write-Error "Update failed: $($_.Exception.Message)"
            return @{ success = $false; error = $_.Exception.Message }
        }
    }

    [PSCustomObject] RebootSystem() {
        try {
            $Response = Invoke-RestMethod -Uri "$($this.BaseUrl)/reboot" -Headers $this.Headers -Method Post -TimeoutSec 10
            return $Response
        }
        catch {
            Write-Error "Reboot failed: $($_.Exception.Message)"
            return @{ success = $false; error = $_.Exception.Message }
        }
    }
}

# Usage example
$Client = [SentinelClient]::new("192.168.1.100", 12345, "abc12345")

# Health check
$Health = $Client.HealthCheck()
Write-Output "Server health: $($Health.status)"

# Get metrics
$Metrics = $Client.GetMetrics()
if ($Metrics) {
    Write-Output "CPU: $($Metrics.cpu)%"
    Write-Output "Memory: $($Metrics.memory.percentage)%"
    Write-Output "Disk: $($Metrics.disk.percentage)%"
}
```

---

## Advanced Use Cases

### Load Balancer Health Checks

If you're using Sentinel Server behind a load balancer:

```bash
# HAProxy health check configuration
# Add to haproxy.cfg:
# option httpchk GET /health
# server sentinel1 192.168.1.100:12345 check
```

### Monitoring Multiple Servers

```python
#!/usr/bin/env python3
import asyncio
import aiohttp
import json
from datetime import datetime

class MultiServerMonitor:
    def __init__(self):
        self.servers = []
    
    def add_server(self, name, host, port, password):
        """Add a server to monitor"""
        self.servers.append({
            'name': name,
            'host': host,
            'port': port,
            'password': password,
            'url': f"http://{host}:{port}"
        })
    
    async def check_server(self, session, server):
        """Check a single server"""
        headers = {"Authorization": f"Bearer {server['password']}"}
        
        try:
            # Health check
            async with session.get(f"{server['url']}/health", timeout=10) as response:
                health = await response.json() if response.status == 200 else None
            
            # Metrics
            async with session.get(f"{server['url']}/metrics", headers=headers, timeout=10) as response:
                metrics = await response.json() if response.status == 200 else None
            
            return {
                'name': server['name'],
                'host': server['host'],
                'port': server['port'],
                'health': health,
                'metrics': metrics,
                'status': 'online' if health and metrics else 'offline'
            }
        
        except Exception as e:
            return {
                'name': server['name'],
                'host': server['host'],
                'port': server['port'],
                'health': None,
                'metrics': None,
                'status': 'error',
                'error': str(e)
            }
    
    async def check_all_servers(self):
        """Check all servers concurrently"""
        async with aiohttp.ClientSession() as session:
            tasks = [self.check_server(session, server) for server in self.servers]
            results = await asyncio.gather(*tasks)
            return results
    
    def print_status(self, results):
        """Print status of all servers"""
        print(f"\n=== Server Status - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")
        
        for result in results:
            status_icon = "✓" if result['status'] == 'online' else "✗"
            print(f"{status_icon} {result['name']} ({result['host']}:{result['port']}) - {result['status'].upper()}")
            
            if result['metrics']:
                metrics = result['metrics']
                print(f"    CPU: {metrics['cpu']:.1f}% | "
                      f"MEM: {metrics['memory']['percentage']:.1f}% | "
                      f"DISK: {metrics['disk']['percentage']:.1f}%")
            elif result.get('error'):
                print(f"    Error: {result['error']}")
            print()

# Usage
async def main():
    monitor = MultiServerMonitor()
    
    # Add servers
    monitor.add_server("Web Server", "192.168.1.100", 12345, "password1")
    monitor.add_server("Database Server", "192.168.1.101", 12346, "password2")
    monitor.add_server("Cache Server", "192.168.1.102", 12347, "password3")
    
    # Monitor continuously
    while True:
        try:
            results = await monitor.check_all_servers()
            monitor.print_status(results)
            await asyncio.sleep(60)  # Check every minute
        except KeyboardInterrupt:
            print("Monitoring stopped")
            break

if __name__ == "__main__":
    asyncio.run(main())
```

### Integration with Grafana

```python
# prometheus_exporter.py
# Export Sentinel Server metrics in Prometheus format for Grafana

from prometheus_client import start_http_server, Gauge
import requests
import time
import logging

class SentinelPrometheusExporter:
    def __init__(self, sentinel_host, sentinel_port, sentinel_password, exporter_port=8000):
        self.sentinel_url = f"http://{sentinel_host}:{sentinel_port}"
        self.headers = {"Authorization": f"Bearer {sentinel_password}"}
        
        # Define Prometheus metrics
        self.cpu_usage = Gauge('sentinel_cpu_usage_percent', 'CPU usage percentage')
        self.memory_usage = Gauge('sentinel_memory_usage_percent', 'Memory usage percentage')
        self.memory_total = Gauge('sentinel_memory_total_gb', 'Total memory in GB')
        self.memory_used = Gauge('sentinel_memory_used_gb', 'Used memory in GB')
        self.disk_usage = Gauge('sentinel_disk_usage_percent', 'Disk usage percentage')
        self.disk_total = Gauge('sentinel_disk_total_gb', 'Total disk space in GB')
        self.disk_used = Gauge('sentinel_disk_used_gb', 'Used disk space in GB')
        self.network_outbound = Gauge('sentinel_network_outbound_kbps', 'Outbound network speed in kbps')
        self.uptime_seconds = Gauge('sentinel_uptime_seconds', 'System uptime in seconds')
        
        # Start Prometheus HTTP server
        start_http_server(exporter_port)
        logging.info(f"Prometheus exporter started on port {exporter_port}")
    
    def collect_metrics(self):
        """Collect metrics from Sentinel Server and update Prometheus metrics"""
        try:
            response = requests.get(f"{self.sentinel_url}/metrics", headers=self.headers, timeout=30)
            
            if response.status_code == 200:
                metrics = response.json()
                
                # Update Prometheus metrics
                self.cpu_usage.set(metrics['cpu'])
                self.memory_usage.set(metrics['memory']['percentage'])
                self.memory_total.set(metrics['memory']['total_gb'])
                self.memory_used.set(metrics['memory']['used_gb'])
                self.disk_usage.set(metrics['disk']['percentage'])
                self.disk_total.set(metrics['disk']['total_gb'])
                self.disk_used.set(metrics['disk']['used_gb'])
                self.network_outbound.set(metrics['network']['outbound_kbits_per_sec'])
                self.uptime_seconds.set(metrics['uptime']['uptime_seconds'])
                
                logging.info("Metrics updated successfully")
            else:
                logging.error(f"Failed to get metrics: HTTP {response.status_code}")
                
        except Exception as e:
            logging.error(f"Error collecting metrics: {e}")
    
    def run(self, interval=30):
        """Run the exporter continuously"""
        logging.info(f"Starting metric collection (interval: {interval}s)")
        
        while True:
            self.collect_metrics()
            time.sleep(interval)

# Usage
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    exporter = SentinelPrometheusExporter(
        sentinel_host="192.168.1.100",
        sentinel_port=12345,
        sentinel_password="abc12345",
        exporter_port=8000
    )
    
    try:
        exporter.run(interval=15)  # Collect every 15 seconds
    except KeyboardInterrupt:
        logging.info("Exporter stopped")
```

These examples demonstrate various ways to integrate Sentinel Server into different monitoring and management workflows. Choose the examples that best fit your use case and customize them as needed.