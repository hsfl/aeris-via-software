#!/usr/bin/env python3
"""
VIA Spectrometer Unit Tests

Each test validates a single unit of functionality with known inputs
and expected outputs. Tests are independent of each other.

Usage:
    python3 test_python_scripts.py        # Quiet (failures only)
    python3 test_python_scripts.py -v     # Verbose (all tests)
"""

import unittest
import sys
import csv
import tempfile
from pathlib import Path

# Add parent directory to path to import scripts
sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from test_data_generator import SpectrometerSimulator


class TestCSVFormat(unittest.TestCase):
    """Tests for CSV data format - validates file structure"""

    def test_csv_has_header(self):
        """CSV file starts with Pixel,Intensity header"""
        sim = SpectrometerSimulator(seed=100)
        intensities = sim.generate_flat_spectrum(intensity=1000)
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        sim.generate_csv(intensities, temp_file)

        with open(temp_file, 'r') as f:
            header = f.readline().strip()

        self.assertEqual(header, "Pixel,Intensity")
        temp_file.unlink()

    def test_csv_has_2048_data_rows(self):
        """CSV file contains exactly 2048 data rows (one per pixel)"""
        sim = SpectrometerSimulator(seed=101)
        intensities = sim.generate_flat_spectrum(intensity=1000)
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        sim.generate_csv(intensities, temp_file)

        with open(temp_file, 'r') as f:
            lines = f.readlines()

        # Header + 2048 data lines
        self.assertEqual(len(lines), 2049)
        temp_file.unlink()

    def test_csv_pixel_range_0_to_2047(self):
        """CSV pixels range from 0 to 2047"""
        sim = SpectrometerSimulator(seed=102)
        intensities = sim.generate_flat_spectrum(intensity=1000)
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        sim.generate_csv(intensities, temp_file)

        pixels = []
        with open(temp_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                pixels.append(int(row["Pixel"]))

        self.assertEqual(pixels[0], 0)
        self.assertEqual(pixels[-1], 2047)
        self.assertEqual(len(pixels), 2048)
        temp_file.unlink()


class TestIntensityRange(unittest.TestCase):
    """Tests for intensity value constraints (16-bit ADC)"""

    def test_intensity_non_negative(self):
        """All intensity values are >= 0"""
        sim = SpectrometerSimulator(seed=200)
        intensities = sim.generate_gaussian_peak()

        for i, val in enumerate(intensities):
            self.assertGreaterEqual(val, 0, f"Pixel {i} has negative intensity")

    def test_intensity_within_16bit(self):
        """All intensity values fit in 16-bit range (0-65535)"""
        sim = SpectrometerSimulator(seed=201)
        intensities = sim.generate_gaussian_peak(amplitude=60000)

        for i, val in enumerate(intensities):
            self.assertLessEqual(val, 65535, f"Pixel {i} exceeds 16-bit max")

    def test_zero_intensity_valid(self):
        """Zero intensity values are handled correctly"""
        sim = SpectrometerSimulator(seed=202)
        intensities = [0] * 2048
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        sim.generate_csv(intensities, temp_file)

        with open(temp_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                self.assertEqual(int(row["Intensity"]), 0)

        temp_file.unlink()

    def test_max_intensity_valid(self):
        """Maximum intensity (65535) is handled correctly"""
        sim = SpectrometerSimulator(seed=203)
        intensities = [65535] * 2048
        temp_file = Path(tempfile.mktemp(suffix=".csv"))

        sim.generate_csv(intensities, temp_file)

        with open(temp_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                self.assertEqual(int(row["Intensity"]), 65535)

        temp_file.unlink()


class TestGaussianPeak(unittest.TestCase):
    """Tests for Gaussian peak generation - validates physics simulation"""

    def test_peak_at_specified_center(self):
        """Peak maximum is within 10 pixels of specified center"""
        sim = SpectrometerSimulator(seed=300)
        center = 1024
        intensities = sim.generate_gaussian_peak(center=center, amplitude=50000)

        max_idx = intensities.index(max(intensities))

        self.assertLess(abs(max_idx - center), 10,
                        f"Peak at {max_idx}, expected near {center}")

    def test_peak_amplitude_matches(self):
        """Peak height approximately matches specified amplitude"""
        sim = SpectrometerSimulator(seed=301)
        amplitude = 40000
        intensities = sim.generate_gaussian_peak(amplitude=amplitude)

        max_val = max(intensities)

        # Allow tolerance for baseline (~1000) and noise (~50)
        self.assertGreater(max_val, amplitude * 0.9)
        self.assertLess(max_val, amplitude + 2000)  # baseline + noise headroom

    def test_gaussian_shape_symmetry(self):
        """Peak is roughly symmetric around center"""
        sim = SpectrometerSimulator(seed=302)
        center = 1024
        intensities = sim.generate_gaussian_peak(center=center, width=100)

        # Compare values equidistant from center
        left_val = intensities[center - 50]
        right_val = intensities[center + 50]

        # Should be within 30% of each other (noise adds variance)
        diff = abs(left_val - right_val)
        avg = (left_val + right_val) / 2
        self.assertLess(diff / avg, 0.3, "Peak is not symmetric")


class TestHexFormat(unittest.TestCase):
    """Tests for raw hex output format - validates firmware communication"""

    def test_hex_starts_with_measurement_header(self):
        """Hex output begins with measurement indicator"""
        sim = SpectrometerSimulator(seed=400)
        intensities = sim.generate_flat_spectrum(intensity=10000)
        hex_output = sim.generate_raw_hex(intensities)

        first_line = hex_output.split('\n')[0]
        self.assertIn("4106-byte", first_line)

    def test_hex_ends_with_completion(self):
        """Hex output ends with completion indicator"""
        sim = SpectrometerSimulator(seed=401)
        intensities = sim.generate_flat_spectrum(intensity=10000)
        hex_output = sim.generate_raw_hex(intensities)

        last_line = hex_output.split('\n')[-1]
        self.assertIn("4106 bytes received", last_line)

    def test_hex_bytes_are_valid(self):
        """All hex values are valid 2-character hex bytes"""
        sim = SpectrometerSimulator(seed=402)
        intensities = sim.generate_flat_spectrum(intensity=10000)
        hex_output = sim.generate_raw_hex(intensities)

        lines = hex_output.split('\n')
        for line in lines[1:-1]:  # Skip header and footer
            if line.strip():
                parts = line.split()
                for part in parts:
                    self.assertEqual(len(part), 2, f"'{part}' is not 2 chars")
                    int(part, 16)  # Must parse as hex


class TestReproducibility(unittest.TestCase):
    """Tests for deterministic behavior - same seed = same output"""

    def test_same_seed_same_gaussian(self):
        """Identical seeds produce identical Gaussian peaks"""
        sim1 = SpectrometerSimulator(seed=500)
        sim2 = SpectrometerSimulator(seed=500)

        data1 = sim1.generate_gaussian_peak()
        data2 = sim2.generate_gaussian_peak()

        self.assertEqual(data1, data2)

    def test_same_seed_same_multipeak(self):
        """Identical seeds produce identical multi-peak spectra"""
        sim1 = SpectrometerSimulator(seed=501)
        sim2 = SpectrometerSimulator(seed=501)

        data1 = sim1.generate_multi_peak()
        data2 = sim2.generate_multi_peak()

        self.assertEqual(data1, data2)

    def test_different_seeds_different_output(self):
        """Different seeds produce different outputs"""
        sim1 = SpectrometerSimulator(seed=502)
        sim2 = SpectrometerSimulator(seed=503)

        data1 = sim1.generate_gaussian_peak()
        data2 = sim2.generate_gaussian_peak()

        self.assertNotEqual(data1, data2)


class TestMultiPeak(unittest.TestCase):
    """Tests for multi-peak spectrum generation"""

    def test_spectrum_length_2048(self):
        """Multi-peak spectrum has exactly 2048 points"""
        sim = SpectrometerSimulator(seed=600)
        intensities = sim.generate_multi_peak()

        self.assertEqual(len(intensities), 2048)

    def test_no_discontinuities(self):
        """Adjacent pixels don't have unrealistic jumps (>5000 counts)"""
        sim = SpectrometerSimulator(seed=601)
        intensities = sim.generate_multi_peak()

        for i in range(len(intensities) - 1):
            jump = abs(intensities[i + 1] - intensities[i])
            self.assertLess(jump, 5000,
                            f"Discontinuity at pixel {i}: jump of {jump}")


class CompactTestResult(unittest.TextTestResult):
    """Custom test result that shows short descriptions."""

    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream, descriptions, verbosity)
        self.successes = []

    def addSuccess(self, test):
        super().addSuccess(test)
        self.successes.append(test)

    def printResults(self):
        """Print compact results with checkmarks."""
        for test in self.successes:
            desc = test.shortDescription() or str(test)
            print(f"    \033[0;32m✓\033[0m {desc}")
        for test, _ in self.failures:
            desc = test.shortDescription() or str(test)
            print(f"    \033[0;31m✗\033[0m {desc}")
        for test, _ in self.errors:
            desc = test.shortDescription() or str(test)
            print(f"    \033[0;31m✗\033[0m {desc}")


def run_tests():
    """Run tests with configurable verbosity"""
    verbose = "-v" in sys.argv or "--verbose" in sys.argv

    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestCSVFormat))
    suite.addTests(loader.loadTestsFromTestCase(TestIntensityRange))
    suite.addTests(loader.loadTestsFromTestCase(TestGaussianPeak))
    suite.addTests(loader.loadTestsFromTestCase(TestHexFormat))
    suite.addTests(loader.loadTestsFromTestCase(TestReproducibility))
    suite.addTests(loader.loadTestsFromTestCase(TestMultiPeak))

    if verbose:
        runner = unittest.TextTestRunner(verbosity=2)
        result = runner.run(suite)
    else:
        # Quiet mode - collect results silently, then print compact summary
        import io
        import contextlib

        # Suppress all stdout during test run (e.g., "Generated CSV" messages)
        with contextlib.redirect_stdout(io.StringIO()):
            stream = open('/dev/null', 'w')
            runner = unittest.TextTestRunner(stream=stream, verbosity=0, resultclass=CompactTestResult)
            result = runner.run(suite)
            stream.close()

        result.printResults()

    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
