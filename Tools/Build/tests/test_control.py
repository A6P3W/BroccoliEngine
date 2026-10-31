from __future__ import annotations

import json

from broccoli_build import cli, control


class FakeResult:
  def __init__(Self, Value: dict[str, object]) -> None:
    Self.Value = Value

  def to_dict(Self) -> dict[str, object]:
    return Self.Value


class FakeControlClient:
  def __init__(Self) -> None:
    Self.Calls: list[tuple[str, tuple[object, ...], dict[str, object]]] = []

  def get_actors(Self) -> FakeResult:
    Self.Calls.append(("get_actors", (), {}))
    return FakeResult({"actors": []})

  def spawn_actor(Self, ClassName: str, **Arguments: object) -> FakeResult:
    Self.Calls.append(("spawn_actor", (ClassName,), Arguments))
    return FakeResult({"actorId": 7})

  def invoke_actor_method(
    Self, ActorId: int, MethodName: str, *, Arguments: dict[str, object]
  ) -> FakeResult:
    Self.Calls.append(("invoke_actor_method", (ActorId, MethodName), {"Arguments": Arguments}))
    return FakeResult({"actorId": ActorId, "methodName": MethodName})

  def stop_engine_update(Self) -> FakeResult:
    Self.Calls.append(("stop_engine_update", (), {}))
    return FakeResult({"operationId": "8", "status": "completed"})

  def start_engine_update(Self) -> FakeResult:
    Self.Calls.append(("start_engine_update", (), {}))
    return FakeResult({"engineUpdate": "running"})

  def step_engine_update(Self, Count: int) -> FakeResult:
    Self.Calls.append(("step_engine_update", (Count,), {}))
    return FakeResult({"operationId": "9", "requestedCount": Count})

  def get_operation_status(Self, OperationId: str) -> FakeResult:
    Self.Calls.append(("get_operation_status", (OperationId,), {}))
    return FakeResult({"operationId": OperationId, "status": "completed"})


def TestSpawnActorDispatchesOnlyThroughControlClient() -> None:
  Client = FakeControlClient()
  Arguments = control.CreateParser().parse_args(
    ["spawn-actor", "ADoorActor", "--x", "12", "--name", "Door01"]
  )

  Result = control.Dispatch(Client, Arguments)  # type: ignore[arg-type]

  assert Result == {"actorId": 7}
  assert Client.Calls == [
    (
      "spawn_actor",
      ("ADoorActor",),
      {
        "Location": None,
        "Rotation": None,
        "Scale": None,
        "LocationX": 12.0,
        "LocationY": None,
        "LocationZ": None,
        "Pitch": None,
        "Yaw": None,
        "Roll": None,
        "ScaleX": None,
        "ScaleY": None,
        "ScaleZ": None,
        "InstanceName": "Door01",
      },
    )
  ]


def TestCallDispatchesJsonArguments() -> None:
  Client = FakeControlClient()
  Arguments = control.CreateParser().parse_args(
    ["call", "42", "open_door", "--arguments", '{"force":true}']
  )

  Result = control.Dispatch(Client, Arguments)  # type: ignore[arg-type]

  assert Result == {"actorId": 42, "methodName": "open_door"}
  assert Client.Calls == [
    ("invoke_actor_method", (42, "open_door"), {"Arguments": {"force": True}})
  ]


def TestUpdateOperationCommandsDispatchThroughControlClient() -> None:
  Client = FakeControlClient()
  Cases = [
    (["stop-engine-update"], {"operationId": "8", "status": "completed"}),
    (["start-engine-update"], {"engineUpdate": "running"}),
    (["step-engine-update", "3"], {"operationId": "9", "requestedCount": 3}),
    (["operation-status", "9"], {"operationId": "9", "status": "completed"}),
  ]

  for Tokens, Expected in Cases:
    Arguments = control.CreateParser().parse_args(Tokens)
    assert control.Dispatch(Client, Arguments) == Expected  # type: ignore[arg-type]

  assert Client.Calls == [
    ("stop_engine_update", (), {}),
    ("start_engine_update", (), {}),
    ("step_engine_update", (3,), {}),
    ("get_operation_status", ("9",), {}),
  ]


def TestRunPrintsCompactJson(monkeypatch, capsys, tmp_path) -> None:  # type: ignore[no-untyped-def]
  class ContextClient(FakeControlClient):
    def __init__(Self, _Config: object) -> None:
      super().__init__()

    def __enter__(Self) -> ContextClient:
      return Self

    def __exit__(Self, *_: object) -> None:
      return None

  monkeypatch.setattr(control, "ControlClient", ContextClient)
  monkeypatch.setattr(control, "ResolveControlConfig", lambda *_: object())

  assert control.Run(["actors"], tmp_path) == 0

  assert json.loads(capsys.readouterr().out) == {"actors": []}


def TestSetTransformRequiresAtLeastOneValue() -> None:
  Client = FakeControlClient()
  Arguments = control.CreateParser().parse_args(["set-transform", "42"])

  try:
    control.Dispatch(Client, Arguments)  # type: ignore[arg-type]
  except ValueError as Error:
    assert str(Error) == "Specify at least one transform value."
  else:
    raise AssertionError("set-transform accepted no transform values")


def TestResolveRegistrationSelectsTheOnlyLiveEngine(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  ExecutablePath = tmp_path / "Bin" / "Game-game.exe"
  RegistrationDirectory = ExecutablePath.parent / "control"
  RegistrationDirectory.mkdir(parents=True)
  (RegistrationDirectory / "12340.json").write_text('{"pid":12340,"port":39100}', encoding="utf-8")
  monkeypatch.setattr(control, "_GetProcessExecutablePath", lambda _: ExecutablePath)

  Registration = control.ResolveRegistration(ExecutablePath, None)

  assert Registration == control.ControlRegistration(12340, 39100)


def TestResolveRegistrationIgnoresStaleRegistrations(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  ExecutablePath = tmp_path / "Bin" / "Game-game.exe"
  RegistrationDirectory = ExecutablePath.parent / "control"
  RegistrationDirectory.mkdir(parents=True)
  (RegistrationDirectory / "12340.json").write_text('{"pid":12340,"port":39100}', encoding="utf-8")
  (RegistrationDirectory / "18520.json").write_text('{"pid":18520,"port":39101}', encoding="utf-8")
  monkeypatch.setattr(
    control,
    "_GetProcessExecutablePath",
    lambda ProcessId: ExecutablePath if ProcessId == 12340 else tmp_path / "Other.exe",
  )

  Registration = control.ResolveRegistration(ExecutablePath, None)

  assert Registration == control.ControlRegistration(12340, 39100)


def TestResolveRegistrationRequiresPidForMultipleLiveEngines(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  ExecutablePath = tmp_path / "Bin" / "Game-game.exe"
  RegistrationDirectory = ExecutablePath.parent / "control"
  RegistrationDirectory.mkdir(parents=True)
  for ProcessId, Port in ((12340, 39100), (18520, 39101)):
    (RegistrationDirectory / f"{ProcessId}.json").write_text(
      f'{{"pid":{ProcessId},"port":{Port}}}', encoding="utf-8"
    )
  monkeypatch.setattr(control, "_GetProcessExecutablePath", lambda _: ExecutablePath)

  try:
    control.ResolveRegistration(ExecutablePath, None)
  except ValueError as Error:
    assert str(Error) == (
      "Multiple BROCCOLI ENGINE instances are running.\n\n"
      "PID 12340\nPID 18520\n\n"
      "Specify the target PID:\n  broccoli.bat control --pid 12340 state"
    )
  else:
    raise AssertionError("multiple live registrations were accepted without --pid")


def TestResolveRegistrationReadsOnlyRequestedPid(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  ExecutablePath = tmp_path / "Bin" / "Game-game.exe"
  RegistrationDirectory = ExecutablePath.parent / "control"
  RegistrationDirectory.mkdir(parents=True)
  (RegistrationDirectory / "12340.json").write_text('{"pid":12340,"port":39100}', encoding="utf-8")
  monkeypatch.setattr(control, "_GetProcessExecutablePath", lambda _: ExecutablePath)

  Registration = control.ResolveRegistration(ExecutablePath, 12340)

  assert Registration == control.ControlRegistration(12340, 39100)


def TestResolveControlConfigSelectsTheExplicitPidConfiguration(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  (tmp_path / "Intermediate").mkdir()
  (tmp_path / "Intermediate" / "LastBuildConfiguration.txt").write_text(
    "Editor\n", encoding="utf-8"
  )
  (tmp_path / ".broccoli-project.json").write_text('{"project_name":"Game"}', encoding="utf-8")
  DebugExecutable = tmp_path / "Publish" / "Debug" / "Binaries" / "Game.exe"
  RegistrationDirectory = DebugExecutable.parent / "control"
  RegistrationDirectory.mkdir(parents=True)
  (RegistrationDirectory / "12340.json").write_text('{"pid":12340,"port":39100}', encoding="utf-8")
  monkeypatch.setattr(control, "_GetProcessExecutablePath", lambda _: DebugExecutable)

  Config = control.ResolveControlConfig(tmp_path, 12340)

  assert Config.Port == 39100


def TestResolveControlConfigRejectsAnExplicitPidFromAnotherProject(monkeypatch, tmp_path) -> None:  # type: ignore[no-untyped-def]
  (tmp_path / "Intermediate").mkdir()
  (tmp_path / "Intermediate" / "LastBuildConfiguration.txt").write_text(
    "Editor\n", encoding="utf-8"
  )
  (tmp_path / ".broccoli-project.json").write_text('{"project_name":"Game"}', encoding="utf-8")
  OtherExecutable = tmp_path / "OtherProject" / "OtherGame.exe"
  monkeypatch.setattr(control, "_GetProcessExecutablePath", lambda _: OtherExecutable)

  try:
    control.ResolveControlConfig(tmp_path, 12340)
  except ValueError as Error:
    assert str(Error) == "PID 12340 is not a running Engine instance of this project."
  else:
    raise AssertionError("another project's Engine PID was accepted")


def TestResolveTargetExecutableUsesPackagedEngineBinary(tmp_path) -> None:
  (tmp_path / "Intermediate").mkdir()
  (tmp_path / "Intermediate" / "LastBuildConfiguration.txt").write_text("Debug\n", encoding="utf-8")
  (tmp_path / ".broccoli-project.json").write_text('{"project_name":"Game"}', encoding="utf-8")

  ExecutablePath = control.ResolveTargetExecutable(tmp_path)

  assert ExecutablePath == tmp_path / "Publish" / "Debug" / "Binaries" / "Game.exe"


def TestBuildCliForwardsControlPid() -> None:
  Arguments = cli.CreateParser().parse_args(["control", "--pid", "12340", "state"])

  assert Arguments.pid == 12340
  assert Arguments.arguments == ["state"]
