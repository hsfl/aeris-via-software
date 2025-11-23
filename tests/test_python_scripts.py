#!/usr/bin/env python3
"""
Unit tests for VIA Python scripts

Tests the data processing scripts without requiring hardware.
"""

import unittest
import sys
import csv
import tempfile
from pathlib import Path

# Add parent directory to path to import scripts
sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from test_data_generator import SpectrometerSimulator


class TestSerialReadPlotting(unittest.TestCase):
    """Test the SerialRead.py plotting functionality"""

    def setUp(self):
        """Create temporary test data files"""
        self.temp_dir = tempfile.mkdtemp()
        self.sim = SpectrometerSimulator(seed=123)

    def test_csv_loading(self):
        """Test CSV file loading"""
        # Generate test data
        intensities = self.sim.generate_gaussian_peak()
        csv_path = Path(self.temp_dir) / "test.csv"

        # Write CSV
        with open(csv_path, 'w') as f:
            f.write("Pixel,Intensity\n")
            for i, val in enumerate(intensities):
                f.write(f"{i},{val}\n")

        # Test loading
        pixels, intensity = [], []
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                pixels.append(int(row["Pixel"]))
                intensity.append(float(row["Intensity"]))

        # Verify
        self.assertEqual(len(pixels), 2048)
        self.assertEqual(len(intensity), 2048)
        self.assertEqual(pixels[0], 0)
        self.assertEqual(pixels[-1], 2047)

    def test_data_range_validation(self):
        """Test that intensity values are in valid range"""
        intensities = self.sim.generate_gaussian_peak()

        for val in intensities:
            self.assertGreaterEqual(val, 0, "Intensity should be non-negative")
            self.assertLessEqual(val, 65535, "Intensity should fit in 16-bit range")

    def test_peak_detection(self):
        """Test that generated peaks are at expected locations"""
        center = 1024
        intensities = self.sim.generate_gaussian_peak(center=center, amplitude=50000)

        # Find maximum
        max_idx = intensities.index(max(intensities))

        # Should be near the center (within 10 pixels due to noise)
        self.assertLess(abs(max_idx - center), 10,
                       f"Peak at {max_idx} should be near {center}")


class TestDataFormats(unittest.TestCase):
    """Test data format conversions"""

    def setUp(self):
        self.sim = SpectrometerSimulator(seed=456)

    def test_csv_format(self):
        """Test CSV output format"""
        intensities = self.sim.generate_flat_spectrum(intensity=10000)
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        self.sim.generate_csv(intensities, temp_file)

        # Verify file exists and has correct number of lines
        self.assertTrue(temp_file.exists())

        with open(temp_file, 'r') as f:
            lines = f.readlines()

        # Header + 2048 data lines
        self.assertEqual(len(lines), 2049)
        self.assertEqual(lines[0].strip(), "Pixel,Intensity")

        # Clean up
        temp_file.unlink()

    def test_hex_output_format(self):
        """Test hex output matches expected format"""
        intensities = self.sim.generate_flat_spectrum(intensity=15000)
        hex_output = self.sim.generate_raw_hex(intensities)

        lines = hex_output.split('\n')

        # Should start with measurement indicator
        self.assertIn("Reading full 4106-byte measurement", lines[0])

        # Should end with completion indicator
        self.assertIn("Full 4106 bytes received", lines[-1])

        # Check hex format in middle lines
        for line in lines[1:-1]:
            if line.strip():
                # Each hex byte should be 2 characters followed by space
                parts = line.split()
                for part in parts:
                    self.assertEqual(len(part), 2, f"Hex byte '{part}' should be 2 chars")
                    int(part, 16)  # Should parse as hex


class TestMultiPeakGeneration(unittest.TestCase):
    """Test multi-peak spectrum generation"""

    def setUp(self):
        self.sim = SpectrometerSimulator(seed=789)

    def test_multi_peak_count(self):
        """Test that multiple peaks are generated"""
        peaks = [
            (400, 50, 30000),
            (1000, 80, 40000),
            (1600, 60, 35000)
        ]
        intensities = self.sim.generate_multi_peak(peaks)

        # Count local maxima (simplified peak detection)
        local_maxima = []
        for i in range(100, len(intensities) - 100):
            if intensities[i] > intensities[i-50] and intensities[i] > intensities[i+50]:
                if intensities[i] > 10000:  # Above background
                    local_maxima.append(i)

        # Should find approximately 3 peaks
        self.assertGreaterEqual(len(local_maxima), 2, "Should detect multiple peaks")

    def test_spectrum_continuity(self):
        """Test that spectrum has no discontinuities"""
        intensities = self.sim.generate_multi_peak()

        # Check for unrealistic jumps (> 10000 counts between adjacent pixels)
        max_jump = 0
        for i in range(len(intensities) - 1):
            jump = abs(intensities[i+1] - intensities[i])
            max_jump = max(max_jump, jump)

        self.assertLess(max_jump, 10000, "Adjacent pixels should not have huge jumps")


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and error handling"""

    def setUp(self):
        self.sim = SpectrometerSimulator()

    def test_zero_intensity(self):
        """Test handling of zero intensity"""
        intensities = [0] * 2048
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        self.sim.generate_csv(intensities, temp_file)

        # Should create file without errors
        self.assertTrue(temp_file.exists())
        temp_file.unlink()

    def test_max_intensity(self):
        """Test handling of maximum intensity (65535)"""
        intensities = [65535] * 2048
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        self.sim.generate_csv(intensities, temp_file)

        # Should create file without errors
        self.assertTrue(temp_file.exists())

        # Verify values
        with open(temp_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                self.assertEqual(int(row["Intensity"]), 65535)

        temp_file.unlink()

    def test_reproducibility(self):
        """Test that same seed produces same results"""
        sim1 = SpectrometerSimulator(seed=999)
        sim2 = SpectrometerSimulator(seed=999)

        data1 = sim1.generate_gaussian_peak()
        data2 = sim2.generate_gaussian_peak()

        self.assertEqual(data1, data2, "Same seed should produce identical data")


def run_tests():
    """Run all tests and print summary"""
    print("=" * 70)
    print("VIA SPECTROMETER SOFTWARE TEST SUITE")
    print("=" * 70)
    print()

    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestSerialReadPlotting))
    suite.addTests(loader.loadTestsFromTestCase(TestDataFormats))
    suite.addTests(loader.loadTestsFromTestCase(TestMultiPeakGeneration))
    suite.addTests(loader.loadTestsFromTestCase(TestEdgeCases))

    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    # Print summary
    print()
    print("=" * 70)
    print("TEST SUMMARY")
    print("=" * 70)
    print(f"Tests run:     {result.testsRun}")
    print(f"Successes:     {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"Failures:      {len(result.failures)}")
    print(f"Errors:        {len(result.errors)}")
    print("=" * 70)

    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
