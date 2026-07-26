"""Live MCP stdio integration check against a running BROCCOLI ENGINE."""

from __future__ import annotations

import asyncio
import json
import sys
from datetime import timedelta
from pathlib import Path

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from mcp.types import TextResourceContents


async def run_integration() -> dict[str, object]:
  ProjectRoot = Path(__file__).resolve().parents[1]
  Parameters = StdioServerParameters(
    command=sys.executable,
    args=["-m", "broccoli_mcp"],
    cwd=ProjectRoot,
  )

  async with stdio_client(Parameters) as (ReadStream, WriteStream):
    async with ClientSession(
      ReadStream,
      WriteStream,
      read_timeout_seconds=timedelta(seconds=10),
    ) as Session:
      await Session.initialize()
      Resources = await Session.list_resources()
      ResourceUris = [str(Resource.uri) for Resource in Resources.resources]
      if "game://state" not in ResourceUris:
        raise RuntimeError("game://state is not exposed by the bridge.")
      if "game://world/actors" not in ResourceUris:
        raise RuntimeError("game://world/actors is not exposed by the bridge.")

      Result = await Session.read_resource("game://state")
      if len(Result.contents) != 1 or not isinstance(
        Result.contents[0],
        TextResourceContents,
      ):
        raise RuntimeError("game://state did not return one text resource.")
      State = json.loads(Result.contents[0].text)

      ActorResult = await Session.read_resource("game://world/actors")
      if len(ActorResult.contents) != 1 or not isinstance(
        ActorResult.contents[0],
        TextResourceContents,
      ):
        raise RuntimeError("game://world/actors did not return one text resource.")
      Actors = json.loads(ActorResult.contents[0].text)

  RequiredFields = {
    "sceneName",
    "fps",
    "paused",
    "worldAvailable",
    "actorCount",
  }
  MissingFields = RequiredFields.difference(State)
  if MissingFields:
    raise RuntimeError(f"State response is missing fields: {sorted(MissingFields)}")
  RequiredActorListFields = {"sceneName", "actorCount", "actors"}
  MissingActorListFields = RequiredActorListFields.difference(Actors)
  if MissingActorListFields:
    raise RuntimeError(f"Actor response is missing fields: {sorted(MissingActorListFields)}")
  if Actors["actorCount"] != len(Actors["actors"]):
    raise RuntimeError("Actor count does not match the actors array.")
  return {"state": State, "worldActors": Actors}


def main() -> int:
  State = asyncio.run(run_integration())
  print(json.dumps(State, ensure_ascii=False, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
