import glob
import os
import re
from subprocess import PIPE
import subprocess
from sys import argv
from typing import NamedTuple

RESET = "\033[0m"
BLACK = "\033[0;30m"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
YELLOW = "\033[0;33m"
BLUE = "\033[0;34m"
CYAN = "\033[0;36m"
WHITE = "\033[0;37m"
info_color = BLUE


def info(msg: str):
    return f"{info_color}{msg}{RESET}"


def info2(msg: str):
    return f"{CYAN}{msg}{RESET}"


def warn(msg: str):
    return f"{YELLOW}{msg}{RESET}"


def error(msg: str):
    return f"{RED}{msg}{RESET}"


def success(msg: str):
    return f"{GREEN}{msg}{RESET}"


class Test(NamedTuple):
    dir: str
    input: str | None
    output: str | None
    error: str | None
    source_file: str


def make_test(dir: str, files: list[str]) -> Test | None:
    relative_dir = os.path.relpath(dir, os.path.dirname(os.path.dirname(__file__)))

    input = None
    output = None
    source_file = None
    error = None
    for file in files:
        path = os.path.join(dir, file)
        if file == "out" or file == "output":
            output = open(path).read()
        if file == "in" or file == "input":
            print(warn("Reading from stdin is not yet supported for Lox"))
            input = open(path).read()
        if file == "error":
            error = open(path).read()
        if os.path.splitext(file)[1] == ".lox":
            if source_file is not None:
                print(
                    warn(
                        f"Cannot have more than one lox file for test. The test in {relative_dir} will be skipped"
                    )
                )
                return None
            source_file = path
    if output is None and error is None:
        print(
            warn(
                f"No output or error file was found. The test in {relative_dir} will be skipped"
            )
        )
        return None
    if source_file is None:
        print(
            warn(f"No lox file was found. The test in {relative_dir} will be skipped")
        )
        return None

    return Test(dir, input, output, error, source_file)


def find_tests() -> dict[str, list[Test]]:
    dirname = os.path.dirname(__file__)
    tests: dict[str, list[Test]] = {}
    for subdir, dirs, files in os.walk(dirname):
        if len(dirs) == 0 and len(files) > 0:
            test = make_test(subdir, files)
            if test is not None:
                group = os.path.relpath(os.path.dirname(subdir), dirname)
                if group in tests:
                    tests[group].append(test)
                else:
                    tests[group] = [test]

    # Sort the tests because it's prettier
    for group_tests in tests.values():
        group_tests.sort()

    return tests


def run_test(clox_executable: str, test: Test) -> tuple[bool, str, str]:
    proc = subprocess.Popen(
        [clox_executable, f"{test.source_file}"],
        stdin=PIPE,
        stdout=PIPE,
        stderr=PIPE,
        text=True,
    )
    stdout, stderr = proc.communicate(test.input)
    result = False
    if test.output is not None:
        result = stdout == test.output
    if test.error is not None:
        result = stderr == test.error

    return result, stdout, stderr


def search_clox(glob_pattern: str) -> str | None:
    print(f"{info2(f'Searching in {glob_pattern}')}. ", end="")
    matches = glob.glob(glob_pattern)
    if len(matches) == 1:
        print(f"{success(f'Found executable at {matches[0]}')}.")
        return matches[0]
    elif len(matches) > 1:
        print(f"{info2('Found multiple matches. ')}", end="")
        for match in matches:
            if "release" in match:
                print(f"{success(f'Taking {match}')}.")
                return match
        print(f"{warn(f'Could not find a release executable, taking {matches[0]}')}.")
        return matches[0]
    else:
        print("")
    return None


def try_get_clox_executable_path() -> str:
    clox_executable = None
    if len(argv) == 2:
        clox_executable = argv[1]
    else:
        print(f"{info2('clox executable path not specified')}")
        glob_patterns = ["./**/clox", "../build/**/clox", "../**/clox"]
        for pattern in glob_patterns:
            res = search_clox(pattern)
            if res is not None:
                clox_executable = res
                break
        if clox_executable is None:
            print(f"{error('Could not find clox executable')}")
            exit(1)
    return clox_executable


def get_diff_str(expected: str, found: str):
    diff: list[str] = []
    previous_was_wrong = False
    for index, char in enumerate(found):
        if index >= len(expected):
            if not previous_was_wrong:
                diff += RED
                previous_was_wrong = True
        else:
            if char != expected[index] and not previous_was_wrong:
                diff += RED
                previous_was_wrong = True
            elif char == expected[index] and previous_was_wrong:
                diff += RESET
                previous_was_wrong = False
        diff += char
    diff += RESET
    return "".join(diff)


def compare_outputs(expected: str | None, found: str, name: str):
    if expected is not None:
        print(f"    {error(f'Expected ({name})')}:")
        print(f"{expected}", end="")
        if len(found) > 0:
            print(f"    {info2(f'Found ({name})')}:")
            print(get_diff_str(expected, found), end="")
        elif len(expected) > 0:
            print(f"    {warn(f'Found nothing in {name}')}")
    if expected is None and len(found) > 0:
        print(f"    {error(f'Expected nothing ({name})')}")
        print(f"    {info2(f'Found ({name})')}:")
        print(f"{found}")


def run_tests(tests: dict[str, list[Test]], clox_executable: str):
    total = 0
    total_passes = 0
    total_fails = 0
    for group, group_tests in sorted(tests.items(), key=lambda item: item[0]):
        passes = fails = 0
        print(f"{info(f"Group '{group}'")}:")
        for test in group_tests:
            test_relative = os.path.relpath(test.dir, group)
            result, stdout, stderr = run_test(clox_executable, test)
            if result:
                print(f"  {success(f"Test '{test_relative}' passed")}")
                passes += 1
            else:
                print(f"  {error(f"Test '{test_relative}' failed")}")
                compare_outputs(test.output, stdout, "stdout")
                compare_outputs(test.error, stderr, "stdout")
                fails += 1
        subtotal = passes + fails
        total += subtotal
        total_passes += passes
        total_fails += fails
        print(
            f"{info(f"Group '{group}'")}: {success(f'{passes}/{subtotal} passed')} {', ' + error(f'{fails}/{subtotal} failed') if fails > 0 else ''}"
        )
    print()
    print(
        f"{info('Summary')}: {success(f'{total_passes}/{total} passed')} {', ' + error(f'{total_fails}/{total} failed') if total_fails > 0 else ''}"
    )


def main():
    if len(argv) > 2:
        print(f"Usage: {os.path.relpath(__file__, os.getcwd())} [clox_executable_path]")
        exit(1)

    term = os.getenv("TERM")
    if term is not None:
        if re.search(r"-256(color)?$", term, re.IGNORECASE):
            global info_color
            info_color = "\033[38;5;253m"

    clox_executable = try_get_clox_executable_path()
    tests = find_tests()
    run_tests(tests, clox_executable)


if __name__ == "__main__":
    main()
