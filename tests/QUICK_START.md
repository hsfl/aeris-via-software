# Quick Start - Testing VIA Software Without Hardware

## Run All Tests (Recommended)

```bash
cd tests
./run_all_tests.sh
```

**Expected result:** All tests pass in ~15-40 seconds

## Test Individual Components

### 1. Generate Test Data

```bash
python3 test_data_generator.py
```

Creates realistic spectrum data in `test_data/` directory.

### 2. Run Unit Tests

```bash
python3 test_python_scripts.py
```

Runs 10 automated tests of data processing.

### 3. Simulate Teensy Firmware

**Terminal 1:**
```bash
python3 virtual_serial_port.py
```

**Terminal 2:**
```bash
python3 ../scripts/via_interactive.py /tmp/ttyVIA0
```

Now you can type commands like:
- `help` - Show available commands
- `measure` - Take a measurement
- `status` - Show system status

## Visualize Test Data

```bash
python3 ../scripts/SerialRead.py test_data/gaussian_single.csv
```

Shows a plot of simulated spectrum data.

## What Each Test Does

| Test | What It Checks | Duration |
|------|----------------|----------|
| **Data Generator** | Creates realistic spectrum data | 2s |
| **Unit Tests** | Validates data processing logic | 3s |
| **Firmware Build** | Ensures code compiles | 10-30s |
| **Visualization** | Checks plotting works | 2s |
| **Virtual Serial** | Interactive firmware simulation | Manual |

## Common Issues

### "No module named matplotlib"
```bash
pip install matplotlib
```

### "No module named serial"
```bash
pip install pyserial
```

### "Permission denied" on run_all_tests.sh
```bash
chmod +x run_all_tests.sh
```

## Next Steps

After tests pass, you can:

1. **Modify firmware** - Edit `AvaSpecDriver/src/main.cpp`
2. **Test changes** - Run `./run_all_tests.sh` again
3. **Upload to hardware** - When ready, flash to Teensy 4.1

## Full Documentation

See [README.md](README.md) for complete documentation.
