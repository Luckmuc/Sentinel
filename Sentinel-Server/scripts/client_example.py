#!/usr/bin/env python3
"""
Sentinel Server Client Example
A simple client to demonstrate how to interact with the Sentinel Server API
"""

import requests
import json
import time
import argparse
from urllib.parse import urljoin


class SentinelClient:
    """Client for Sentinel Server API"""
    
    def __init__(self, host, port, password):
        self.base_url = f"http://{host}:{port}"
        self.headers = {
            'Authorization': f'Bearer {password}',
            'Content-Type': 'application/json'
        }
    
    def health_check(self):
        """Check server health"""
        try:
            response = requests.get(urljoin(self.base_url, '/health'), timeout=10)
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Health check failed: {e}")
            return None
    
    def get_metrics(self):
        """Get system metrics"""
        try:
            response = requests.get(
                urljoin(self.base_url, '/metrics'), 
                headers=self.headers, 
                timeout=10
            )
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Failed to get metrics: {e}")
            return None
    
    def update_system(self):
        """Update the system"""
        try:
            response = requests.post(
                urljoin(self.base_url, '/update'), 
                headers=self.headers, 
                timeout=1800  # 30 minutes timeout
            )
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Failed to update system: {e}")
            return None
    
    def reboot_system(self):
        """Reboot the system"""
        try:
            response = requests.post(
                urljoin(self.base_url, '/reboot'), 
                headers=self.headers, 
                timeout=10
            )
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Failed to reboot system: {e}")
            return None
    
    def get_info(self):
        """Get server information"""
        try:
            response = requests.get(urljoin(self.base_url, '/info'), timeout=10)
            return response.json() if response.status_code == 200 else None
        except requests.RequestException as e:
            print(f"Failed to get info: {e}")
            return None


def print_metrics(metrics):
    """Pretty print system metrics"""
    if not metrics:
        print("No metrics available")
        return
    
    print(f"\n{'='*50}")
    print(f"SYSTEM METRICS - {metrics['timestamp']}")
    print(f"{'='*50}")
    
    # CPU
    print(f"CPU Usage: {metrics['cpu']}%")
    
    # Memory
    mem = metrics['memory']
    print(f"Memory: {mem['used_gb']:.1f}GB / {mem['total_gb']:.1f}GB ({mem['percentage']:.1f}%)")
    
    # Disk
    disk = metrics['disk']
    print(f"Disk: {disk['used_gb']:.1f}GB / {disk['total_gb']:.1f}GB ({disk['percentage']:.1f}%)")
    
    # Network
    net = metrics['network']
    print(f"Network Out: {net['outbound_kbits_per_sec']:.2f} kbit/s")
    print(f"Total Sent: {net['total_sent_gb']:.2f}GB")
    print(f"Total Received: {net['total_received_gb']:.2f}GB")
    
    # Uptime
    uptime = metrics['uptime']
    print(f"Uptime: {uptime['uptime_formatted']}")
    print(f"Boot Time: {uptime['boot_time']}")
    print(f"{'='*50}\n")


def monitor_loop(client, interval=30):
    """Continuous monitoring loop"""
    print(f"Starting monitoring loop (interval: {interval}s)")
    print("Press Ctrl+C to stop")
    
    try:
        while True:
            metrics = client.get_metrics()
            if metrics:
                print_metrics(metrics)
            else:
                print("Failed to retrieve metrics")
            
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\nMonitoring stopped")


def main():
    parser = argparse.ArgumentParser(description='Sentinel Server Client')
    parser.add_argument('host', help='Server IP address')
    parser.add_argument('port', type=int, help='Server port')
    parser.add_argument('password', help='Authentication password')
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Health command
    subparsers.add_parser('health', help='Check server health')
    
    # Metrics command
    subparsers.add_parser('metrics', help='Get system metrics')
    
    # Monitor command
    monitor_parser = subparsers.add_parser('monitor', help='Start monitoring loop')
    monitor_parser.add_argument('--interval', type=int, default=30, 
                               help='Monitoring interval in seconds (default: 30)')
    
    # Update command
    subparsers.add_parser('update', help='Update the system')
    
    # Reboot command
    subparsers.add_parser('reboot', help='Reboot the system')
    
    # Info command
    subparsers.add_parser('info', help='Get server information')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    # Create client
    client = SentinelClient(args.host, args.port, args.password)
    
    # Execute command
    if args.command == 'health':
        result = client.health_check()
        if result:
            print(f"Server Status: {result['status']}")
            print(f"Timestamp: {result['timestamp']}")
        else:
            print("Health check failed")
    
    elif args.command == 'metrics':
        metrics = client.get_metrics()
        print_metrics(metrics)
    
    elif args.command == 'monitor':
        monitor_loop(client, args.interval)
    
    elif args.command == 'update':
        print("Starting system update...")
        result = client.update_system()
        if result and result.get('success'):
            print("System update completed successfully")
            if result.get('upgrade_output'):
                print("Upgrade output:")
                print(result['upgrade_output'])
        else:
            print("System update failed")
            if result and result.get('error'):
                print(f"Error: {result['error']}")
    
    elif args.command == 'reboot':
        print("Initiating system reboot...")
        result = client.reboot_system()
        if result and result.get('success'):
            print("Reboot initiated successfully")
        else:
            print("Reboot failed")
            if result and result.get('error'):
                print(f"Error: {result['error']}")
    
    elif args.command == 'info':
        result = client.get_info()
        if result:
            print(json.dumps(result, indent=2))
        else:
            print("Failed to get server information")


if __name__ == "__main__":
    main()