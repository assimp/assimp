import tinyusdz
import sys
import argparse

if len(sys.argv) < 2:
    print("Need input.usd/.usdc/.usdz")
    sys.exit(-1)

filepath = sys.argv[1]

# Use Path() is recommended
# from pathlib import Path
# filepath = Path(sys.argv[1])

a = tinyusdz.Stage()

stage: tinyusdz.Stage = tinyusdz.load_usd(filepath)
print(stage)
print(stage.export_to_string())