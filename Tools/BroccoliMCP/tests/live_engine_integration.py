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
      Tools = await Session.list_tools()
      ToolNames = [Tool.name for Tool in Tools.tools]
      if "game://state" not in ResourceUris:
        raise RuntimeError("game://state is not exposed by the bridge.")
      if "game://world/actors" not in ResourceUris:
        raise RuntimeError("game://world/actors is not exposed by the bridge.")
      if "game://logs/recent" not in ResourceUris:
        raise RuntimeError("game://logs/recent is not exposed by the bridge.")
      for ToolName in ("spawn_actor", "destroy_actor", "set_actor_transform"):
        if ToolName not in ToolNames:
          raise RuntimeError(f"{ToolName} is not exposed by the bridge.")

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

      LogResult = await Session.read_resource("game://logs/recent")
      if len(LogResult.contents) != 1 or not isinstance(
        LogResult.contents[0],
        TextResourceContents,
      ):
        raise RuntimeError("game://logs/recent did not return one text resource.")
      Logs = json.loads(LogResult.contents[0].text)

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
  RequiredLogFields = {
    "entries",
    "count",
    "oldestAvailableSequence",
    "latestSequence",
    "nextAfterSequence",
    "historyLost",
    "hasMore",
  }
  MissingLogFields = RequiredLogFields.difference(Logs)
  if MissingLogFields:
    raise RuntimeError(f"Log response is missing fields: {sorted(MissingLogFields)}")
  if Logs["count"] != len(Logs["entries"]):
    raise RuntimeError("Log count does not match the entries array.")
  return {"state": State, "worldActors": Actors, "recentLogs": Logs}


def main() -> int:
  State = asyncio.run(run_integration())
  print(json.dumps(State, ensure_ascii=False, indent=2))
  return 0


if __name__ == "__main__":
  sys.exit(main())
