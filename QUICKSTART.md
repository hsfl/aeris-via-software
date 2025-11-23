# VIA Spectrometer Quick Start

Get running with VIA in 5 minutes.

---

## 1. Connect to the Testing Pi

```bash
ssh pi@192.168.4.163
# Password: aeris
```

---

## 2. Get Latest Code & Test

```bash
via              # Go to VIA directory
git pull         # Get updates
via-test         # Run tests (should all pass)
```

---

## 3. Connect Hardware

1. Plug Teensy 4.1 into Pi via USB
2. Connect AvaSpec to Teensy's USB host port
3. Verify: `ports` should show `/dev/ttyACM0`

---

## 4. Launch Console

```bash
via-console
```

You'll see:
```
VIA>
```

---

## 5. Take a Measurement

```
VIA> measure
```

Data saves automatically to `~/Aeris/data/via/`

---

## Common Commands

In the console (`VIA>` prompt):

| Command | What it does |
|---------|--------------|
| `help` | Show all commands |
| `measure` | Take one measurement |
| `status` | System info |
| `auto 60` | Measure every 60 seconds |
| `stop` | Stop auto mode |
| `Ctrl+C` | Exit console |

---

## Shortcuts You Can Use

| Command | What it does |
|---------|--------------|
| `via` | Go to VIA software directory |
| `via-test` | Run all tests |
| `via-console` | Launch console |
| `aeris-status` | Check system health |
| `ports` | List serial ports |

---

## Quick Troubleshooting

**No Teensy found?**
- Replug USB cable
- Run `ports` to check

**Permission denied?**
- Logout and login again: `exit` then `ssh pi@192.168.4.163`

**Tests fail?**
- Check status: `aeris-status`

---

## Typical Session

```bash
# Connect
ssh pi@192.168.4.163

# Update and test
via && git pull && via-test

# Launch console
via-console

# In console:
VIA> measure
VIA> status
VIA> Ctrl+C to exit
```

---

## Where's My Data?

```bash
ls ~/Aeris/data/via/
cd ~/Aeris/data/via/20251122.1430/  # Your session folder
cat VIA.20251122.1430.15.csv        # Your measurement
```

---

## Reference Card (Print This!)

```
┌─────────────────────────────────────────────┐
│ VIA QUICK REFERENCE                         │
├─────────────────────────────────────────────┤
│ CONNECT                                     │
│   ssh pi@192.168.4.163  (pwd: aeris)       │
│                                             │
│ WORKFLOW                                    │
│   via → git pull → via-test → via-console  │
│                                             │
│ CONSOLE                                     │
│   measure   Take measurement                │
│   status    System info                     │
│   help      Show commands                   │
│   Ctrl+C    Exit                            │
│                                             │
│ DATA                                        │
│   ~/Aeris/data/via/YYYYMMDD.HHMM/         │
└─────────────────────────────────────────────┘
```

---

## That's It!

For more details: `tests/README.md`

Happy testing! 🚀
