from __future__ import annotations

import json

import anyio
import httpx
import pytest
from mcp.shared.memory import create_connected_server_and_client_session

from broccoli_mcp.config import BridgeConfig
from broccoli_mcp.engine_client import EngineClient
from broccoli_mcp.errors import InvalidEngineResponse
from broccoli_mcp.models import LogEntry, RecentLogs
from broccoli_mcp.server import create_server, read_logs_resource

LOG_DATA = {
  "entries": [
    {
      "sequence": 125,
      "timestamp": "2026-07-26T08:30:15.123Z",
      "level": "warning",
      "category": "InitializeAutomation",
      "message": "Automation server startup failed.",
    }
  ],
  "count": 1,
  "oldestAvailableSequence": 42,
  "latestSequence": 125,
  "nextAfterSequence": 125,
  "droppedEntries": 0,
  "historyLost": False,
  "hasMore": False,
}


def test_recent_logs_model_accepts_valid_data() -> None:
  Logs = RecentLogs.from_mapping(LOG_DATA, Operation="test logs")

  assert Logs.Count == 1
  assert Logs.Entries[0].Level == "warning"
  assert Logs.to_dict() == LOG_DATA


def test_recent_logs_model_rejects_count_mismatch() -> None:
  with pytest.raises(InvalidEngineResponse, match="count"):
    RecentLogs.from_mapping({**LOG_DATA, "count": 2}, Operation="test logs")


def test_recent_logs_model_rejects_non_increasing_sequences() -> None:
  DuplicateEntries = [LOG_DATA["entries"][0], LOG_DATA["entries"][0]]
  with pytest.raises(InvalidEngineResponse, match="increasing"):
    RecentLogs.from_mapping(
      {**LOG_DATA, "entries": DuplicateEntries, "count": 2},
      Operation="test logs",
    )


def test_get_recent_logs_sends_query_parameters() -> None:
  def handler(Request: httpx.Request) -> httpx.Response:
    assert Request.method == "GET"
    assert Request.url == (
      "http://127.0.0.1:39100/api/v1/logs/recent?limit=25&level=warning&afterSequence=100"
    )
    return httpx.Response(
      200,
      json={"success": True, "data": LOG_DATA},
      headers={"Content-Type": "application/json"},
    )

  Client = EngineClient(BridgeConfig(), Transport=httpx.MockTransport(handler))
  try:
    Logs = Client.get_recent_logs(Limit=25, Level="WARNING", AfterSequence=100)
  finally:
    Client.close()

  assert Logs.LatestSequence == 125


@pytest.mark.parametrize(
  "Arguments",
  [
    {"Limit": 0},
    {"Limit": 1001},
    {"Level": "trace"},
    {"AfterSequence": -1},
  ],
)
def test_get_recent_logs_rejects_invalid_query(Arguments: dict[str, object]) -> None:
  Client = EngineClient(BridgeConfig())
  try:
    with pytest.raises(ValueError):
      Client.get_recent_logs(**Arguments)  # type: ignore[arg-type]
  finally:
    Client.close()


class FakeLogClient:
  def get_recent_logs(Self, *, Limit: int) -> RecentLogs:
    assert Limit == 100
    Entry = LogEntry(
      Sequence=125,
      Timestamp="2026-07-26T08:30:15.123Z",
      Level="warning",
      Category="InitializeAutomation",
      Message="Automation server startup failed.",
    )
    return RecentLogs(
      Entries=(Entry,),
      Count=1,
      OldestAvailableSequence=42,
      LatestSequence=125,
      NextAfterSequence=125,
      DroppedEntries=0,
      HistoryLost=False,
      HasMore=False,
    )


def test_logs_resource_returns_envelope_free_json_and_is_registered() -> None:
  Client = FakeLogClient()

  assert json.loads(read_logs_resource(Client)) == LOG_DATA  # type: ignore[arg-type]

  Server = create_server(Client)  # type: ignore[arg-type]
  assert [str(Resource.uri) for Resource in Server._resource_manager.list_resources()] == [
    "game://state",
    "game://world/actors",
    "game://logs/recent",
  ]


def test_mcp_client_can_read_recent_logs_resource() -> None:
  Server = create_server(FakeLogClient())  # type: ignore[arg-type]

  async def execute() -> str:
    async with create_connected_server_and_client_session(Server) as Session:
      Read = await Session.read_resource("game://logs/recent")
      return Read.contents[0].text  # type: ignore[union-attr]

  assert json.loads(anyio.run(execute)) == LOG_DATA
