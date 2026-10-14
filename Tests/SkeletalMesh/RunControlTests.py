"""Run API checks against a Debug instance started with broccoli.bat run Debug --control."""
import argparse
import json
from pathlib import Path
import subprocess
import time

from GenerateFixtures import Generate


Root = Path(__file__).resolve().parents[2]


def Control(Arguments, ProcessId):
    Command = [str(Root / "broccoli.bat"), "control"]
    if ProcessId is not None:
        Command += ["--pid", str(ProcessId)]
    Completed = subprocess.run(
        Command + Arguments, cwd=Root, capture_output=True, text=True, encoding="utf-8")
    if Completed.returncode:
        raise RuntimeError(Completed.stderr or Completed.stdout)
    return json.loads(Completed.stdout)


def Main():
    Parser = argparse.ArgumentParser(description=__doc__)
    Parser.add_argument("--pid", type=int)
    Parser.add_argument("--output", type=Path, default=Root / ".tmp/skeletal-results.json")
    Options = Parser.parse_args()
    Configuration = (Root / "Intermediate/LastBuildConfiguration.txt").read_text(
        encoding="utf-8").strip()
    if Configuration != "Debug":
        raise RuntimeError("Build Debug last before running these Debug control tests")
    Generate(Root / "Publish/Debug/Resources/Launcher/SkeletalFixtures")
    Deadline = time.monotonic() + 10
    while True:
        try:
            if Control(["state"], Options.pid)["worldAvailable"]:
                break
        except RuntimeError:
            if time.monotonic() >= Deadline:
                raise
        if time.monotonic() >= Deadline:
            raise RuntimeError("Debug world did not become ready")
        time.sleep(0.1)
    Control(["system-call", "open_level_by_path", "--arguments",
             json.dumps({"levelPath": "Game/Animation3DTest.BLevel"})], Options.pid)
    Deadline = time.monotonic() + 10
    while True:
        Actors = Control(["actors"], Options.pid)
        Foxes = [Actor for Actor in Actors["actors"]
                 if Actor["className"] == "AAnimation3DTestActor"]
        if len(Foxes) == 3:
            break
        if time.monotonic() >= Deadline:
            raise RuntimeError("Animation3DTest did not create three Fox actors")
        time.sleep(0.1)
    Report = {"actors": Actors, "playback": {}}
    for Actor in Foxes:
        Report["playback"][Actor["instanceName"]] = Control(
            ["call", str(Actor["actorId"]), "get_status"], Options.pid)["result"]
    ActorId = str(Foxes[0]["actorId"])
    Report["api"] = Control(["call", ActorId, "run_tests"], Options.pid)["result"]
    Report["fixtures"] = Control(
        ["call", ActorId, "test_fixtures", "--arguments",
         json.dumps({"directory": "/Game/SkeletalFixtures"})], Options.pid)["result"]
    Expected = {"FoxSurvey": 1.0, "FoxWalk": 0.75, "FoxRun": 1.5}
    Report["three_actor_playback"] = all(
        Report["playback"][Name]["playing"] and
        Report["playback"][Name]["speed"] == Speed
        for Name, Speed in Expected.items())
    Report["lifecycle"] = []
    for Index in range(12):
        Spawn = Control(
            ["spawn-actor", "AAnimation3DTestActor", "--name", f"FoxLifecycle{Index}",
             "--scale", "0.02"], Options.pid)
        SpawnId = str(Spawn["actorId"])
        State = Control(["call", SpawnId, "get_status"], Options.pid)["result"]
        Destroy = Control(["destroy-actor", SpawnId], Options.pid)
        Report["lifecycle"].append({"spawn": Spawn, "state": State, "destroy": Destroy})
        if not State["playing"] or State["names"] != ["Survey", "Walk", "Run"]:
            raise RuntimeError(f"Actor lifecycle check failed at iteration {Index}")
    if not Report["three_actor_playback"]:
        raise RuntimeError("The three Fox actors did not have the configured playback speeds")
    Options.output.parent.mkdir(parents=True, exist_ok=True)
    Options.output.write_text(
        json.dumps(Report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8", newline="\n")
    Failures = [Name for Section in ("api", "fixtures")
                for Name, Passed in Report[Section].items() if Passed is not True]
    if Failures:
        raise SystemExit("Failed: " + ", ".join(Failures))
    Count = len(Report["api"]) + len(Report["fixtures"])
    print(f"{Count} API checks and 12 actor lifecycle cycles passed; report: {Options.output}")


if __name__ == "__main__":
    Main()
