# VIA Spectrometer Quick Start

## Connect to the Pi 400

```bash
ssh aeris@192.168.120.22
```

> **Off-campus?** Connect via Tailscale first.

---

## Run the AERIS Console

```bash
aeris
```

That's it. You'll see the interactive menu:

```
╔════════════════════════════════════════════╗
║          AERIS Control Panel               ║
╚════════════════════════════════════════════╝

┌──────────────────┐
│ VIA Spectrometer │
├──────────────────┴───────────────────────┐
│ 1) Unit Tests        Python test suite   │
│ 2) Simulation        Virtual serial port │
│ 3) HIL Test          Hardware-in-loop    │
└──────────────────────────────────────────┘
...
```

Pick an option:
- **1** - Run unit tests
- **2** - Run simulation (no hardware needed)
- **3** - Connect to real Teensy hardware

---

## CLI Shortcuts

Skip the menu with direct commands:

```bash
aeris via test      # Run unit tests
aeris via sim       # Run simulation
aeris via sim -v    # Simulation with verbose output
aeris update        # Pull latest code
aeris help          # Show all commands
```

---

## VIA Console Commands

Once in the VIA console:

| Command | What it does |
|---------|--------------|
| `help` | Show all commands |
| `measure` | Take one measurement |
| `status` | System info |
| `auto N` | Measure every N seconds |
| `stop` | Stop auto mode |
| `Ctrl+C` | Exit |

---

## View Your Data

Data is saved to `~/Aeris/data/via/YYYYMMDD.HHMM/`:

```bash
ls ~/Aeris/data/via/
```

---

## Troubleshooting

**Can't connect?** Check you're on network `192.168.120.x` or use Tailscale.

**Permission denied on serial?** Run `sudo usermod -a -G dialout $USER` and re-login.

**Tests failing?** Use `aeris via test -v` for verbose output.

---

## More Info

- [README.md](README.md) - Full documentation
- [tests/README.md](tests/README.md) - Test details
