"""MCP stdio server exposing BROCCOLI ENGINE resources."""

import json
import logging

from mcp.server import MCPServer

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


def read_actors_resource(Client: EngineClient) -> str:
  """Read and serialize actors in the current engine world for MCP."""

  try:
    Actors = Client.get_actors()
    return json.dumps(Actors.to_dict(), ensure_ascii=False, separators=(",", ":"))
  except BridgeError:
    raise
  except Exception:
    LOGGER.exception("Unexpected error while reading game://world/actors")
    raise BridgeInternalError(Operation="read game://world/actors") from None


def read_logs_resource(Client: EngineClient) -> str:
  """Read and serialize recent in-memory engine logs for MCP."""

  try:
    Logs = Client.get_recent_logs(Limit=100)
    return json.dumps(Logs.to_dict(), ensure_ascii=False, separators=(",", ":"))
  except BridgeError:
    raise
  except Exception:
    LOGGER.exception("Unexpected error while reading game://logs/recent")
    raise BridgeInternalError(Operation="read game://logs/recent") from None


def create_server(Client: EngineClient) -> MCPServer:
  """Create an MCP server backed by a process-owned HTTP client."""

  Mcp = MCPServer("BROCCOLI ENGINE")

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

  @Mcp.resource(
    "game://world/actors",
    name="BROCCOLI ENGINE World Actors",
    description="Actors in the current world with identity and transform data.",
    mime_type="application/json",
  )
  def game_world_actors() -> str:
    LOGGER.info("Reading resource game://world/actors")
    try:
      Result = read_actors_resource(Client)
    except BridgeError as Error:
      LOGGER.warning("Resource game://world/actors failed: %s", Error.Code)
      raise ValueError(format_mcp_error(Error)) from None
    LOGGER.info("Resource game://world/actors completed")
    return Result

  @Mcp.resource(
    "game://logs/recent",
    name="BROCCOLI ENGINE Recent Logs",
    description="Recent in-memory engine logs.",
    mime_type="application/json",
  )
  def game_recent_logs() -> str:
    LOGGER.info("Reading resource game://logs/recent")
    try:
      Result = read_logs_resource(Client)
    except BridgeError as Error:
      LOGGER.warning("Resource game://logs/recent failed: %s", Error.Code)
      raise ValueError(format_mcp_error(Error)) from None
    LOGGER.info("Resource game://logs/recent completed")
    return Result

  @Mcp.tool(
    name="get_system_commands",
    description="Get registered BROCCOLI ENGINE system command descriptors.",
  )
  def get_system_commands() -> dict[str, object]:
    try:
      return Client.get_system_commands().to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except Exception:
      LOGGER.exception("Unexpected error while getting system commands")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="get system commands"))
      ) from None

  @Mcp.tool(
    name="get_actor",
    description="Get the current state of a world actor by ID.",
  )
  def get_actor(actor_id: int) -> dict[str, object]:
    try:
      return Client.get_actor(actor_id).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while getting an actor")
      raise ValueError(format_mcp_error(BridgeInternalError(Operation="get actor"))) from None

  @Mcp.tool(name="get_actor_components", description="Get components held by a world actor.")
  def get_actor_components(actor_id: int) -> dict[str, object]:
    try:
      return Client.get_actor_components(actor_id).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while getting actor components")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="get actor components"))
      ) from None

  @Mcp.tool(
    name="call_component_method",
    description="Call an automation method on a component identified by actor and component IDs.",
  )
  def call_component_method(
    actor_id: int,
    component_id: int,
    method: str,
    arguments: dict[str, object] | None = None,
  ) -> dict[str, object]:
    try:
      return dict(Client.invoke_component_method(actor_id, component_id, method, arguments))
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None

  @Mcp.tool(
    name="find_actors",
    description="Find world actors by exact class name and/or instance name.",
  )
  def find_actors(
    class_name: str | None = None,
    instance_name: str | None = None,
  ) -> dict[str, object]:
    try:
      return Client.find_actors(ClassName=class_name, InstanceName=instance_name).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while finding actors")
      raise ValueError(format_mcp_error(BridgeInternalError(Operation="find actors"))) from None

  @Mcp.tool(
    name="get_registered_actor_classes",
    description="Get actor classes registered in BROCCOLI ENGINE.",
  )
  def get_registered_actor_classes() -> dict[str, object]:
    try:
      return Client.get_registered_actor_classes().to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except Exception:
      LOGGER.exception("Unexpected error while getting registered actor classes")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="get registered actor classes"))
      ) from None

  @Mcp.tool(
    name="get_levels",
    description="Get levels registered in BROCCOLI ENGINE.",
  )
  def get_levels() -> dict[str, object]:
    try:
      return Client.get_levels().to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except Exception:
      LOGGER.exception("Unexpected error while getting registered levels")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="get registered levels"))
      ) from None

  @Mcp.tool(
    name="get_class_methods",
    description="Get automation methods registered for an actor class.",
  )
  def get_class_methods(class_name: str) -> dict[str, object]:
    try:
      return Client.get_class_methods(class_name).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while getting actor class methods")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="get actor class methods"))
      ) from None

  @Mcp.tool(
    name="spawn_actor",
    description="Spawn a registered actor in the current world.",
  )
  def spawn_actor(
    class_name: str,
    location_x: float = 0.0,
    location_y: float = 0.0,
    rotation: float = 0.0,
    scale: float = 1.0,
    instance_name: str | None = None,
  ) -> dict[str, object]:
    try:
      return Client.spawn_actor(
        class_name,
        LocationX=location_x,
        LocationY=location_y,
        Rotation=rotation,
        Scale=scale,
        InstanceName=instance_name,
      ).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while spawning an actor")
      raise ValueError(format_mcp_error(BridgeInternalError(Operation="spawn actor"))) from None

  @Mcp.tool(
    name="destroy_actor",
    description="Request destruction of an actor in the current world.",
  )
  def destroy_actor(actor_id: int) -> dict[str, object]:
    try:
      return Client.destroy_actor(actor_id).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while destroying an actor")
      raise ValueError(format_mcp_error(BridgeInternalError(Operation="destroy actor"))) from None

  @Mcp.tool(
    name="set_actor_transform",
    description="Partially update an actor transform in the current world.",
  )
  def set_actor_transform(
    actor_id: int,
    location_x: float | None = None,
    location_y: float | None = None,
    rotation: float | None = None,
    scale: float | None = None,
  ) -> dict[str, object]:
    try:
      return Client.set_actor_transform(
        actor_id,
        LocationX=location_x,
        LocationY=location_y,
        Rotation=rotation,
        Scale=scale,
      ).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while setting an actor transform")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="set actor transform"))
      ) from None

  @Mcp.tool(
    name="invoke_actor_method",
    description="Invoke a registered automation method on an actor.",
  )
  def invoke_actor_method(
    actor_id: int,
    method_name: str,
    arguments: dict[str, object] | None = None,
  ) -> dict[str, object]:
    try:
      return Client.invoke_actor_method(actor_id, method_name, Arguments=arguments).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while invoking an actor method")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="invoke actor method"))
      ) from None

  @Mcp.tool(
    name="execute_system_command",
    description="Execute a registered BROCCOLI ENGINE system command.",
  )
  def execute_system_command(
    command_name: str,
    arguments: dict[str, object] | None = None,
  ) -> dict[str, object]:
    try:
      return Client.execute_system_command(command_name, Arguments=arguments).to_dict()
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None
    except ValueError:
      raise
    except Exception:
      LOGGER.exception("Unexpected error while executing a system command")
      raise ValueError(
        format_mcp_error(BridgeInternalError(Operation="execute system command"))
      ) from None

  @Mcp.tool(
    name="open_level_by_id",
    description="Open a registered BROCCOLI ENGINE level by scene ID.",
  )
  def open_level_by_id(scene_id: int) -> dict[str, object]:
    Result = Client.execute_system_command(
      "open_level_by_id",
      Arguments={"sceneId": scene_id},
    )
    CommandResult = Result.Result
    if CommandResult.get("queued") is not True:
      raise ValueError(f"Level ID {scene_id} could not be queued.")
    return CommandResult

  @Mcp.tool(
    name="open_level_by_path",
    description="Open a BROCCOLI ENGINE level by file path.",
  )
  def open_level_by_path(level_path: str) -> dict[str, object]:
    Result = Client.execute_system_command(
      "open_level_by_path",
      Arguments={"levelPath": level_path},
    )
    CommandResult = Result.Result
    if CommandResult.get("queued") is not True:
      raise ValueError(f"Level path '{level_path}' could not be queued.")
    return CommandResult

  @Mcp.tool(
    name="get_component_methods",
    description="Get registered automation methods for a component on an actor.",
  )
  def get_component_methods(actor_id: int, component_id: int) -> dict[str, object]:
    try:
      return dict(Client.get_component_methods(actor_id, component_id))
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None

  @Mcp.tool(
    name="invoke_component_method",
    description="Invoke a registered automation method on a component of an actor.",
  )
  def invoke_component_method(
    actor_id: int,
    component_id: int,
    method_name: str,
    arguments: dict[str, object] | None = None,
  ) -> dict[str, object]:
    try:
      return dict(Client.invoke_component_method(actor_id, component_id, method_name, arguments))
    except BridgeError as Error:
      raise ValueError(format_mcp_error(Error)) from None

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
