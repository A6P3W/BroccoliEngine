from __future__ import annotations

import json

from broccoli_build import control


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
        "LocationX": 12.0,
        "LocationY": 0.0,
        "Rotation": 0.0,
        "Scale": 1.0,
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


def TestRunPrintsCompactJson(monkeypatch, capsys) -> None:  # type: ignore[no-untyped-def]
  class ContextClient(FakeControlClient):
    def __init__(Self, _Config: object) -> None:
      super().__init__()

    def __enter__(Self) -> ContextClient:
      return Self

    def __exit__(Self, *_: object) -> None:
      return None

  monkeypatch.setattr(control, "ControlClient", ContextClient)
  monkeypatch.setattr(control, "load_config", lambda _: object())

  assert control.Run(["actors"]) == 0

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
