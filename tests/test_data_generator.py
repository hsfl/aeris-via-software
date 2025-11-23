#!/usr/bin/env python3
"""
Test Data Generator for VIA Spectrometer Testing

Generates realistic simulated spectrometer data for testing
without requiring physical hardware.
"""

import random
import math
from pathlib import Path


class SpectrometerSimulator:
    """Simulates AvaSpec-Mini2048CL spectrometer output"""

    def __init__(self, num_pixels=2048, seed=42):
        """
        Initialize simulator

        Args:
            num_pixels: Number of pixels (default 2048)
            seed: Random seed for reproducibility
        """
        self.num_pixels = num_pixels
        self.seed = seed
        self.rng = random.Random(seed)

    def generate_gaussian_peak(self, center=1024, width=100, amplitude=50000):
        """
        Generate a spectrum with a Gaussian peak

        Args:
            center: Center pixel of the peak
            width: Width (standard deviation) of the peak
            amplitude: Maximum intensity

        Returns:
            List of intensity values (one per pixel)
        """
        intensities = []
        baseline = 1000  # Background noise level

        for pixel in range(self.num_pixels):
            # Gaussian peak
            gaussian = amplitude * math.exp(-((pixel - center) ** 2) / (2 * width ** 2))

            # Add noise
            noise = self.rng.gauss(0, 50)

            # Combine
            intensity = int(baseline + gaussian + noise)

            # Clamp to valid range
            intensity = max(0, min(65535, intensity))
            intensities.append(intensity)

        return intensities

    def generate_multi_peak(self, peaks=None):
        """
        Generate spectrum with multiple peaks

        Args:
            peaks: List of (center, width, amplitude) tuples
                   If None, generates 3 random peaks

        Returns:
            List of intensity values
        """
        if peaks is None:
            peaks = [
                (512, 80, 40000),
                (1024, 100, 55000),
                (1536, 90, 45000)
            ]

        intensities = [1000] * self.num_pixels  # Baseline

        for center, width, amplitude in peaks:
            for pixel in range(self.num_pixels):
                gaussian = amplitude * math.exp(-((pixel - center) ** 2) / (2 * width ** 2))
                intensities[pixel] += int(gaussian)

        # Add noise
        for i in range(self.num_pixels):
            noise = self.rng.gauss(0, 50)
            intensities[i] = max(0, min(65535, int(intensities[i] + noise)))

        return intensities

    def generate_flat_spectrum(self, intensity=10000):
        """
        Generate a flat (uniform) spectrum

        Args:
            intensity: Constant intensity value

        Returns:
            List of intensity values
        """
        return [intensity + self.rng.randint(-100, 100) for _ in range(self.num_pixels)]

    def generate_csv(self, intensities, output_path):
        """
        Write intensity data to CSV file

        Args:
            intensities: List of intensity values
            output_path: Path to output CSV file
        """
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w') as f:
            f.write("Pixel,Intensity\n")
            for pixel, intensity in enumerate(intensities):
                f.write(f"{pixel},{intensity}\n")

        print(f"Generated CSV: {output_path}")

    def generate_raw_hex(self, intensities):
        """
        Generate raw hex output matching Teensy serial format

        Args:
            intensities: List of intensity values

        Returns:
            String containing hex-formatted data
        """
        lines = []
        lines.append("Reading full 4106-byte measurement...")

        # Header (10 bytes) - simplified version
        header = [0x20, 0x00, 0x0A, 0x10, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00]

        # Convert intensities to bytes (little-endian 16-bit)
        data_bytes = header.copy()
        for intensity in intensities:
            data_bytes.append(intensity & 0xFF)  # LSB
            data_bytes.append((intensity >> 8) & 0xFF)  # MSB

        # Format as hex lines (16 bytes per line)
        for i in range(0, len(data_bytes), 16):
            chunk = data_bytes[i:i+16]
            hex_line = ' '.join(f"{b:02X}" for b in chunk)
            lines.append(hex_line)

        lines.append("Full 4106 bytes received")
        return '\n'.join(lines)


def generate_test_datasets():
    """Generate a variety of test datasets"""
    sim = SpectrometerSimulator()
    test_dir = Path(__file__).parent / "test_data"
    test_dir.mkdir(exist_ok=True)

    print("Generating test datasets...")

    # 1. Single Gaussian peak
    data1 = sim.generate_gaussian_peak(center=1024, width=100, amplitude=50000)
    sim.generate_csv(data1, test_dir / "gaussian_single.csv")

    # 2. Multiple peaks
    data2 = sim.generate_multi_peak()
    sim.generate_csv(data2, test_dir / "multi_peak.csv")

    # 3. Flat spectrum
    data3 = sim.generate_flat_spectrum(intensity=15000)
    sim.generate_csv(data3, test_dir / "flat_spectrum.csv")

    # 4. Low signal
    data4 = sim.generate_gaussian_peak(center=800, width=50, amplitude=5000)
    sim.generate_csv(data4, test_dir / "low_signal.csv")

    # 5. High signal
    data5 = sim.generate_gaussian_peak(center=1500, width=150, amplitude=60000)
    sim.generate_csv(data5, test_dir / "high_signal.csv")

    # 6. Generate raw hex output sample
    hex_output = sim.generate_raw_hex(data1)
    with open(test_dir / "raw_hex_sample.txt", 'w') as f:
        f.write(hex_output)
    print(f"Generated hex sample: {test_dir / 'raw_hex_sample.txt'}")

    print("\nTest data generation complete!")
    print(f"Test data location: {test_dir}")


if __name__ == "__main__":
    generate_test_datasets()
