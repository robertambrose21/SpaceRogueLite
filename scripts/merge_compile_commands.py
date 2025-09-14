#!/usr/bin/env python3
import json
from pathlib import Path
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Merge compile_commands.json from subdirectories")
parser.add_argument(
    "--root",
    type=str,
    default="..",
    help="Root directory to search for compile_commands.json (default: ..)"
)
parser.add_argument(
    "--output",
    type=str,
    default="../compile_commands.json",
    help="Output path for merged compile_commands.json (default: ../compile_commands.json)"
)
args = parser.parse_args()

root_dir = Path(args.root).resolve()
merged_file = Path(args.output).resolve()

# Overwrite any existing file
if merged_file.exists():
    merged_file.unlink()

merged_file.parent.mkdir(parents=True, exist_ok=True)

# Find all compile_commands.json recursively
all_files = list(root_dir.rglob("compile_commands.json"))

merged_commands = []

for f in all_files:
    # Only include files in Release folders
    if "Release" not in f.parts:
        continue

    with open(f) as cf:
        commands = json.load(cf)
        for entry in commands:
            # Convert directory to absolute path
            entry["directory"] = str(Path(f).parent.resolve())
        merged_commands.extend(commands)
        print(f"Merged {f} ({len(commands)} entries)")

# Write merged file
with open(merged_file, "w") as out:
    json.dump(merged_commands, out, indent=2)

print(f"\nTotal compile commands: {len(merged_commands)}")
print(f"Merged file written to {merged_file}")