"""Generate a BroccoliEngine game project or reconfigure this repository in place."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import stat
import subprocess
import tempfile
from collections.abc import Callable
from enum import IntEnum
from pathlib import Path

PROJECT_NAME_PATTERN = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
TEMPLATE_SUFFIX = ".template"


class SetupError(RuntimeError):
    """Raised when project setup cannot complete safely."""


class SetupState(IntEnum):
    INITIAL = 0
    VALIDATED = 1
    TEMPLATES_PREPARED = 2
    ENGINE_MOVED = 3
    ROOT_INITIALIZED = 4
    SUBMODULE_REGISTERED = 5
    COMPLETED = 6


FailureHook = Callable[[SetupState], None]


def ValidateProjectName(ProjectName: str) -> None:
    if not PROJECT_NAME_PATTERN.fullmatch(ProjectName):
        raise SetupError(
            "Project name must start with an ASCII letter and contain only "
            "ASCII letters, digits, or underscores."
        )
    if ProjectName.casefold() == "broccoliengine":
        raise SetupError("Project name must not be BroccoliEngine.")


def RunGit(
    WorkingDirectory: Path,
    *Arguments: str,
    Check: bool = True,
) -> subprocess.CompletedProcess[str]:
    Result = subprocess.run(
        ["git", *Arguments],
        cwd=WorkingDirectory,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    if Check and Result.returncode != 0:
        Details = Result.stderr.strip() or Result.stdout.strip()
        raise SetupError(f"git {' '.join(Arguments)} failed: {Details}")
    return Result


def TemplateDestination(RelativePath: Path, ProjectName: str) -> Path:
    RenderedPath = Path(
        *(Part.replace("{PROJECT_NAME}", ProjectName) for Part in RelativePath.parts)
    )
    Name = RenderedPath.name
    if Name == "gitignore.template":
        return RenderedPath.with_name(".gitignore")
    if Name.endswith(TEMPLATE_SUFFIX):
        return RenderedPath.with_name(Name[: -len(TEMPLATE_SUFFIX)])
    return RenderedPath


def RenderTemplates(
    TemplateRoot: Path,
    OutputRoot: Path,
    ProjectName: str,
) -> list[Path]:
    ValidateProjectName(ProjectName)
    if not TemplateRoot.is_dir():
        raise SetupError(f"Template directory was not found: {TemplateRoot}")

    TemplateFiles = sorted(
        PathValue for PathValue in TemplateRoot.rglob("*") if PathValue.is_file()
    )
    if not TemplateFiles:
        raise SetupError(f"Template directory is empty: {TemplateRoot}")

    Destinations = [
        OutputRoot / TemplateDestination(Source.relative_to(TemplateRoot), ProjectName)
        for Source in TemplateFiles
    ]
    ExistingPaths = [PathValue for PathValue in Destinations if PathValue.exists()]
    if ExistingPaths:
        Paths = "\n".join(str(PathValue) for PathValue in ExistingPaths)
        raise SetupError(f"Refusing to overwrite existing generated files:\n{Paths}")

    CreatedPaths: list[Path] = []
    try:
        for Source, Destination in zip(TemplateFiles, Destinations, strict=True):
            Destination.parent.mkdir(parents=True, exist_ok=True)
            Content = Source.read_text(encoding="utf-8")
            Rendered = Content.replace("{PROJECT_NAME}", ProjectName)
            Destination.write_text(
                Rendered.replace("\r\n", "\n"),
                encoding="utf-8",
                newline="\n",
            )
            CreatedPaths.append(Destination)
    except Exception:
        RemoveGeneratedPaths(OutputRoot, CreatedPaths)
        raise

    return CreatedPaths


def RemoveGeneratedPaths(OutputRoot: Path, GeneratedPaths: list[Path]) -> None:
    for GeneratedPath in reversed(GeneratedPaths):
        if GeneratedPath.is_file() or GeneratedPath.is_symlink():
            GeneratedPath.unlink(missing_ok=True)

    ParentDirectories = sorted(
        {
            Parent
            for GeneratedPath in GeneratedPaths
            for Parent in GeneratedPath.parents
            if Parent != OutputRoot and OutputRoot in Parent.parents
        },
        key=lambda PathValue: len(PathValue.parts),
        reverse=True,
    )
    for ParentDirectory in ParentDirectories:
        try:
            ParentDirectory.rmdir()
        except OSError:
            pass


def ValidateInPlaceRepository(ProjectRoot: Path, ProjectName: str) -> tuple[str, str]:
    ValidateProjectName(ProjectName)
    if not ProjectRoot.is_dir():
        raise SetupError(f"Project root was not found: {ProjectRoot}")

    TopLevel = RunGit(ProjectRoot, "rev-parse", "--show-toplevel").stdout.strip()
    if Path(TopLevel).resolve() != ProjectRoot.resolve():
        raise SetupError("The current directory must be the Git repository root.")

    OriginUrl = RunGit(ProjectRoot, "remote", "get-url", "origin").stdout.strip()
    HeadCommit = RunGit(ProjectRoot, "rev-parse", "HEAD").stdout.strip()
    if not OriginUrl:
        raise SetupError("The origin remote URL is empty.")
    if not HeadCommit:
        raise SetupError("HEAD could not be resolved.")
    if RunGit(ProjectRoot, "status", "--porcelain").stdout.strip():
        raise SetupError("The repository has uncommitted changes.")

    for Name in ("BroccoliEngine", ProjectName):
        if (ProjectRoot / Name).exists():
            raise SetupError(f"{Name}/ already exists.")

    TemplateRoot = ProjectRoot / "ProjectTemplates" / "Default"
    if not TemplateRoot.is_dir():
        raise SetupError(f"Template directory was not found: {TemplateRoot}")

    return OriginUrl, HeadCommit


def NotifyState(State: SetupState, Hook: FailureHook | None) -> None:
    if Hook is not None:
        Hook(State)


def RestoreAbsorbedGitDirectory(ProjectRoot: Path, EngineRoot: Path) -> None:
    EngineGit = EngineRoot / ".git"
    ModuleGit = ProjectRoot / ".git" / "modules" / "BroccoliEngine"
    if not EngineGit.is_file() or not ModuleGit.is_dir():
        return

    EngineGit.unlink()
    shutil.move(str(ModuleGit), str(EngineGit))
    RunGit(
        EngineRoot,
        "--git-dir",
        str(EngineGit),
        "--work-tree",
        str(EngineRoot),
        "config",
        "--unset",
        "core.worktree",
        Check=False,
    )


def RemoveTree(TreeRoot: Path) -> None:
    def MakeWritableAndRetry(
        Function: Callable[[str], None], PathName: str, ErrorInfo: object
    ) -> None:
        del ErrorInfo
        os.chmod(PathName, stat.S_IWRITE)
        Function(PathName)

    shutil.rmtree(TreeRoot, onerror=MakeWritableAndRetry)


def RollbackInPlace(
    ProjectRoot: Path,
    EngineRoot: Path,
    MovedNames: list[str],
    GeneratedPaths: list[Path],
    State: SetupState,
) -> None:
    RestoreAbsorbedGitDirectory(ProjectRoot, EngineRoot)

    RootGit = ProjectRoot / ".git"
    if State >= SetupState.ROOT_INITIALIZED and RootGit.exists():
        RemoveTree(RootGit)

    GitModules = ProjectRoot / ".gitmodules"
    GitModules.unlink(missing_ok=True)
    RemoveGeneratedPaths(ProjectRoot, GeneratedPaths)

    if EngineRoot.is_dir():
        for Name in reversed(MovedNames):
            Source = EngineRoot / Name
            Destination = ProjectRoot / Name
            if Source.exists():
                shutil.move(str(Source), str(Destination))
        try:
            EngineRoot.rmdir()
        except OSError:
            pass


def VerifySubmodule(ProjectRoot: Path, HeadCommit: str) -> None:
    StageOutput = RunGit(
        ProjectRoot, "ls-files", "--stage", "BroccoliEngine"
    ).stdout.strip()
    Fields = StageOutput.split()
    if len(Fields) < 4 or Fields[0] != "160000" or Fields[1] != HeadCommit:
        raise SetupError(
            "BroccoliEngine was not registered as the expected gitlink: "
            f"{StageOutput or '<empty>'}"
        )

    StatusOutput = RunGit(
        ProjectRoot, "submodule", "status", "BroccoliEngine"
    ).stdout.rstrip()
    if not StatusOutput.startswith(" ") or HeadCommit not in StatusOutput:
        raise SetupError(f"Unexpected submodule status: {StatusOutput or '<empty>'}")


def ReconfigureInPlace(
    ProjectRoot: Path,
    ProjectName: str,
    Hook: FailureHook | None = None,
) -> None:
    ProjectRoot = ProjectRoot.resolve()
    State = SetupState.INITIAL
    EngineRoot = ProjectRoot / "BroccoliEngine"
    MovedNames: list[str] = []
    GeneratedPaths: list[Path] = []

    OriginUrl = ""
    HeadCommit = ""
    try:
        OriginUrl, HeadCommit = ValidateInPlaceRepository(ProjectRoot, ProjectName)
        State = SetupState.VALIDATED
        NotifyState(State, Hook)

        with tempfile.TemporaryDirectory(prefix="broccoli-project-") as TemporaryName:
            TemporaryRoot = Path(TemporaryName)
            TemplateRoot = ProjectRoot / "ProjectTemplates" / "Default"
            RenderTemplates(TemplateRoot, TemporaryRoot, ProjectName)
            State = SetupState.TEMPLATES_PREPARED
            NotifyState(State, Hook)

            OriginalEntries = list(ProjectRoot.iterdir())
            EngineRoot.mkdir()
            for Entry in OriginalEntries:
                shutil.move(str(Entry), str(EngineRoot / Entry.name))
                MovedNames.append(Entry.name)
            State = SetupState.ENGINE_MOVED
            NotifyState(State, Hook)

            GeneratedPaths = [
                Destination
                for Source in sorted(
                    PathValue
                    for PathValue in TemporaryRoot.rglob("*")
                    if PathValue.is_file()
                )
                for Destination in [ProjectRoot / Source.relative_to(TemporaryRoot)]
            ]
            for Source in sorted(
                PathValue
                for PathValue in TemporaryRoot.rglob("*")
                if PathValue.is_file()
            ):
                Destination = ProjectRoot / Source.relative_to(TemporaryRoot)
                Destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(Source, Destination)

            RunGit(ProjectRoot, "init")
            State = SetupState.ROOT_INITIALIZED
            NotifyState(State, Hook)

            GitModules = ProjectRoot / ".gitmodules"
            GitModules.write_text("", encoding="utf-8", newline="\n")
            RunGit(
                ProjectRoot,
                "config",
                "-f",
                str(GitModules),
                "submodule.BroccoliEngine.path",
                "BroccoliEngine",
            )
            RunGit(
                ProjectRoot,
                "config",
                "-f",
                str(GitModules),
                "submodule.BroccoliEngine.url",
                OriginUrl,
            )
            RunGit(ProjectRoot, "add", ".gitmodules", "BroccoliEngine")
            RunGit(ProjectRoot, "submodule", "init", "BroccoliEngine")
            VerifySubmodule(ProjectRoot, HeadCommit)
            RunGit(ProjectRoot, "submodule", "absorbgitdirs", "BroccoliEngine")
            VerifySubmodule(ProjectRoot, HeadCommit)
            State = SetupState.SUBMODULE_REGISTERED
            NotifyState(State, Hook)

            Marker = ProjectRoot / ".broccoli-project.json"
            GeneratedPaths.append(Marker)
            Marker.write_text(
                json.dumps(
                    {
                        "schema_version": 1,
                        "project_name": ProjectName,
                        "engine": {
                            "path": "BroccoliEngine",
                            "origin": OriginUrl,
                            "commit": HeadCommit,
                        },
                    },
                    ensure_ascii=False,
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
                newline="\n",
            )
            State = SetupState.COMPLETED
            NotifyState(State, Hook)
    except Exception as Error:
        if EngineRoot.exists() or MovedNames:
            RollbackInPlace(ProjectRoot, EngineRoot, MovedNames, GeneratedPaths, State)
        if isinstance(Error, SetupError):
            raise
        raise SetupError(
            f"Project setup failed during {State.name}: {Error}"
        ) from Error


def LoadProjectMarker(ProjectRoot: Path) -> dict[str, object]:
    Marker = ProjectRoot / ".broccoli-project.json"
    if not Marker.is_file():
        raise SetupError(
            "This directory is not a generated Broccoli project: "
            ".broccoli-project.json was not found."
        )

    try:
        Data = json.loads(Marker.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as Error:
        raise SetupError(f"Could not read the project marker: {Error}") from Error

    if not isinstance(Data, dict) or Data.get("schema_version") != 1:
        raise SetupError("The project marker schema is not supported.")

    ProjectName = Data.get("project_name")
    Engine = Data.get("engine")
    if not isinstance(ProjectName, str):
        raise SetupError("The project marker does not contain a valid project name.")
    ValidateProjectName(ProjectName)
    if not isinstance(Engine, dict) or Engine.get("path") != "BroccoliEngine":
        raise SetupError("The project marker does not describe BroccoliEngine.")

    return Data


def DefaultResetBackupRoot(ProjectRoot: Path) -> Path:
    BaseName = f"{ProjectRoot.name}.broccoli-project-backup"
    Candidate = ProjectRoot.parent / BaseName
    Suffix = 1
    while Candidate.exists():
        Candidate = ProjectRoot.parent / f"{BaseName}-{Suffix}"
        Suffix += 1
    return Candidate


def ValidateResetProject(ProjectRoot: Path, BackupRoot: Path) -> None:
    LoadProjectMarker(ProjectRoot)

    TopLevel = RunGit(ProjectRoot, "rev-parse", "--show-toplevel").stdout.strip()
    if Path(TopLevel).resolve() != ProjectRoot.resolve():
        raise SetupError(
            "The current directory must be the generated Git repository root."
        )

    EngineRoot = ProjectRoot / "BroccoliEngine"
    if not EngineRoot.is_dir():
        raise SetupError("BroccoliEngine/ was not found.")

    StageOutput = RunGit(
        ProjectRoot, "ls-files", "--stage", "BroccoliEngine"
    ).stdout.strip()
    Fields = StageOutput.split()
    if len(Fields) < 4 or Fields[0] != "160000":
        raise SetupError("BroccoliEngine is not registered as a gitlink.")

    EngineGit = EngineRoot / ".git"
    ModuleGit = ProjectRoot / ".git" / "modules" / "BroccoliEngine"
    if not EngineGit.is_file() or not ModuleGit.is_dir():
        raise SetupError(
            "BroccoliEngine does not have an absorbed submodule Git directory."
        )

    ResolvedBackupRoot = BackupRoot.resolve()
    if ResolvedBackupRoot == ProjectRoot or ProjectRoot in ResolvedBackupRoot.parents:
        raise SetupError("The reset backup must be outside the generated project root.")
    if ResolvedBackupRoot.exists():
        raise SetupError(f"Reset backup already exists: {ResolvedBackupRoot}")
    if not ResolvedBackupRoot.parent.is_dir():
        raise SetupError(
            f"Reset backup parent directory was not found: {ResolvedBackupRoot.parent}"
        )


def ReabsorbGitDirectory(ProjectRoot: Path, EngineRoot: Path) -> None:
    EngineGit = EngineRoot / ".git"
    RootGit = ProjectRoot / ".git"
    if EngineGit.is_dir() and RootGit.is_dir():
        RunGit(
            ProjectRoot,
            "submodule",
            "absorbgitdirs",
            "BroccoliEngine",
            Check=False,
        )


def ResetProject(ProjectRoot: Path, BackupRoot: Path | None = None) -> Path:
    ProjectRoot = ProjectRoot.resolve()
    if BackupRoot is None:
        BackupRoot = DefaultResetBackupRoot(ProjectRoot)
    else:
        BackupRoot = BackupRoot.resolve()

    ValidateResetProject(ProjectRoot, BackupRoot)

    EngineRoot = ProjectRoot / "BroccoliEngine"
    BackedUpNames: list[str] = []
    RestoredEngineNames: list[str] = []

    try:
        RestoreAbsorbedGitDirectory(ProjectRoot, EngineRoot)
        BackupRoot.mkdir()

        for Entry in list(ProjectRoot.iterdir()):
            if Entry == EngineRoot:
                continue
            BackedUpNames.append(Entry.name)
            shutil.move(str(Entry), str(BackupRoot / Entry.name))

        for Entry in list(EngineRoot.iterdir()):
            RestoredEngineNames.append(Entry.name)
            shutil.move(str(Entry), str(ProjectRoot / Entry.name))

        EngineRoot.rmdir()
        return BackupRoot
    except Exception as Error:
        if not EngineRoot.exists():
            EngineRoot.mkdir()

        for Name in reversed(RestoredEngineNames):
            Source = ProjectRoot / Name
            if Source.exists():
                shutil.move(str(Source), str(EngineRoot / Name))

        if BackupRoot.is_dir():
            for Name in reversed(BackedUpNames):
                Source = BackupRoot / Name
                if Source.exists():
                    shutil.move(str(Source), str(ProjectRoot / Name))
            try:
                BackupRoot.rmdir()
            except OSError:
                pass

        ReabsorbGitDirectory(ProjectRoot, EngineRoot)
        if isinstance(Error, SetupError):
            raise
        raise SetupError(f"Project reset failed: {Error}") from Error


def ParseArguments() -> argparse.Namespace:
    Parser = argparse.ArgumentParser(description=__doc__)
    Parser.add_argument("--name", help="Project and executable name")
    Mode = Parser.add_mutually_exclusive_group(required=True)
    Mode.add_argument(
        "--output", type=Path, help="Render templates into this directory"
    )
    Mode.add_argument(
        "--in-place",
        action="store_true",
        help="Convert the current clean engine repository into a game workspace",
    )
    Mode.add_argument(
        "--reset-project",
        action="store_true",
        help="Restore BroccoliEngine to the repository root",
    )
    Parser.add_argument(
        "--backup-output",
        type=Path,
        help="Directory that receives the removed game workspace",
    )
    Arguments = Parser.parse_args()
    if not Arguments.reset_project and Arguments.name is None:
        Parser.error("--name is required with --output or --in-place")
    if Arguments.reset_project and Arguments.name is not None:
        Parser.error("--name cannot be used with --reset-project")
    if not Arguments.reset_project and Arguments.backup_output is not None:
        Parser.error("--backup-output requires --reset-project")
    return Arguments


def Main() -> int:
    Arguments = ParseArguments()
    ScriptRoot = Path(__file__).resolve().parent
    try:
        if Arguments.reset_project:
            BackupRoot = ResetProject(Path.cwd(), Arguments.backup_output)
            print(f"Restored BroccoliEngine repository: {Path.cwd()}")
            print(f"Game workspace backup: {BackupRoot}")
        elif Arguments.output is not None:
            OutputRoot = Arguments.output.resolve()
            RenderTemplates(
                ScriptRoot / "ProjectTemplates" / "Default",
                OutputRoot,
                Arguments.name,
            )
            print(f"Generated project templates: {OutputRoot}")
            print(
                "Please specify the path to your toolchain file in CMakeUserPresets.json"
            )
        else:
            ReconfigureInPlace(Path.cwd(), Arguments.name)
            print(f"Reconfigured project: {Path.cwd()}")
            print(
                "Please specify the path to your toolchain file in CMakeUserPresets.json"
            )
    except SetupError as Error:
        print(f"[error] {Error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(Main())
