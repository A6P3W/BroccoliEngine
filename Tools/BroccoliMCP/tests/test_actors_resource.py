from __future__ import annotations

import json

from broccoli_mcp.models import ActorInfo, ActorList, ActorTransform
from broccoli_mcp.server import create_server, read_actors_resource


class FakeActorClient:
  def get_actors(Self) -> ActorList:
    Actor = ActorInfo(
      ActorId=7,
      InstanceName="ATestActor_1",
      ClassName="ATestActor",
      Transform=ActorTransform(
        LocationX=1.0,
        LocationY=2.0,
        Rotation=3.0,
        Scale=1.0,
      ),
    )
    return ActorList(SceneName="SceneTest", ActorCount=1, Actors=(Actor,))


def test_actors_resource_returns_envelope_free_json() -> None:
  Result = read_actors_resource(FakeActorClient())  # type: ignore[arg-type]

  assert json.loads(Result) == {
    "sceneName": "SceneTest",
    "actorCount": 1,
    "actors": [
      {
        "actorId": 7,
        "instanceName": "ATestActor_1",
        "className": "ATestActor",
        "transform": {
          "location": {"x": 1.0, "y": 2.0},
          "rotation": 3.0,
          "scale": 1.0,
        },
      }
    ],
  }


def test_server_lists_world_actors_resource() -> None:
  Server = create_server(FakeActorClient())  # type: ignore[arg-type]

  Resources = Server._resource_manager.list_resources()

  assert [str(Resource.uri) for Resource in Resources] == [
    "game://state",
    "game://world/actors",
  ]
