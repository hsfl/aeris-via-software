# VIA Spectrometer Quick Start

Complete workflow from unit tests to flatsat integration.

---

## Local Development & Testing

### 1. Run Unit Tests

Test without hardware:

```bash
cd tests
./run_all_tests.sh
```

Expected: All tests pass (~15-40 seconds)

**What it tests:**
- Data generation
- Python unit tests (10+ tests)
- Firmware build (if PlatformIO installed)
- Visualization

### 2. Interactive Simulation (Optional)

Test console commands without hardware:

**Terminal 1:**
```bash
cd tests
python3 virtual_serial_port.py
```

**Terminal 2:**
```bash
./VIA.sh /tmp/ttyVIA0
```

Try commands: `help`, `measure`, `status`

---

## Hardware-in-Loop (HIL) Testing

### 3. Connect Hardware

1. Plug Teensy 4.1 into computer via USB
2. Connect AvaSpec to Teensy's USB host port
3. Verify connection: `ls /dev/ttyACM*`

### 4. Upload Firmware

```bash
cd AvaSpecDriver
pio run --target upload
```

### 5. Test with Real Hardware

```bash
./VIA.sh
```

In console:
```
VIA> measure
VIA> status
```

Data saves to session folder displayed at startup.

---

## Flatsat Integration

### 6. Connect to Testing Pi

```bash
ssh pi@192.168.4.163
# Password: aeris
```

### 7. Update and Test

```bash
via              # Go to VIA directory
git pull         # Get updates
via-test         # Run tests
```

### 8. Connect Flatsat Hardware

1. Plug Teensy 4.1 into Pi via USB
2. Connect AvaSpec to Teensy's USB host port
3. Verify: `ports` should show `/dev/ttyACM0`

### 9. Launch Console

```bash
via-console
```

### 10. Take Measurements

```
VIA> measure
VIA> auto 60    # Auto-measure every 60s
VIA> stop       # Stop auto mode
```

Data saves to `~/Aeris/data/via/`

---

## Command Reference

| Command | What it does |
|---------|--------------|
| `help` | Show all commands |
| `measure` | Take one measurement |
| `status` | System info |
| `auto N` | Measure every N seconds |
| `stop` | Stop auto mode |
| `Ctrl+C` | Exit console |

---

## Pi Shortcuts

| Command | What it does |
|---------|--------------|
| `via` | Go to VIA directory |
| `via-test` | Run all tests |
| `via-console` | Launch console |
| `aeris-status` | Check system health |
| `ports` | List serial ports |

---

## Data Locations

**Local development:**
```
tests/test_data/           # Generated test data
```

**HIL testing:**
```
Session folder shown at startup
```

**Flatsat:**
```bash
~/Aeris/data/via/YYYYMMDD.HHMM/
├── VIA.YYYYMMDD.HHMM.log         # Session log
├── VIA.YYYYMMDD.HHMM.XX.csv      # Measurements
```

---

## Troubleshooting

**Tests fail?**
```bash
cd tests
python3 test_python_scripts.py  # Run tests individually
```

**No Teensy found?**
- Replug USB cable
- Check: `ls /dev/ttyACM*` (local) or `ports` (Pi)

**Permission denied?**
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Logout and login again

**Can't connect to Pi?**
- Check IP: `192.168.4.163`
- Password: `aeris`

---

## Typical Workflows

### Development Cycle
```bash
# 1. Local testing
cd tests && ./run_all_tests.sh

# 2. Upload to hardware
cd AvaSpecDriver && pio run --target upload

# 3. Test with hardware
./VIA.sh
```

### Flatsat Session
```bash
# Connect
ssh pi@192.168.4.163

# Update and test
via && git pull && via-test

# Run flatsat
via-console

# In console:
VIA> measure
VIA> status
VIA> Ctrl+C
```

---

## Reference Card

```
┌──────────────────────────────────────────────┐
│ VIA TESTING WORKFLOW                         │
├──────────────────────────────────────────────┤
│ LOCAL                                        │
│   tests/run_all_tests.sh                     │
│   pio run --target upload                    │
│   ./VIA.sh                                   │
│                                              │
│ FLATSAT (Pi 400)                             │
│   ssh pi@192.168.4.163  (pwd: aeris)        │
│   via → git pull → via-test → via-console   │
│                                              │
│ CONSOLE COMMANDS                             │
│   measure    Take measurement                │
│   auto 60    Auto every 60s                  │
│   stop       Stop auto                       │
│   status     System info                     │
│   Ctrl+C     Exit                            │
│                                              │
│ DATA                                         │
│   Local: Session folder at startup           │
│   Flatsat: ~/Aeris/data/via/YYYYMMDD.HHMM/  │
└──────────────────────────────────────────────┘
```

---

## More Information

- **Tests:** `tests/README.md`
- **Full docs:** `README.md`
- **Firmware:** `AvaSpecDriver/README.md`
