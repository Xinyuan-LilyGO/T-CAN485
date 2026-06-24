#!/usr/bin/env python3
"""Merge the current ESP-IDF build outputs into one 0x0 flash image.

This script reads the existing build/flasher_args.json file and calls esptool.
It does not invoke idf.py, so it will not trigger a project build.

Default output name format: [firmware-source]_firmware_<YYYYmmddHHMM>.bin.
"""

from __future__ import annotations

import argparse
import json
import re
import shlex
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


PROJECT_DIR = Path(__file__).resolve().parent


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Merge existing ESP-IDF build binaries into one firmware image "
            "flashed at 0x0."
        )
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        type=Path,
        default=PROJECT_DIR / "build",
        help="ESP-IDF build directory. Defaults to this project's build directory.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help=(
            "Output firmware file. Defaults to "
            "[firmware-source]_firmware_<date>.bin in the project directory."
        ),
    )
    parser.add_argument(
        "--name",
        help="Override the firmware source name used in the output file.",
    )
    parser.add_argument(
        "--chip",
        help=(
            "Override chip name. Defaults to flasher_args.json or "
            "project_description.json."
        ),
    )
    parser.add_argument(
        "--flash-offset",
        default="0x0",
        help="Base flash offset for the merged raw image. Defaults to 0x0.",
    )
    parser.add_argument(
        "--fill-flash-size",
        help=(
            "Pad the output to this flash size. Defaults to "
            "flash_settings.flash_size."
        ),
    )
    parser.add_argument(
        "--no-fill",
        action="store_true",
        help="Do not pad the output image to the configured flash size.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the esptool command without running it.",
    )
    return parser.parse_args()


def load_json(path: Path, *, required: bool = True) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as file:
            data = json.load(file)
    except FileNotFoundError:
        if not required:
            return {}
        raise SystemExit(
            f"Missing {path}. Build the project once before merging firmware."
        )
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc

    if not isinstance(data, dict):
        raise SystemExit(f"Invalid {path}: expected a JSON object.")
    return data


def resolve_path(path: Path) -> Path:
    if path.is_absolute():
        return path.resolve()
    return (Path.cwd() / path).resolve()


def safe_name(name: str) -> str:
    name = name.strip().strip("[]")
    name = re.sub(r'[<>:"/\\|?*\x00-\x1F]+', "_", name)
    name = re.sub(r"\s+", "_", name)
    name = re.sub(r"_+", "_", name)
    name = name.strip("._ ")
    return name or "firmware"


def project_dir_from_description(
    build_dir: Path, project_description: dict[str, Any]
) -> Path:
    project_path = project_description.get("project_path")
    if isinstance(project_path, str) and project_path:
        return Path(project_path).resolve()
    return build_dir.parent


def project_name_from_build(
    build_dir: Path,
    flasher_args: dict[str, Any],
    project_description: dict[str, Any],
    override: str | None,
) -> str:
    candidates: list[Any] = [
        override,
        project_description.get("project_name"),
        project_description.get("app_bin"),
        flasher_args.get("app", {}).get("file")
        if isinstance(flasher_args.get("app"), dict)
        else None,
        build_dir.parent.name,
    ]
    for candidate in candidates:
        if not isinstance(candidate, str) or not candidate:
            continue
        if candidate.endswith(".bin"):
            candidate = Path(candidate).stem
        return safe_name(candidate)
    return "firmware"


def flash_files_from_args(flasher_args: dict[str, Any]) -> list[tuple[str, str]]:
    flash_files = flasher_args.get("flash_files", {})
    if not isinstance(flash_files, dict) or not flash_files:
        raise SystemExit(
            "Invalid flasher_args.json: flash_files must be a non-empty "
            "object."
        )

    def offset_value(item: tuple[str, Any]) -> int:
        return int(str(item[0]), 0)

    files: list[tuple[str, str]] = []
    for offset, file_name in sorted(flash_files.items(), key=offset_value):
        files.append((str(offset), str(file_name)))
    return files


def default_output_name(project_name: str) -> str:
    timestamp = datetime.now().strftime("%Y%m%d%H%M")
    return f"[{project_name}]_firmware_{timestamp}.bin"


def format_command(command: list[str]) -> str:
    if sys.platform == "win32":
        return subprocess.list2cmdline(command)
    return " ".join(shlex.quote(part) for part in command)


def validate_build_files(
    build_dir: Path, flash_files: list[tuple[str, str]]
) -> None:
    missing_files: list[str] = []
    for offset, file_name in flash_files:
        file_path = build_dir / str(file_name)
        if not file_path.is_file():
            missing_files.append(f"  {offset}: {file_path}")

    if missing_files:
        missing = "\n".join(missing_files)
        raise SystemExit(
            f"Missing build output files:\n{missing}\nBuild the project once "
            "before merging firmware."
        )


def esptool_supports_modern_merge() -> bool:
    try:
        result = subprocess.run(
            [sys.executable, "-m", "esptool", "merge-bin", "--help"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
    except FileNotFoundError:
        return False
    return result.returncode == 0


def append_flash_settings(
    command: list[str], flash_settings: dict[str, Any], *, modern: bool
) -> None:
    option_names = {
        "flash_mode": "--flash-mode" if modern else "--flash_mode",
        "flash_freq": "--flash-freq" if modern else "--flash_freq",
        "flash_size": "--flash-size" if modern else "--flash_size",
    }
    for key, option in option_names.items():
        value = flash_settings.get(key)
        if value:
            command.extend([option, str(value)])


def build_esptool_command(
    *,
    chip: str,
    output: Path,
    flash_offset: str,
    fill_flash_size: str | None,
    flash_settings: dict[str, Any],
    flash_files: list[tuple[str, str]],
    modern: bool,
) -> list[str]:
    command = [sys.executable, "-m", "esptool", "--chip", chip]
    if modern:
        command.extend(
            [
                "merge-bin",
                "--output",
                str(output),
                "--format",
                "raw",
                "--target-offset",
                flash_offset,
            ]
        )
        if fill_flash_size:
            command.extend(["--pad-to-size", str(fill_flash_size)])
    else:
        command.extend(
            [
                "merge_bin",
                "-o",
                str(output),
                "-f",
                "raw",
                "-t",
                flash_offset,
            ]
        )
        if fill_flash_size:
            command.extend(["--fill-flash-size", str(fill_flash_size)])

    append_flash_settings(command, flash_settings, modern=modern)
    for offset, file_name in flash_files:
        command.extend([offset, file_name])
    return command


def main() -> int:
    args = parse_args()
    build_dir = resolve_path(args.build_dir)
    flasher_args = load_json(build_dir / "flasher_args.json")
    project_description = load_json(
        build_dir / "project_description.json", required=False
    )
    flash_files = flash_files_from_args(flasher_args)
    validate_build_files(build_dir, flash_files)

    extra_esptool_args = flasher_args.get("extra_esptool_args", {})
    if not isinstance(extra_esptool_args, dict):
        extra_esptool_args = {}
    flash_settings = flasher_args.get("flash_settings", {})
    if not isinstance(flash_settings, dict):
        flash_settings = {}
    chip = args.chip or extra_esptool_args.get("chip")
    chip = chip or project_description.get("target")
    if not chip:
        raise SystemExit(
            "Unable to determine chip. Pass --chip, for example: "
            "--chip esp32p4"
        )
    chip = str(chip)

    project_name = project_name_from_build(
        build_dir, flasher_args, project_description, args.name
    )
    project_dir = project_dir_from_description(build_dir, project_description)
    if args.output is None:
        output = project_dir / default_output_name(project_name)
    else:
        output = resolve_path(args.output)

    fill_flash_size = None
    if not args.no_fill:
        fill_flash_size = args.fill_flash_size or flash_settings.get("flash_size")

    modern_esptool = esptool_supports_modern_merge()
    command = build_esptool_command(
        chip=chip,
        output=output,
        flash_offset=args.flash_offset,
        fill_flash_size=fill_flash_size,
        flash_settings=flash_settings,
        flash_files=flash_files,
        modern=modern_esptool,
    )

    print(f"Build dir: {build_dir}")
    print(f"Project:   {project_name}")
    print(f"Output:    {output}")
    print(f"Chip:      {chip}")
    print(f"Esptool:   {'modern' if modern_esptool else 'legacy'}")
    if fill_flash_size:
        print(f"Fill size: {fill_flash_size}")
    else:
        print("Fill size: disabled")
    print()
    print("Command:")
    print(f"  cd {build_dir}")
    print(f"  {format_command(command)}")

    if args.dry_run:
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    sys.stdout.flush()
    try:
        subprocess.run(command, cwd=build_dir, check=True)
    except FileNotFoundError as exc:
        raise SystemExit(f"Failed to run Python: {exc}") from exc
    except subprocess.CalledProcessError as exc:
        return exc.returncode

    print()
    print(f"Merged firmware created: {output}")
    print(f"Flash this file at offset: {args.flash_offset}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
