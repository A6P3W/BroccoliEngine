"""MCP stdio server exposing BROCCOLI ENGINE resources."""

import json
import logging

from mcp.server.fastmcp import FastMCP

from .config import BridgeConfig
from .engine_client import EngineClient
from .errors import BridgeError, BridgeInternalError, format_mcp_error

LOGGER = logging.getLogger(__name__)


def read_state_resource(Client: EngineClient) -> str:
  """Read and serialize the current engine state for MCP."""

  try:
    State = Client.get_state()
    return json.dumps(State.to_dict(), ensure_ascii=False, separators=(",", ":"))
  except BridgeError:
    raise
  except Exception:
    LOGGER.exception("Unexpected error while reading game://state")
    raise BridgeInternalError(Operation="read game://state") from None


def create_server(Client: EngineClient) -> FastMCP:
  """Create an MCP server backed by a process-owned HTTP client."""

  Mcp = FastMCP("BROCCOLI ENGINE")

  @Mcp.resource(
    "game://state",
    name="BROCCOLI ENGINE State",
    description="Current scene, performance, pause, world, and actor state.",
    mime_type="application/json",
  )
  def game_state() -> str:
    LOGGER.info("Reading resource game://state")
    try:
      Result = read_state_resource(Client)
    except BridgeError as Error:
      LOGGER.warning("Resource game://state failed: %s", Error.Code)
      raise ValueError(format_mcp_error(Error)) from None
    LOGGER.info("Resource game://state completed")
    return Result

  return Mcp


def run_server(Config: BridgeConfig) -> None:
  """Run stdio and close the process-owned HTTP client on exit."""

  Client = EngineClient(Config)
  LOGGER.info("BROCCOLI ENGINE MCP Bridge started")
  try:
    create_server(Client).run(transport="stdio")
  finally:
    Client.close()
    LOGGER.info("BROCCOLI ENGINE MCP Bridge stopped")
