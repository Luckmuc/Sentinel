# Sentinel Server Makefile
# Provides common development and testing tasks

.PHONY: help install uninstall verify test clean dev-setup lint

# Default target
help:
	@echo "Sentinel Server - Available Commands:"
	@echo ""
	@echo "Installation:"
	@echo "  install    - Install Sentinel Server (requires sudo)"
	@echo "  uninstall  - Remove Sentinel Server (requires sudo)"
	@echo "  verify     - Verify installation"
	@echo ""
	@echo "Development:"
	@echo "  dev-setup  - Set up development environment"
	@echo "  test       - Run basic tests"
	@echo "  lint       - Run code linting"
	@echo "  clean      - Clean temporary files"
	@echo ""
	@echo "Usage:"
	@echo "  sudo make install    # Install the service"
	@echo "  make verify          # Check installation"
	@echo "  sudo make uninstall  # Remove the service"

# Installation targets
install:
	@echo "Installing Sentinel Server..."
	@sudo ./scripts/install.sh

uninstall:
	@echo "Uninstalling Sentinel Server..."
	@sudo ./scripts/uninstall.sh

verify:
	@echo "Verifying Sentinel Server installation..."
	@./scripts/verify.sh

# Development targets
dev-setup:
	@echo "Setting up development environment..."
	@python3 -m pip install --user -r requirements.txt
	@python3 -m pip install --user pylint flake8
	@echo "Development environment ready!"

test:
	@echo "Running basic tests..."
	@python3 -m py_compile src/sentinel_server.py
	@python3 -c "import src.sentinel_server; print('Import test passed')"
	@echo "Basic tests completed!"

lint:
	@echo "Running code linting..."
	@python3 -m flake8 src/ --max-line-length=100 --ignore=E501,W503 || true
	@python3 -m pylint src/ --disable=C0114,C0115,C0116 || true
	@echo "Linting completed!"

clean:
	@echo "Cleaning temporary files..."
	@find . -type f -name "*.pyc" -delete
	@find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@echo "Cleanup completed!"

# Quick development server (for testing only)
dev-run:
	@echo "Starting development server (testing only)..."
	@echo "Note: This runs without systemd for development purposes"
	@python3 src/sentinel_server.py