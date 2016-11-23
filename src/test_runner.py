#!/usr/bin/env python

import sys
import os
import logging
import logging.handlers
import subprocess
import argparse
import tempfile
import os.path
import json

fmt_string='%(levelname)s:%(asctime)s %(filename)s:%(lineno)d] %(message)s'
logging.basicConfig(format=fmt_string)
formatter = logging.Formatter(fmt_string)
for h in logging.getLogger().handlers: h.setFormatter(formatter)
logger = logging.getLogger('it.test')
logger.setLevel(logging.DEBUG)

def generate_options():
    parser = argparse.ArgumentParser(description='run smf integration tests')
    parser.add_argument('--binary', type=str,
                        help='binary program to run')
    parser.add_argument('--directory', type=str,
                        help='source directory of binary. needed for metadata')
    return parser



def get_git_root():
    ret = str(subprocess.check_output("git rev-parse --show-toplevel",
                                      shell=True))
    if ret is None:
        log.error("Cannot get the git root")
        sys.exit(1)
    return "".join(ret.split())

def test_environ():
    e = os.environ
    git_root = get_git_root()
    e["GIT_ROOT"] = git_root
    libs = "{}/src/third_party/lib:{}/src/third_party/lib64:{}".format(
        git_root,git_root, e["LD_LIBRARY_PATH"])
    e["LD_LIBRARY_PATH"]=libs
    e["GLOG_logtostderr"]='1'
    e["GLOG_v"]='1'
    e["GLOG_vmodule"]=''
    e["GLOG_logbufsecs"]='0'
    e["GLOG_log_dir"]='.'
    e["GTEST_COLOR"]='no'
    e["GTEST_SHUFFLE"]='1'
    e["GTEST_BREAK_ON_FAILURE"]='1'
    e["GTEST_REPEAT"]='1'
    return e


def bash_command(cmd, environ):
    exe = "'exec {}'".format(cmd)
    log.info("Executing: %s\n", exe)
    return subprocess.call(['/bin/bash', '-c', exe], env=environ)


def load_test_config(directory):
    test_cfg = directory + "/test.json"
    if os.path.isfile(test_cfg) is not True:
        return None
    json_data = open(test_cfg).read()
    return json.loads(json_data)


def exec_test(binary, source_dir):
    os.chdir(source_dir)
    test_env = test_environ()

    cfg = load_test_config(options.directory)
    if cfg is None:
        return bash_command(binary, test_env)

    if cfg.has_key("tmp_home"):
        dirpath = tempfile.mkdtemp()
        logger.info("Executing test in tmp dir %s" % dirpath)
        os.chdir(path)
        test_env["HOME"]=dirpath

    cmd = ' '.join([options.binary] + cfg["args"])
    logger.info("Executing test: %s" % cmd)
    return bash_command(cmd, test_env)

def main():
    parser = generate_options()
    options, program_options = parser.parse_known_args()

    if not options.binary:
        parser.print_help()
        raise Exception("Missing binary")
    if not options.directory:
        parser.print_help()
        raise Exception("Missing source directory")

    exec_test(options.binary, options.directory)

if __name__ == '__main__':
    main()
