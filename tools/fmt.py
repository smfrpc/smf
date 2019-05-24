#!/usr/bin/env python

import os
import sys
import subprocess
import datetime
import logging
import logging.handlers
import argparse
import distutils.util

fmt_string = 'smf::fmt %(levelname)s:%(asctime)s %(filename)s:%(lineno)d] %(message)s'
logging.basicConfig(format=fmt_string)
formatter = logging.Formatter(fmt_string)

for h in logging.getLogger().handlers:
    h.setFormatter(formatter)

logger = logging.getLogger('smf::fmt')
logger.setLevel(logging.INFO)


def generate_options():
    parser = argparse.ArgumentParser(description='run smf formatter fmt.py')
    parser.add_argument(
        '--log',
        type=str,
        default='INFO',
        help='info,debug, type log levels. i.e: --log=debug')
    parser.add_argument(
        '--tidy',
        type=distutils.util.strtobool,
        default='false',
        help='run formatter with clang-tidy')
    parser.add_argument(
        '--incremental',
        type=distutils.util.strtobool,
        default='false',
        help='run formatter ONLY on last commit')
    return parser


def get_git_root():
    ret = str(
        subprocess.check_output("git rev-parse --show-toplevel", shell=True))
    assert ret is not None, "Failed getting git root"
    return "".join(ret.split())


def get_build_dir_type(d):
    build_dir = "%s/build_%s" % (get_git_root(), d)
    if not os.path.isdir(build_dir): return None
    return build_dir


def get_debug_build_dir():
    return get_build_dir_type("debug")


def get_release_build_dir():
    return get_build_dir_type("release")


def run_subprocess(cmd):
    logger.debug("Executing command: %s" % cmd)
    proc = subprocess.Popen(
        "exec %s" % cmd, stdout=sys.stdout, stderr=sys.stderr, shell=True)
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
        raise subprocess.CalledProcessError(return_code, cmd)


def get_clang_prog(prog):
    """ALL tools for clang should be pinned to one version"""
    CLANG_SOURCE_VERSION = "8.0.0"
    ret = subprocess.check_output("which %s" % prog, shell=True)
    if ret != None:
        original = "".join(str(ret).split())
        if "".join(
                str(
                    subprocess.check_output(
                        "%s --version | grep %s | awk \'{print $3}\'" %
                        (original, CLANG_SOURCE_VERSION),
                        shell=True)).split()) != CLANG_SOURCE_VERSION:
            return None
        return original
    return None


def get_clang_format(options):
    return get_clang_prog("clang-format")


def get_clang_tidy(options):
    if options.tidy == True:
        clang_tidy = get_clang_prog("clang-tidy")
        if not clang_tidy:
            raise Exception(
                "error tried to run clang tidy but binary not found")
        return clang_tidy
    else:
        return None


def get_git_files(options):
    git_args = 'ls-files'
    if options.incremental == True:
        git_args = 'show --pretty="" --name-only'
    ret = str(
        subprocess.check_output(
            "cd %s && git %s" % (get_git_root(), git_args), shell=True))
    assert ret is not None, "Failed getting files tracked by git"

    # Ensure that you keep this up to date w/ files you want to skip
    white_list = ["src/include/smf/rpc_generated.h"]
    return list(filter(lambda x: x not in white_list, ret.split("\n")))


def is_clang_fmt_file(filename):
    for ext in [".cc", ".cpp", ".h", ".hpp", ".proto", ".java", ".js"]:
        if filename.endswith(ext): return True
    return False


def is_clang_tidy_file(filename):
    for ext in [".cc", ".cpp"]:
        if filename.endswith(ext): return True
    return False


def is_double_slash(filename):
    if is_clang_fmt_file(filename): return True
    for ext in [".scala", ".fbs"]:
        if filename.endswith(ext): return True
    return False


def get_legal_header(filename):
    hdr = "%s Copyright %s %s\n%s\n\n"

    def comment_char():
        if is_double_slash(filename): return "//"
        return "#"

    return hdr % (comment_char(), datetime.date.today().year, "SMF Authors",
                  comment_char())


def is_script_file(filename):
    for ext in [".sh", ".bash", ".py"]:
        if filename.endswith(ext): return True
    return False


def double_slash_legal(filename):
    if is_double_slash(filename):
        with open(filename, "r+b") as f:
            last_pos = f.tell()
            line1 = f.readline()
            if "// Copyright" not in line1:
                f.seek(last_pos)
                content = f.read()
                f.seek(0)
                f.write(get_legal_header(filename))
                f.write(content)


def hash_legal(filename):
    if is_script_file(filename):
        with open(filename, "r+b") as f:
            last_pos = f.tell()
            line1 = f.readline()
            content = f.read()
            if "# Copyright" not in content:
                f.seek(0)
                f.write(line1)
                f.write('\n')
                f.write(get_legal_header(filename))
                f.write(content)


def insert_legal(filename):
    double_slash_legal(filename)
    hash_legal(filename)


def main():
    parser = generate_options()
    options, program_options = parser.parse_known_args()
    logger.info("Options --log=%s" % options.log)
    logger.info("Options --tidy=%s" % options.tidy)
    logger.info("Options --incremental=%s" % options.incremental)
    logger.setLevel(getattr(logging, options.log.upper(), None))

    files = get_git_files(options)
    clang_fmt = get_clang_format(options)
    clang_tidy = get_clang_tidy(options)
    root = get_git_root()

    logger.info("Git root: %s" % root)
    logger.info("Formatting %s files" % len(files))
    logger.info("Clang format: %s" % clang_fmt)
    logger.info("Clang tidy: %s" % clang_tidy)

    for f in files:
        f = "%s/%s" % (root, f)
        logger.debug("Formatting file: %s" % f)
        sys.stdout.write(".")
        insert_legal(f)
        try:
            if is_clang_fmt_file(f):
                # fixing code
                if is_clang_tidy_file(f) and clang_tidy != None:
                    run_subprocess("%s -header-filter=.* -fix %s" %
                                   (clang_tidy, f))
                # fixing formatting
                if clang_fmt != None:
                    run_subprocess("%s -i %s" % (clang_fmt, f))

        except Exception as e:
            logger.exception("Error %s, file: %s" % (e, f))
            sys.exit(1)

    sys.stdout.write("\nDone\n")


if __name__ == '__main__':
    main()
