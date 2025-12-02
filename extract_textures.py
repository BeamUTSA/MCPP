#!/usr/bin/env python3
"""
MCPP Texture Extractor
Extracts block textures from a Minecraft JAR and generates a texture atlas.

Usage:
    python extract_textures.py [--jar PATH] [--output DIR] [--atlas-size SIZE]
"""

import argparse
import json
import math
import os
import sys
import zipfile
from pathlib import Path
from typing import Dict, List, Tuple

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)


# Default paths
DEFAULT_JAR_PATH = r"C:\Users\brayt\AppData\Roaming\.minecraft\versions\1.21.4\1.21.4.jar"
DEFAULT_OUTPUT_DIR = "assets/textures/blocks"
DEFAULT_ATLAS_SIZE = 4096
TEXTURE_SIZE = 16  # Standard Minecraft texture size


class TextureExtractor:
    def __init__(self, jar_path: str, output_dir: str, atlas_size: int):
        self.jar_path = Path(jar_path)
        self.output_dir = Path(output_dir)
        self.atlas_size = atlas_size
        self.textures: Dict[str, Image.Image] = {}
        self.atlas_mapping: Dict[str, Dict] = {}

    def extract_from_jar(self) -> int:
        """Extract block textures from the Minecraft JAR file."""
        if not self.jar_path.exists():
            print(f"ERROR: JAR file not found: {self.jar_path}")
            return 0

        print(f"Opening JAR: {self.jar_path}")

        texture_path_prefix = "assets/minecraft/textures/block/"
        extracted_count = 0

        with zipfile.ZipFile(self.jar_path, 'r') as jar:
            # List all files in the texture directory
            texture_files = [
                f for f in jar.namelist()
                if f.startswith(texture_path_prefix) and f.endswith('.png')
            ]

            print(f"Found {len(texture_files)} block textures")

            for tex_path in texture_files:
                # Get the texture name without path and extension
                tex_name = Path(tex_path).stem

                # Skip animated textures (they have .mcmeta files) for now
                # We'll handle them as static by taking the first frame

                try:
                    with jar.open(tex_path) as tex_file:
                        img = Image.open(tex_file).convert('RGBA')

                        # Handle animated textures (taller than wide)
                        if img.height > img.width:
                            # Take just the first frame
                            img = img.crop((0, 0, img.width, img.width))

                        # Resize to standard size if needed
                        if img.width != TEXTURE_SIZE or img.height != TEXTURE_SIZE:
                            img = img.resize((TEXTURE_SIZE, TEXTURE_SIZE), Image.Resampling.NEAREST)

                        self.textures[tex_name] = img
                        extracted_count += 1

                except Exception as e:
                    print(f"  Warning: Failed to load {tex_name}: {e}")

        print(f"Successfully extracted {extracted_count} textures")
        return extracted_count

    def build_atlas(self) -> Image.Image:
        """Build a texture atlas from extracted textures."""
        if not self.textures:
            print("ERROR: No textures to build atlas from")
            return None

        num_textures = len(self.textures)
        textures_per_row = self.atlas_size // TEXTURE_SIZE
        max_textures = textures_per_row * textures_per_row

        if num_textures > max_textures:
            print(f"WARNING: Too many textures ({num_textures}) for atlas size. "
                  f"Max is {max_textures}. Some will be skipped.")

        print(f"Building {self.atlas_size}x{self.atlas_size} atlas "
              f"({textures_per_row}x{textures_per_row} = {max_textures} slots)")

        # Create the atlas image
        atlas = Image.new('RGBA', (self.atlas_size, self.atlas_size), (0, 0, 0, 0))

        # Sort textures by name for consistent ordering
        sorted_names = sorted(self.textures.keys())

        for idx, tex_name in enumerate(sorted_names):
            if idx >= max_textures:
                break

            tex_img = self.textures[tex_name]

            # Calculate position in atlas
            grid_x = idx % textures_per_row
            grid_y = idx // textures_per_row
            pixel_x = grid_x * TEXTURE_SIZE
            pixel_y = grid_y * TEXTURE_SIZE

            # Paste texture into atlas
            atlas.paste(tex_img, (pixel_x, pixel_y))

            # Store UV coordinates (normalized 0-1)
            u_min = pixel_x / self.atlas_size
            v_min = pixel_y / self.atlas_size
            u_max = (pixel_x + TEXTURE_SIZE) / self.atlas_size
            v_max = (pixel_y + TEXTURE_SIZE) / self.atlas_size

            self.atlas_mapping[tex_name] = {
                "index": idx,
                "grid": [grid_x, grid_y],
                "pixel": [pixel_x, pixel_y],
                "uv": {
                    "min": [u_min, v_min],
                    "max": [u_max, v_max]
                }
            }

        print(f"Placed {len(self.atlas_mapping)} textures in atlas")
        return atlas

    def generate_block_registry(self) -> Dict:
        """Generate a block registry with common Minecraft blocks."""
        # Define the core blocks we want to use
        # Maps our BlockID enum names to texture names
        core_blocks = {
            "Air": None,  # No texture
            "Stone": {"all": "stone"},
            "Dirt": {"all": "dirt"},
            "Grass": {
                "top": "grass_block_top",
                "bottom": "dirt",
                "side": "grass_block_side"
            },
            "Cobblestone": {"all": "cobblestone"},
            "OakPlanks": {"all": "oak_planks"},
            "OakLog": {
                "top": "oak_log_top",
                "bottom": "oak_log_top",
                "side": "oak_log"
            },
            "OakLeaves": {"all": "oak_leaves"},
            "Sand": {"all": "sand"},
            "Gravel": {"all": "gravel"},
            "GoldOre": {"all": "gold_ore"},
            "IronOre": {"all": "iron_ore"},
            "CoalOre": {"all": "coal_ore"},
            "DiamondOre": {"all": "diamond_ore"},
            "Bedrock": {"all": "bedrock"},
            "Water": {"all": "water_still"},
            "Lava": {"all": "lava_still"},
            "Glass": {"all": "glass"},
            "Sandstone": {
                "top": "sandstone_top",
                "bottom": "sandstone_bottom",
                "side": "sandstone"
            },
            "Brick": {"all": "bricks"},
            "TNT": {
                "top": "tnt_top",
                "bottom": "tnt_bottom",
                "side": "tnt_side"
            },
            "MossyCobblestone": {"all": "mossy_cobblestone"},
            "Obsidian": {"all": "obsidian"},
            "CraftingTable": {
                "top": "crafting_table_top",
                "bottom": "oak_planks",
                "side": "crafting_table_side"
            },
            "Furnace": {
                "top": "furnace_top",
                "bottom": "furnace_top",
                "side": "furnace_side",
                "front": "furnace_front"
            },
            "Snow": {"all": "snow"},
            "Ice": {"all": "ice"},
            "Clay": {"all": "clay"},
            "Netherrack": {"all": "netherrack"},
            "SoulSand": {"all": "soul_sand"},
            "Glowstone": {"all": "glowstone"},
            "StoneBricks": {"all": "stone_bricks"},
            "MossyStoneBricks": {"all": "mossy_stone_bricks"},
            "CrackedStoneBricks": {"all": "cracked_stone_bricks"},
            "Deepslate": {"all": "deepslate"},
            "DeepslateCoalOre": {"all": "deepslate_coal_ore"},
            "DeepslateIronOre": {"all": "deepslate_iron_ore"},
            "DeepslateDiamondOre": {"all": "deepslate_diamond_ore"},
            "Diorite": {"all": "diorite"},
            "Granite": {"all": "granite"},
            "Andesite": {"all": "andesite"},
        }

        registry = {"blocks": []}

        for block_name, faces in core_blocks.items():
            block_entry = {
                "name": block_name,
                "id": len(registry["blocks"]),
                "opaque": block_name not in ["Air", "Glass", "Water", "OakLeaves"],
                "solid": block_name not in ["Air", "Water", "Lava"],
            }

            if faces is None:
                block_entry["textures"] = None
            elif "all" in faces:
                tex_name = faces["all"]
                if tex_name in self.atlas_mapping:
                    uv = self.atlas_mapping[tex_name]["uv"]
                    block_entry["textures"] = {
                        "all": {"name": tex_name, "uv": uv}
                    }
                else:
                    print(f"  Warning: Texture '{tex_name}' not found for {block_name}")
                    block_entry["textures"] = None
            else:
                block_entry["textures"] = {}
                for face, tex_name in faces.items():
                    if tex_name in self.atlas_mapping:
                        uv = self.atlas_mapping[tex_name]["uv"]
                        block_entry["textures"][face] = {"name": tex_name, "uv": uv}
                    else:
                        print(f"  Warning: Texture '{tex_name}' not found for {block_name}.{face}")

            registry["blocks"].append(block_entry)

        return registry

    def save_outputs(self, atlas: Image.Image):
        """Save the atlas image and mapping files."""
        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)

        # Save atlas image
        atlas_path = self.output_dir / "block_atlas.png"
        atlas.save(atlas_path, "PNG")
        print(f"Saved atlas: {atlas_path}")

        # Save full texture mapping (for debugging/reference)
        mapping_path = self.output_dir / "atlas_mapping.json"
        with open(mapping_path, 'w') as f:
            json.dump(self.atlas_mapping, f, indent=2)
        print(f"Saved mapping: {mapping_path}")

        # Generate and save block registry
        registry = self.generate_block_registry()
        registry_path = self.output_dir / "block_registry.json"
        with open(registry_path, 'w') as f:
            json.dump(registry, f, indent=2)
        print(f"Saved registry: {registry_path}")

        # Generate C++ header with block IDs
        self.generate_cpp_header(registry)

    def generate_cpp_header(self, registry: Dict):
        """Generate a C++ header file with block definitions."""
        header_path = self.output_dir / "BlockTypes.generated.h"

        lines = [
            "#pragma once",
            "",
            "// AUTO-GENERATED FILE - DO NOT EDIT",
            "// Generated by extract_textures.py",
            "",
            "#include <cstdint>",
            "",
            "namespace MCPP {",
            "",
            "enum class BlockID : uint8_t {",
        ]

        for block in registry["blocks"]:
            lines.append(f"    {block['name']} = {block['id']},")

        lines.append(f"    COUNT = {len(registry['blocks'])}")
        lines.append("};")
        lines.append("")
        lines.append(f"constexpr uint32_t BLOCK_COUNT = {len(registry['blocks'])};")
        lines.append("")
        lines.append("} // namespace MCPP")
        lines.append("")

        with open(header_path, 'w') as f:
            f.write('\n'.join(lines))
        print(f"Saved C++ header: {header_path}")


def main():
    parser = argparse.ArgumentParser(description="Extract Minecraft textures and build atlas")
    parser.add_argument("--jar", default=DEFAULT_JAR_PATH, help="Path to Minecraft JAR file")
    parser.add_argument("--output", default=DEFAULT_OUTPUT_DIR, help="Output directory")
    parser.add_argument("--atlas-size", type=int, default=DEFAULT_ATLAS_SIZE,
                        help="Atlas size in pixels (must be power of 2)")
    args = parser.parse_args()

    # Validate atlas size
    if args.atlas_size & (args.atlas_size - 1) != 0:
        print("ERROR: Atlas size must be a power of 2")
        sys.exit(1)

    print("=" * 60)
    print("MCPP Texture Extractor")
    print("=" * 60)

    extractor = TextureExtractor(args.jar, args.output, args.atlas_size)

    # Extract textures
    count = extractor.extract_from_jar()
    if count == 0:
        print("No textures extracted. Exiting.")
        sys.exit(1)

    # Build atlas
    atlas = extractor.build_atlas()
    if atlas is None:
        sys.exit(1)

    # Save outputs
    extractor.save_outputs(atlas)

    print("=" * 60)
    print("Done!")
    print("=" * 60)


if __name__ == "__main__":
    main()