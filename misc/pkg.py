#!/usr/bin/env python

# Copyright 2017 Alexander Gallego
#

import sys
import os
import logging
import logging.handlers
import subprocess
import argparse
import tempfile
import os.path
import json
import glob
import shutil
import re

fmt_string = '%(levelname)s:%(asctime)s %(filename)s:%(lineno)d] %(message)s'
logging.basicConfig(format=fmt_string)
formatter = logging.Formatter(fmt_string)
for h in logging.getLogger().handlers:
    h.setFormatter(formatter)
logger = logging.getLogger('pkg')
logger.setLevel(logging.DEBUG)

BOOTSTRAP_ENVIRONMENT = b"""
import os
import sys
__entry_point__ = None
if '__file__' in locals() and __file__ is not None:
  __entry_point__ = os.path.dirname(__file__)
elif '__loader__' in locals():
  from zipimport import zipimporter
  from pkgutil import ImpLoader
  if hasattr(__loader__, 'archive'):
    __entry_point__ = __loader__.archive
  elif isinstance(__loader__, ImpLoader):
    __entry_point__ = os.path.dirname(__loader__.get_filename())
if __entry_point__ is None:
  sys.stderr.write('Could not launch python executable!\\n')
  sys.exit(2)
sys.path[0] = os.path.abspath(sys.path[0])
sys.path.insert(0, os.path.abspath(os.path.join(__entry_point__, '.bootstrap')))
from _pex.pex_bootstrapper import bootstrap_pex
bootstrap_pex(__entry_point__)
"""

RUN_TEST = b"""
import os
import sys
import subprocess
def run_subprocess(cmd):
    proc =  subprocess.Popen(
        cmd,
        stdout=sys.stdout,
        stderr=sys.stderr,
        env=environ,
        shell=True
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

"""


def get_ldd_lines(binary):
    ret = str(subprocess.check_output("ldd {}".format(binary), shell=True))
    if ret is None:
        raise Exception("Error ldd'ing binary: %s", ret)
    return ret.splitlines()


def clean_ldd(output, lines):
    def link_line(line):
        match = re.match(r'\t(.*) => (.*) \(0x', line)
        if match:
            output[match.group(1)] = match.group(2)
        return match

    def hard_dep_line(line):
        match = re.match(r'\t(/.*) \(0x', line)
        if match:
            output[match.group(1)] = match.group(1)
        return match

    filters = [link_line, hard_dep_line]
    for line in lines:
        any(f(line) for f in filters)


def get_binary_libs(binary):
    libraries = {}
    ldd_libs = get_ldd_lines(binary)
    clean_ldd(libraries, ldd_libs)
    return libraries


def create_archive_layout():
    dirpath = tempfile.mkdtemp()
    dirs = ["/lib", "/bin", "/etc"]
    for d in dirs:
        os.makedirs(dirpath + d)
    return dirpath


def create_archive(binary, runner):
    root_dir = create_archive_layout()
    logger.info("Packaging at %s", root_dir)
    libs = get_binary_libs(binary)
    shutil.copy2(binary, root_dir + "/bin/" + os.path.basename(binary))
    for key in libs:
        l = libs[key]
        shutil.copy2(l, root_dir + "/lib/" + os.path.basename(l))
    shutil.copy2(runner, root_dir + "/" + os.path.basename(runner))


def generate_options():
    parser = argparse.ArgumentParser(description='run smf integration tests')
    parser.add_argument('--binary', type=str, help='binary program to run')
    parser.add_argument('--runner', type=str, help='program to run executable')
    return parser


def main():
    parser = generate_options()
    options, program_options = parser.parse_known_args()
    if not options.binary:
        parser.print_help()
        raise Exception("Missing binary")
    if not options.runner:
        parser.print_help()
        raise Exception("Missing runner script usually r.py")
    create_archive(options.binary, options.runner)


if __name__ == '__main__':
    main()
