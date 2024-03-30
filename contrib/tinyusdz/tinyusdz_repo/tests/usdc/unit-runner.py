import os
import sys
import glob
import subprocess

# Default configuration Assume running this script from <tinyusdz> root directory
if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("--basedir", type=str, default="tests/usdc")
    parser.add_argument("--app", type=str, default="./build/test_tinyusdz")
    args = parser.parse_args()

    app = args.app    

    failed = []
    #false_negatives = []
    
    print("Basedir: ", args.basedir)
    print("App: ", args.app)

    cnt = 0
    # success expected
    for fname in glob.glob(os.path.join(args.basedir, "*.usdc")):
        print(fname)
        cmd = [app, fname]

        ret = subprocess.call(cmd)
        if ret != 0:
            failed.append(fname)

        cnt += 1

    # failure expected
    for fname in glob.glob(os.path.join(args.basedir, "failure-case/*.usdc")):
        cmd = [app, fname]

        ret = subprocess.call(cmd)
        if ret == 0:
            failed.append(fname)

        cnt += 1
         
    print("=================================")

    if len(failed) > 0:
        for fname in failed:
            # failed
            print("ERROR: parse failed for : ", fname)

    #if len(false_negatives) > 0:
    #    for fname in false_negatives:
    #        print("WARN: parse should fail but reported success : ", fname)

    print("Tested {} USDC files.".format(cnt))

    if len(failed) > 0:
        print("USDC parse test failed")
        sys.exit(1)
    else:
        print("USDC parse test success")
        sys.exit(0)
