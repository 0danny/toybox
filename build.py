import sys
sys.dont_write_bytecode = True

import os
import subprocess
import argparse

# Constants
PROJECT_NAME = "toybox"
BUILD_FOLDER = "build"


def track_process(command, custom_name=""):
    proc = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )

    name = custom_name or command[0]

    for line in proc.stdout:
        print(f"[{name}]: {line.strip()}")

    proc.wait()

    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, command)


def copy_assets():
    # Placeholder for asset copy logic
    print("[build]: Assets have been copied to build folder.")


def get_executable_name():
    return f"{PROJECT_NAME}.exe" if os.name == "nt" else f"./{PROJECT_NAME}"


def run_build(launch_app, build_type):
    print(f"[build]: Running build for {PROJECT_NAME} with type {build_type}")

    os.makedirs(BUILD_FOLDER, exist_ok=True)
    os.chdir(BUILD_FOLDER)

    # Configure Ninja project
    track_process([
        "cmake",
        "..",
        "-G", "Ninja",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ])

    # Build with Ninja
    track_process(["ninja"])

    copy_assets()

    if launch_app:
        exe_path = os.path.join(
            os.getcwd(),
            get_executable_name()
        )

        print(f"----------------- Starting {PROJECT_NAME} -----------------")
        subprocess.run([exe_path])


def main():
    parser = argparse.ArgumentParser(
        description=f"Build script for {PROJECT_NAME}"
    )

    parser.add_argument(
        "--nl",
        action="store_true",
        help="Don't launch the app after build"
    )

    parser.add_argument(
        "--bt",
        choices=[
            "Debug",
            "Release",
            "RelWithDebInfo",
            "MinSizeRel"
        ],
        default="RelWithDebInfo",
        help="CMake build type (default: RelWithDebInfo)"
    )

    args = parser.parse_args()

    run_build(
        launch_app=not args.nl,
        build_type=args.bt
    )


if __name__ == "__main__":
    main()