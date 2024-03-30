import argparse
import os
import subprocess
import glob

def run(config):
    cmd = "./test_tinyusdz"

    failure_cases = []
    success_cases = []

    glob_pattern = ["**/*.usd", "**/*.usda", "**/*.usdc", "**/*.usdz"]

    print("Find USD files under: ", config["path"])
    fs = []
    for pat in glob_pattern:
        fs.extend(glob.glob(os.path.join(config["path"], pat), recursive=True))

    for f in fs:
        print(f)
        ret = subprocess.run([cmd, f])
        if ret.returncode != 0:
            failure_cases.append(f)
        else:
            success_cases.append(f)

        print(ret.returncode)

    print("Failure cases: =====================")
    for f in failure_cases:
        print(f)

def main():

    # Assume script is run from <tinyusdz>/build, e.g.:
    #
    # python ../tests/parse_usd/runner.py

    conf = {}
    parser = argparse.ArgumentParser(description='USD parse tester.')
    parser.add_argument('usd_path', type=str, nargs='?', default="../tests/usda",
                    help='Path to USD source tree')

    args = parser.parse_args()

    conf["path"] = args.usd_path

    run(conf)

if __name__ == '__main__':
    main()
