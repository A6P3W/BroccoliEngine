"""Shared filesystem and process helpers for the build commands."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


def EnsureDirectory(Directory: Path) -> None:
    Directory.mkdir(parents=True, exist_ok=True)


def RequireFile(FilePath: Path, Description: str) -> None:
    if not FilePath.is_file():
        raise ValueError(f"{Description} does not exist: {FilePath}")


def RequireDirectory(Directory: Path, Description: str) -> None:
    if not Directory.is_dir():
        raise ValueError(f"{Description} does not exist: {Directory}")


def RemovePath(PathValue: Path) -> None:
    if not PathValue.exists():
        return

    try:
        if PathValue.is_dir():
            shutil.rmtree(PathValue)
        else:
            PathValue.unlink()
    except OSError as Error:
        raise RuntimeError(
            f"Failed to remove '{PathValue}'. Close any process using this path and try again: {Error}"
        ) from Error


def CopyFile(Source: Path, DestinationDirectory: Path) -> None:
    RequireFile(Source, "Required file")
    EnsureDirectory(DestinationDirectory)
    try:
        shutil.copy2(Source, DestinationDirectory / Source.name)
    except OSError as Error:
        raise RuntimeError(
            f"Failed to copy '{Source}' to '{DestinationDirectory}': {Error}"
        ) from Error


def CopyDirectory(Source: Path, Destination: Path) -> None:
    RequireDirectory(Source, "Required directory")
    try:
        shutil.copytree(Source, Destination, dirs_exist_ok=True)
    except OSError as Error:
        raise RuntimeError(
            f"Failed to copy '{Source}' to '{Destination}': {Error}"
        ) from Error


def ConvertLevels(
    ConvertLevelsScript: Path, ResourcesDirectory: Path, Clean: bool = False
) -> None:
    RequireFile(ConvertLevelsScript, "ConvertLevels.py")
    RequireDirectory(ResourcesDirectory, "Resources directory")
    Command = [
        sys.executable,
        str(ConvertLevelsScript),
        "--input",
        str(ResourcesDirectory),
        "--output",
        str(ResourcesDirectory),
        "--force",
        "--verify",
        "--delete-source",
    ]
    if Clean:
        Command.append("--clean")
    subprocess.run(Command, check=True)
