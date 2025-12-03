# Deprecated Files

This directory contains files that are no longer actively used in the VIA software project.

## Docker (`docker/`)

**Status:** Deprecated as of November 2025

**Reason:** Switched to dedicated Pi 400 testing machine for better hardware integration and simpler deployment.

**Contents:**
- `Dockerfile` - Docker image for VIA software
- `docker-build.sh` - Build script for Docker container
- `.dockerignore` - Docker ignore file
- `DOCKER_QUICKSTART.md` - Docker setup guide

**If you need Docker:** These files are preserved for reference but are no longer maintained. The Pi 400 setup provides better hardware access and simpler workflow.

## Windows Scripts (`windows/`)

**Status:** Deprecated as of November 2025

**Reason:** Team standardized on Linux/macOS development with Pi 400 for testing.

**Contents:**
- `VIA.bat` - Windows batch script for VIA operations
- `QUICKSTART.md` - Windows setup guide

**If you need Windows support:** Use WSL2 or the Pi 400 testing machine instead.

---

## Current Workflow

Instead of Docker or Windows batch files, use:

1. **Development:** Any platform (Linux/macOS/Windows with WSL2)
2. **Testing:** Pi 400 testing machine at 192.168.4.163
   ```bash
   ssh pi@192.168.4.163
   cd ~/Aeris/aeris-via-software
   git pull
   via-test
   via-console
   ```

See main [README.md](../README.md) for current setup instructions.
