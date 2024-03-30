import os
import glob
import subprocess

# Default configuration Assume running this script from <tinyusdz> root directory
if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("--basedir", type=str, default="../usda")
    args = parser.parse_args()

    failed = []
    
    print("Basedir: ", args.basedir)

    # success expected
    for fname in glob.glob(os.path.join(args.basedir, "*.usda")):
        print(fname)
        basename = os.path.basename(fname)    
        out_filename = os.path.splitext(basename)[0] + ".usdc"
        cmd = ["usdcat", fname, '-o', out_filename]
        print(cmd)

        ret = subprocess.call(cmd)
        if ret != 0:
            failed.append(fname)

    print("=================================")

    if len(failed) > 0:
        for fname in failed:
            # failed
            print("failed to execute : ", fname)
