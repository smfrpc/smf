#!/usr/bin/env python

import os
import sys
import subprocess
import datetime
import logging
import logging.handlers

fmt_string = 'smf::fmt %(levelname)s:%(asctime)s %(filename)s:%(lineno)d] %(message)s'
logging.basicConfig(format=fmt_string)
formatter = logging.Formatter(fmt_string)

for h in logging.getLogger().handlers:
    h.setFormatter(formatter)

logger = logging.getLogger('smf::fmt')
logger.setLevel(logging.INFO)


def get_git_root():
    ret = str(
        subprocess.check_output("git rev-parse --show-toplevel", shell=True))
    assert ret is not None, "Failed getting git root"
    return "".join(ret.split())


def get_git_user():
    ret = str(subprocess.check_output("git config user.name", shell=True))
    assert ret is not None, "Failed getting git user"
    return "".join(ret.split("\n"))


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


def get_cpplint():
    cmd = "%s/%s" % (get_git_root(), "src/third_party/bin/cpplint.py")
    assert os.path.exists(cmd)
    return cmd


def get_clang_format():
    ret = str(subprocess.check_output("which clang-format", shell=True))
    assert ret is not None, "Failed getting clang-format binary"
    return "".join(ret.split())


def get_git_files():
    ret = str(
        subprocess.check_output(
            "cd %s && git ls-files" % get_git_root(), shell=True))
    assert ret is not None, "Failed getting files tracked by git"
    return ret.split("\n")


def is_clang_fmt_file(filename):
    for ext in [".cc", ".cpp", ".h", ".hpp", ".proto", ".java", ".js"]:
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

    return hdr % (comment_char(), datetime.date.today().year, get_git_user(),
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
                f.write("%s\n" % line1)
                f.write(get_legal_header(filename))
                f.write(content)


def insert_legal(filename):
    double_slash_legal(filename)
    hash_legal(filename)


def cpplint_process(cmd, filename):
    """Has to have a special process since it just
   prints sutff to stdout willy nilly"""
    lint = subprocess.check_output("%s %s" % (cmd, filename), shell=True)
    if "Total errors found: 0" not in lint:
        logger.info("Error: %s" % lint)
        sys.exit(1)


def main():
    files = get_git_files()
    clang_fmt = get_clang_format()
    cpplint = get_cpplint()
    root = get_git_root()
    logger.info("Git root: %s" % root)
    logger.info("Formatting %s files" % len(files))
    logger.info("Clang format: %s" % clang_fmt)
    logger.info("CPPLINT: %s" % cpplint)

    for f in files:
        f = "%s/%s" % (root, f)
        logger.debug("Formatting file: %s" % f)
        sys.stdout.write(".")
        insert_legal(f)
        try:
            if is_clang_fmt_file(f):
                run_subprocess("%s -i %s" % (clang_fmt, f))
                cpplint_process("%s --verbose=5 --counting=detailed" % cpplint,
                                f)
        except Exception as e:
            logger.exception("Error %s, file: %s" % (e, f))
            sys.exit(1)

    sys.stdout.write("\nDone\n")

if __name__ == '__main__':
    main()
