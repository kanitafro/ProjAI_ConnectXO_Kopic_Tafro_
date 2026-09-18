#!/usr/bin/env python3
"""
ConnectXO macOS Icon Generator
===============================
Generates multi-resolution .icns file from a source PNG image.

Usage (macOS):
	python3 generate_macos_icon.py path/to/source_image.png

Prerequisites:
	- Python 3.6+
	- Pillow: pip install Pillow
	- macOS (for iconutil) OR ImageMagick (cross-platform)

Output:
	- macApp.icns (ready to use in your project)
"""

import os
import sys
import subprocess
import platform
from pathlib import Path

try:
	from PIL import Image
except ImportError:
	print("ERROR: Pillow not installed. Install with:")
	print("  pip install Pillow")
	sys.exit(1)


class IconGenerator:
	"""Generate macOS .icns icon from PNG"""

	# Standard macOS icon sizes
	ICON_SIZES = [16, 32, 64, 128, 256, 512, 1024]

	def __init__(self, source_image, output_dir="."):
		self.source_image = Path(source_image)
		self.output_dir = Path(output_dir)
		self.iconset_dir = self.output_dir / "macApp.iconset"
		self.icns_file = self.output_dir / "macApp.icns"

		if not self.source_image.exists():
			raise FileNotFoundError(f"Source image not found: {source_image}")

	def validate_source(self):
		"""Verify source image is suitable"""
		try:
			img = Image.open(self.source_image)
			width, height = img.size

			print(f"✓ Source image: {self.source_image.name}")
			print(f"  Size: {width}x{height}")
			print(f"  Format: {img.format}")
			print(f"  Mode: {img.mode}")

			# Warnings
			if width < 1024 or height < 1024:
				print(f"  ⚠ WARNING: Image smaller than 1024x1024")
				print(f"    For best quality, use at least 1024x1024 pixels")

			if img.mode != 'RGBA':
				print(f"  ℹ NOTE: Converting {img.mode} to RGBA for transparency")

			return True
		except Exception as e:
			print(f"✗ Error validating source: {e}")
			return False

	def generate_resizes(self):
		"""Create resized icons for each size"""
		try:
			# Create iconset directory
			self.iconset_dir.mkdir(parents=True, exist_ok=True)
			print(f"\n✓ Creating iconset directory: {self.iconset_dir}")

			# Open and convert source to RGBA
			source_img = Image.open(self.source_image).convert('RGBA')

			# Generate each size
			print(f"\nGenerating icon sizes:")
			for size in self.ICON_SIZES:
				# Resize with high-quality filter
				resized = source_img.resize(
					(size, size),
					Image.Resampling.LANCZOS
				)

				# Save as PNG
				output_file = self.iconset_dir / f"icon_{size}x{size}.png"
				resized.save(output_file, 'PNG', quality=95)

				size_kb = output_file.stat().st_size / 1024
				print(f"  {size:4d}x{size:<4d} → {output_file.name} ({size_kb:.1f} KB)")

			return True
		except Exception as e:
			print(f"✗ Error generating resizes: {e}")
			return False

	def convert_to_icns(self):
		"""Convert iconset to .icns using appropriate tool"""
		system = platform.system()

		if system == 'Darwin':  # macOS
			return self._convert_with_iconutil()
		else:
			print(f"\n⚠ WARNING: Running on {system} (not macOS)")
			print(f"  iconutil is only available on macOS")
			print(f"  Options:")
			print(f"    1. Transfer files to macOS and run: iconutil -c icns macApp.iconset")
			print(f"    2. Use online converter: https://convertio.co/png-icns/")
			print(f"    3. Use ImageMagick with additional tools")
			print(f"\n  Iconset folder created at: {self.iconset_dir}")
			return False

	def _convert_with_iconutil(self):
		"""Convert using macOS iconutil command"""
		try:
			print(f"\n✓ Converting iconset to .icns using iconutil...")

			# Check if iconutil is available
			result = subprocess.run(
				['which', 'iconutil'],
				capture_output=True,
				text=True
			)

			if result.returncode != 0:
				print(f"✗ iconutil not found")
				print(f"  Install Xcode Command Line Tools:")
				print(f"    xcode-select --install")
				return False

			# Convert iconset to icns
			subprocess.run(
				['iconutil', '-c', 'icns', str(self.iconset_dir), '-o', str(self.icns_file)],
				check=True,
				capture_output=True
			)

			size_kb = self.icns_file.stat().st_size / 1024
			print(f"  {self.icns_file.name} ({size_kb:.1f} KB)")
			print(f"\n✓ SUCCESS: Created {self.icns_file}")

			return True
		except subprocess.CalledProcessError as e:
			print(f"✗ iconutil error: {e}")
			return False
		except Exception as e:
			print(f"✗ Conversion failed: {e}")
			return False

	def cleanup_temporary_iconset(self, keep=False):
		"""Optionally remove temporary iconset folder"""
		if keep:
			print(f"\n✓ Keeping iconset folder: {self.iconset_dir}")
			print(f"  (Can be reused for future icon updates)")
		else:
			try:
				import shutil
				shutil.rmtree(self.iconset_dir)
				print(f"\n✓ Cleaned up temporary iconset folder")
			except Exception as e:
				print(f"⚠ Could not remove iconset: {e}")

	def run(self, keep_iconset=True):
		"""Generate icon - main workflow"""
		print("=" * 60)
		print("ConnectXO macOS Icon Generator")
		print("=" * 60)

		# Step 1: Validate
		if not self.validate_source():
			return False

		# Step 2: Generate resizes
		if not self.generate_resizes():
			return False

		# Step 3: Convert to icns
		if not self.convert_to_icns():
			print(f"\nℹ Iconset files ready at: {self.iconset_dir}")
			if not keep_iconset:
				self.cleanup_temporary_iconset(keep=False)
			return False

		# Cleanup
		self.cleanup_temporary_iconset(keep=keep_iconset)

		print("\n" + "=" * 60)
		print("✓ Icon generation complete!")
		print("=" * 60)
		print(f"\nNext steps:")
		print(f"  1. Replace res/appIcon/macApp.icns with the generated file")
		print(f"  2. Rebuild your project: cmake --build . --config Release")
		print(f"  3. Test in Finder, Dock, and DMG installer")

		return True


def main():
	if len(sys.argv) < 2:
		print("Usage: python3 generate_macos_icon.py <source_image.png> [output_dir]")
		print("\nExample:")
		print("  python3 generate_macos_icon.py /path/to/ConnectXO_icon_1024.png")
		print("  python3 generate_macos_icon.py ConnectXO_icon.png ./res/appIcon")
		sys.exit(1)

	source_image = sys.argv[1]
	output_dir = sys.argv[2] if len(sys.argv) > 2 else "."

	generator = IconGenerator(source_image, output_dir)
	success = generator.run(keep_iconset=True)

	sys.exit(0 if success else 1)


if __name__ == "__main__":
	main()
