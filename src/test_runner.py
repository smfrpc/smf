#!/usr/bin/env python3

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
import random
import string
from string import Template

fmt_string = 'TESTRUNNER %(levelname)s:%(asctime)s %(filename)s:%(lineno)d] %(message)s'
logging.basicConfig(format=fmt_string)
formatter = logging.Formatter(fmt_string)
for h in logging.getLogger().handlers:
    h.setFormatter(formatter)
logger = logging.getLogger('it.test')
# Set to logging.DEBUG to see more info about tests
logger.setLevel(logging.INFO)

KTEST_DIR_PREFIX = "smf_test_"


def gen_alphanum(x=16):
    return ''.join(
        random.choice(string.ascii_letters + string.digits) for _ in range(x))


def generate_options():
    parser = argparse.ArgumentParser(description='run smf integration tests')
    parser.add_argument('--binary', type=str, help='binary program to run')
    parser.add_argument(
        '--directory',
        type=str,
        help='source directory of binary. needed for metadata')
    parser.add_argument(
        '--test_type',
        type=str,
        default="unit",
        help='either integration or unit. ie: --test_type unit')
    return parser


def test_environ():
    e = os.environ.copy()
    ld_path = ""
    if "LD_LIBRARY_PATH" in e:
        ld_path = e["LD_LIBRARY_PATH"]
    e["GLOG_logtostderr"] = '1'
    e["GLOG_v"] = '1'
    e["GLOG_vmodule"] = ''
    e["GLOG_logbufsecs"] = '0'
    e["GLOG_log_dir"] = '.'
    e["GTEST_COLOR"] = 'no'
    e["GTEST_SHUFFLE"] = '1'
    e["GTEST_BREAK_ON_FAILURE"] = '1'
    e["GTEST_REPEAT"] = '1'
    return e


def run_subprocess(cmd, cfg, environ):
    logger.info("\nTest: {}\nConfig: {}".format(cmd, cfg))
    if not os.path.exists(cfg["execution_directory"]):
        raise Exception("Test directory does not exist: {}".format(
            cfg["execution_directory"]))

    os.chdir(cfg["execution_directory"])
    hydrated_cmd = Template(cmd).safe_substitute(cfg)
    logger.info("Hydrated cmd:\n%s\n" % hydrated_cmd)
    proc = subprocess.Popen(
        "exec %s" % hydrated_cmd,
        stdout=sys.stdout,
        stderr=sys.stderr,
        env=environ,
        cwd=cfg["execution_directory"],
        shell=True)
    return_code = 0
    try:
        return_code = proc.wait()
        sys.stdout.flush()
        sys.stderr.flush()
    except Exception as e:
        logger.exception("Could not run command: % ", e)
        proc.kill()
        raise

    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, hydrated_cmd)


def set_up_test_environment(cfg):
    test_env = test_environ()
    dirpath = os.getcwd()
    if "tmp_home" in cfg:
        dirpath = tempfile.mkdtemp(
            suffix=gen_alphanum(), prefix=KTEST_DIR_PREFIX)
        logger.debug("Executing test in tmp dir %s" % dirpath)
        os.chdir(dirpath)
        test_env["HOME"] = dirpath
    if "copy_files" in cfg:
        files_to_copy = cfg["copy_files"]
        if isinstance(files_to_copy, list):
            for f in files_to_copy:
                ff = cfg["source_directory"] + "/" + f
                for glob_file in glob.glob(ff):
                    shutil.copy(glob_file, dirpath)
    cfg["execution_directory"] = dirpath
    return test_env


def clean_test_resources(cfg):
    if "execution_directory" in cfg:
        exec_dir = cfg["execution_directory"]
        if KTEST_DIR_PREFIX in exec_dir:
            if "remove_test_dir" in cfg and \
               cfg["remove_test_dir"] is False:
                logger.info("Skipping rm -r tmp dir: %s" % exec_dir)
            else:
                logger.debug("Removing tmp dir: %s" % exec_dir)
                shutil.rmtree(exec_dir)


def load_test_configuration(directory):
    try:
        test_cfg = directory + "/test.json"
        if os.path.isfile(test_cfg) is not True: return {}
        json_data = open(test_cfg).read()
        ret = json.loads(json_data)
        ret["source_directory"] = directory
        return ret
    except Exception as e:
        logger.exception("Could not load test configuration %s" % e)
        raise


def get_full_executable(binary, cfg):
    cmd = binary
    if "args" in cfg:
        cmd = ' '.join([cmd] + cfg["args"])
    return cmd


def execute(cmd, test_env, cfg):
    run_subprocess(cmd, cfg, test_env)
    if all(k in cfg for k in ["repeat_in_same_dir", "repeat_times"]):
        # substract one from the already executed test
        repeat = int(cfg["repeat_times"]) - 1
        while repeat > 0:
            logger.debug("Repeating test: %s, %s more time(s)", cmd, repeat)
            repeat = repeat - 1
            run_subprocess(cmd, cfg, test_env)


def prepare_test(binary, source_dir):
    cfg = load_test_configuration(source_dir)
    test_env = set_up_test_environment(cfg)
    cmd = get_full_executable(binary, cfg)
    return (cmd, test_env, cfg)


def main():
    parser = generate_options()
    options, program_options = parser.parse_known_args()

    if not options.binary:
        parser.print_help()
        raise Exception("Missing binary")
    if not options.directory:
        parser.print_help()
        raise Exception("Missing source directory")
    if not options.test_type:
        if (options.test_type is not "unit"
                or options.test_type is not "integration"
                or options.test_type is not "benchmark"):
            parser.print_help()
            raise Exception("Missing test_type ")

    (cmd, env, cfg) = prepare_test(options.binary, options.directory)
    execute(cmd, env, cfg)
    clean_test_resources(cfg)


if __name__ == '__main__':
    main()
