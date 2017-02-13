#!/usr/bin/env python
import os
import sys
import subprocess
import argparse
import glob
def run_subprocess(cmd):
    env = os.environ.copy()
    cwd = os.getcwd()
    env["LD_PRELOAD"] = cwd + "/lib/ld-linux-x86-64.so.2:" + cwd + "/lib/libc.so.6"
    env["LD_LIBRARY_PATH"] = ".:lib:" + cwd + "/lib"
    print(env)
    proc =  subprocess.Popen(
        cmd,
        stdout=sys.stdout,
        stderr=sys.stderr,
        env=env,
        shell=False
    )
    return_code = 0

    try:
        return_code = proc.wait()
        sys.stdout.flush()
        sys.stderr.flush()
    except Exception as e:
        logger.exception("Could not run command: % ", e)
        proc.terminate()
        raise

def generate_options():
    parser = argparse.ArgumentParser(description='run smf integration tests')
    parser.add_argument('--binary', type=str, help='binary program to run')
    return parser

def main():
    parser = generate_options()
    options, program_options = parser.parse_known_args()
    if not options.binary:
        parser.print_help()
        raise Exception("Missing binary")

    run_subprocess(options.binary)

if __name__ == '__main__':
    main()
