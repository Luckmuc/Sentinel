#!/usr/bin/env python3
"""
Sentinel Server - System Monitoring and Management Service
A secure monitoring service for Ubuntu/Debian systems that provides:
- System metrics monitoring (CPU, RAM, Storage, Network, Uptime)
- Remote system management (updates, reboot)
- REST API with authentication
"""

import os
import sys
import json
import time
import socket
import random
import string
import hashlib
import logging
import psutil
import subprocess
from datetime import datetime, timedelta
from threading import Thread, Lock
from flask import Flask, request, jsonify
from werkzeug.security import check_password_hash, generate_password_hash
import signal


class SystemMonitor:
    """Core system monitoring functionality"""
    
    def __init__(self):
        self.network_stats_lock = Lock()
        self.last_network_stats = None
        self.last_network_time = None
        
    def get_cpu_usage(self):
        """Get CPU usage percentage"""
        return psutil.cpu_percent(interval=1)
    
    def get_memory_usage(self):
        """Get RAM usage in GB"""
        memory = psutil.virtual_memory()
        return {
            'total_gb': round(memory.total / (1024**3), 2),
            'used_gb': round(memory.used / (1024**3), 2),
            'available_gb': round(memory.available / (1024**3), 2),
            'percentage': memory.percent
        }
    
    def get_disk_usage(self):
        """Get storage usage in GB"""
        disk = psutil.disk_usage('/')
        return {
            'total_gb': round(disk.total / (1024**3), 2),
            'used_gb': round(disk.used / (1024**3), 2),
            'free_gb': round(disk.free / (1024**3), 2),
            'percentage': round((disk.used / disk.total) * 100, 2)
        }
    
    def get_network_usage(self):
        """Get network outbound usage in kbit/s"""
        with self.network_stats_lock:
            current_stats = psutil.net_io_counters()
            current_time = time.time()
            
            if self.last_network_stats is None:
                self.last_network_stats = current_stats
                self.last_network_time = current_time
                return {'outbound_kbits_per_sec': 0}
            
            time_delta = current_time - self.last_network_time
            bytes_sent_delta = current_stats.bytes_sent - self.last_network_stats.bytes_sent
            
            # Convert bytes to kbits per second
            kbits_per_sec = (bytes_sent_delta * 8) / (1024 * time_delta) if time_delta > 0 else 0
            
            self.last_network_stats = current_stats
            self.last_network_time = current_time
            
            return {
                'outbound_kbits_per_sec': round(kbits_per_sec, 2),
                'total_sent_gb': round(current_stats.bytes_sent / (1024**3), 2),
                'total_received_gb': round(current_stats.bytes_recv / (1024**3), 2)
            }
    
    def get_uptime(self):
        """Get system uptime"""
        boot_time = psutil.boot_time()
        uptime_seconds = time.time() - boot_time
        uptime_delta = timedelta(seconds=uptime_seconds)
        
        return {
            'uptime_seconds': int(uptime_seconds),
            'uptime_formatted': str(uptime_delta).split('.')[0],
            'boot_time': datetime.fromtimestamp(boot_time).isoformat()
        }
    
    def get_all_metrics(self):
        """Get all system metrics"""
        return {
            'timestamp': datetime.now().isoformat(),
            'cpu': self.get_cpu_usage(),
            'memory': self.get_memory_usage(),
            'disk': self.get_disk_usage(),
            'network': self.get_network_usage(),
            'uptime': self.get_uptime()
        }


class SystemManager:
    """System management operations"""
    
    @staticmethod
    def update_system():
        """Run apt update and upgrade"""
        try:
            # Run apt update
            update_result = subprocess.run(
                ['sudo', 'apt', 'update', '-y'],
                capture_output=True,
                text=True,
                timeout=300
            )
            
            # Run apt upgrade
            upgrade_result = subprocess.run(
                ['sudo', 'apt', 'upgrade', '-y'],
                capture_output=True,
                text=True,
                timeout=1800  # 30 minutes timeout
            )
            
            return {
                'success': update_result.returncode == 0 and upgrade_result.returncode == 0,
                'update_output': update_result.stdout,
                'upgrade_output': upgrade_result.stdout,
                'errors': update_result.stderr + upgrade_result.stderr
            }
        except subprocess.TimeoutExpired:
            return {
                'success': False,
                'error': 'Update operation timed out'
            }
        except Exception as e:
            return {
                'success': False,
                'error': str(e)
            }
    
    @staticmethod
    def reboot_system():
        """Reboot the system"""
        try:
            subprocess.Popen(['sudo', 'reboot'])
            return {'success': True, 'message': 'Reboot initiated'}
        except Exception as e:
            return {'success': False, 'error': str(e)}


class ConfigManager:
    """Configuration management"""
    
    def __init__(self, config_file='/etc/sentinel-server/config.json'):
        self.config_file = config_file
        self.config = self.load_config()
    
    def load_config(self):
        """Load configuration from file"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r') as f:
                    return json.load(f)
        except Exception as e:
            logging.error(f"Error loading config: {e}")
        
        # Return default config if file doesn't exist or error occurred
        return self.generate_default_config()
    
    def save_config(self):
        """Save configuration to file"""
        try:
            os.makedirs(os.path.dirname(self.config_file), exist_ok=True)
            with open(self.config_file, 'w') as f:
                json.dump(self.config, f, indent=4)
            # Secure the config file
            os.chmod(self.config_file, 0o600)
            return True
        except Exception as e:
            logging.error(f"Error saving config: {e}")
            return False
    
    def generate_default_config(self):
        """Generate default configuration"""
        return {
            'port': self.find_available_port(),
            'password_hash': self.generate_password(),
            'created_at': datetime.now().isoformat()
        }
    
    def find_available_port(self):
        """Find an available port that's not commonly used by system services"""
        # Avoid system ports (1-1023) and commonly used ports
        forbidden_ranges = [
            (1, 1023),      # System ports
            (3000, 3010),   # Development servers
            (5000, 5010),   # Flask default
            (8000, 8010),   # HTTP alternatives
            (8080, 8090),   # HTTP proxies
            (9000, 9010),   # Various services
        ]
        
        for _ in range(100):  # Try up to 100 times
            port = random.randint(10000, 65535)
            
            # Check if port is in forbidden range
            if any(start <= port <= end for start, end in forbidden_ranges):
                continue
            
            # Check if port is available
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.bind(('localhost', port))
                    return port
            except OSError:
                continue
        
        # Fallback to random high port
        return random.randint(50000, 60000)
    
    def generate_password(self):
        """Generate a random 8-character password and return its hash"""
        password = ''.join(random.choices(string.ascii_letters + string.digits, k=8))
        password_hash = generate_password_hash(password)
        
        # Store the plain password temporarily for display during installation
        self.plain_password = password
        
        return password_hash
    
    def get_server_ip(self):
        """Get the server's IP address"""
        try:
            # Connect to a remote address to determine local IP
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.connect(("8.8.8.8", 80))
                return s.getsockname()[0]
        except Exception:
            return "127.0.0.1"


class SentinelServer:
    """Main Sentinel Server application"""
    
    def __init__(self):
        self.config_manager = ConfigManager()
        self.monitor = SystemMonitor()
        self.manager = SystemManager()
        self.app = Flask(__name__)
        self.setup_logging()
        self.setup_routes()
        
    def setup_logging(self):
        """Setup logging configuration"""
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler('/var/log/sentinel-server.log'),
                logging.StreamHandler()
            ]
        )
    
    def authenticate(self, password):
        """Authenticate request with password"""
        return check_password_hash(
            self.config_manager.config['password_hash'], 
            password
        )
    
    def require_auth(self, f):
        """Decorator to require authentication"""
        def decorated(*args, **kwargs):
            auth_header = request.headers.get('Authorization')
            if not auth_header or not auth_header.startswith('Bearer '):
                return jsonify({'error': 'Authentication required'}), 401
            
            password = auth_header.split(' ')[1]
            if not self.authenticate(password):
                return jsonify({'error': 'Invalid authentication'}), 401
            
            return f(*args, **kwargs)
        decorated.__name__ = f.__name__
        return decorated
    
    def setup_routes(self):
        """Setup Flask routes"""
        
        @self.app.route('/health', methods=['GET'])
        def health_check():
            return jsonify({
                'status': 'healthy',
                'service': 'sentinel-server',
                'timestamp': datetime.now().isoformat()
            })
        
        @self.app.route('/metrics', methods=['GET'])
        @self.require_auth
        def get_metrics():
            return jsonify(self.monitor.get_all_metrics())
        
        @self.app.route('/update', methods=['POST'])
        @self.require_auth
        def update_system():
            result = self.manager.update_system()
            status_code = 200 if result['success'] else 500
            return jsonify(result), status_code
        
        @self.app.route('/reboot', methods=['POST'])
        @self.require_auth
        def reboot_system():
            result = self.manager.reboot_system()
            status_code = 200 if result['success'] else 500
            return jsonify(result), status_code
        
        @self.app.route('/info', methods=['GET'])
        def get_info():
            return jsonify({
                'service': 'Sentinel Server',
                'version': '1.0.0',
                'endpoints': ['/health', '/metrics', '/update', '/reboot', '/info'],
                'authentication': 'Bearer token required for protected endpoints'
            })
    
    def run(self):
        """Run the Sentinel Server"""
        port = self.config_manager.config['port']
        logging.info(f"Starting Sentinel Server on port {port}")
        
        # Save config on startup
        self.config_manager.save_config()
        
        # Display connection info if this is the first run
        if hasattr(self.config_manager, 'plain_password'):
            ip = self.config_manager.get_server_ip()
            print(f"\n{'='*50}")
            print("SENTINEL SERVER STARTED")
            print(f"{'='*50}")
            print(f"IP Address: {ip}")
            print(f"Port: {port}")
            print(f"Password: {self.config_manager.plain_password}")
            print(f"{'='*50}\n")
            
            # Remove plain password from memory after display
            delattr(self.config_manager, 'plain_password')
        
        self.app.run(host='0.0.0.0', port=port, debug=False)


def signal_handler(signum, frame):
    """Handle shutdown signals"""
    logging.info("Received shutdown signal, stopping Sentinel Server...")
    sys.exit(0)


if __name__ == "__main__":
    # Set up signal handlers for graceful shutdown
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    try:
        server = SentinelServer()
        server.run()
    except Exception as e:
        logging.error(f"Failed to start Sentinel Server: {e}")
        sys.exit(1)