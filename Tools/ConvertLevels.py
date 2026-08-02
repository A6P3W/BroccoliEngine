#!/usr/bin/env python3
"""Convert between Broccoli .BLevel.json source files and encrypted .BLevel files."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


CRYPTO_KEY = b"BroccoliEngine_Simple_Key_2026!"


def XorTransform(Data: bytes) -> bytes:
    return bytes(Value ^ CRYPTO_KEY[Index % len(CRYPTO_KEY)] for Index, Value in enumerate(Data))


def ReadAndValidateJson(InputPath: Path) -> bytes:
    Data = InputPath.read_bytes()
    try:
        json.loads(Data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as Error:
        raise ValueError(f"Invalid JSON: {InputPath}: {Error}") from Error
    return Data


def DecodeAndValidateBLevel(InputPath: Path) -> object:
    try:
        return json.loads(XorTransform(InputPath.read_bytes()).decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as Error:
        raise ValueError(f"Invalid BLevel: {InputPath}: {Error}") from Error


def VerifyBLevelOutput(OutputPath: Path) -> None:
    DecodeAndValidateBLevel(OutputPath)


def VerifyJsonOutput(OutputPath: Path) -> None:
    try:
        json.loads(OutputPath.read_bytes().decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as Error:
        raise ValueError(f"Verification failed: {OutputPath}: {Error}") from Error


def FindJsonFiles(InputDirectory: Path) -> list[Path]:
    return sorted(
        PathValue
        for PathValue in InputDirectory.rglob("*")
        if PathValue.is_file() and PathValue.name.lower().endswith(".blevel.json")
    )


def FindBLevelFiles(InputDirectory: Path) -> list[Path]:
    return sorted(
        PathValue
        for PathValue in InputDirectory.rglob("*")
        if PathValue.is_file() and PathValue.suffix.lower() == ".blevel"
    )


def GetBLevelOutputRelativePath(InputRelativePath: Path) -> Path:
    return InputRelativePath.with_suffix("").with_suffix(".BLevel")


def GetJsonOutputRelativePath(InputRelativePath: Path) -> Path:
    return InputRelativePath.with_suffix(".BLevel.json")


def CleanOutput(OutputDirectory: Path, ExpectedOutputs: set[Path]) -> int:
    RemovedCount = 0
    for PathValue in OutputDirectory.rglob("*"):
        if not PathValue.is_file() or PathValue.suffix.lower() != ".blevel":
            continue
        if PathValue.relative_to(OutputDirectory) in ExpectedOutputs:
            continue
        PathValue.unlink()
        RemovedCount += 1
        print(f"Removed {PathValue}")
    return RemovedCount


def EncryptLevels(Arguments: argparse.Namespace, InputDirectory: Path, OutputDirectory: Path) -> int:
    JsonFiles = FindJsonFiles(InputDirectory)
    ExpectedOutputs = {
        GetBLevelOutputRelativePath(InputPath.relative_to(InputDirectory)) for InputPath in JsonFiles
    }
    OutputDirectory.mkdir(parents=True, exist_ok=True)

    if Arguments.clean:
        CleanOutput(OutputDirectory, ExpectedOutputs)

    ConvertedCount = 0
    SkippedCount = 0
    for InputPath in JsonFiles:
        RelativePath = InputPath.relative_to(InputDirectory)
        OutputPath = OutputDirectory / GetBLevelOutputRelativePath(RelativePath)
        JsonData = ReadAndValidateJson(InputPath)

        if OutputPath.exists() and not Arguments.force:
            SkippedCount += 1
            print(f"Skipped {OutputPath} (use --force to overwrite)")
        else:
            OutputPath.parent.mkdir(parents=True, exist_ok=True)
            OutputPath.write_bytes(XorTransform(JsonData))
            ConvertedCount += 1
            print(f"Converted {InputPath} -> {OutputPath}")

        if Arguments.verify or Arguments.delete_source:
            VerifyBLevelOutput(OutputPath)
        if Arguments.delete_source:
            InputPath.unlink()
            print(f"Removed {InputPath}")

    print(f"Converted: {ConvertedCount}, skipped: {SkippedCount}")
    return 0


def DecryptLevels(Arguments: argparse.Namespace, InputDirectory: Path, OutputDirectory: Path) -> int:
    BLevelFiles = FindBLevelFiles(InputDirectory)
    OutputDirectory.mkdir(parents=True, exist_ok=True)

    ConvertedCount = 0
    SkippedCount = 0
    for InputPath in BLevelFiles:
        RelativePath = InputPath.relative_to(InputDirectory)
        OutputPath = OutputDirectory / GetJsonOutputRelativePath(RelativePath)
        LevelData = DecodeAndValidateBLevel(InputPath)

        if OutputPath.exists() and not Arguments.force:
            SkippedCount += 1
            print(f"Skipped {OutputPath} (use --force to overwrite)")
        else:
            OutputPath.parent.mkdir(parents=True, exist_ok=True)
            OutputPath.write_text(json.dumps(LevelData, indent=2, ensure_ascii=False), encoding="utf-8")
            ConvertedCount += 1
            print(f"Converted {InputPath} -> {OutputPath}")

        if Arguments.verify:
            VerifyJsonOutput(OutputPath)

    print(f"Converted: {ConvertedCount}, skipped: {SkippedCount}")
    return 0


def ParseArguments() -> argparse.Namespace:
    Parser = argparse.ArgumentParser(
        description="Convert between Broccoli .BLevel.json source files and encrypted .BLevel files."
    )
    Parser.add_argument("--input", required=True, type=Path, help="Input directory")
    Parser.add_argument("--output", required=True, type=Path, help="Output directory")
    Parser.add_argument("--decrypt", action="store_true", help="Convert .BLevel files to formatted .BLevel.json files")
    Parser.add_argument("--force", action="store_true", help="Overwrite existing output files")
    Parser.add_argument(
        "--delete-source",
        action="store_true",
        help="Delete each .BLevel.json source after its encrypted output is verified",
    )
    Parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove .BLevel files in the output directory without a matching input JSON file",
    )
    Parser.add_argument(
        "--verify",
        action="store_true",
        help="Validate every converted or existing output file",
    )
    return Parser.parse_args()


def Main() -> int:
    Arguments = ParseArguments()
    InputDirectory = Arguments.input.resolve()
    OutputDirectory = Arguments.output.resolve()

    if not InputDirectory.is_dir():
        raise ValueError(f"Input directory does not exist: {InputDirectory}")
    if Arguments.delete_source and not Arguments.force:
        raise ValueError("--delete-source requires --force.")
    if Arguments.decrypt and Arguments.delete_source:
        raise ValueError("--delete-source is only supported when encrypting .BLevel.json files.")
    if Arguments.decrypt and Arguments.clean:
        raise ValueError("--clean is only supported when encrypting .BLevel.json files.")

    if Arguments.decrypt:
        return DecryptLevels(Arguments, InputDirectory, OutputDirectory)
    return EncryptLevels(Arguments, InputDirectory, OutputDirectory)


if __name__ == "__main__":
    try:
        sys.exit(Main())
    except (OSError, ValueError) as Error:
        print(Error, file=sys.stderr)
        sys.exit(1)