"""HTTP client for the BROCCOLI ENGINE Automation API."""

from __future__ import annotations

import json
import logging
from collections.abc import Mapping
from typing import Any

import httpx

from .config import BridgeConfig
from .errors import (
  BridgeInternalError,
  EngineApiError,
  EngineTimeout,
  EngineUnavailable,
  InvalidEngineResponse,
)
from .models import (
  AUTOMATION_NAME_PATTERN,
  LOG_LEVELS,
  MAX_ACTOR_ID,
  ActorInfo,
  ActorList,
  ActorMethodList,
  ActorMethodResult,
  DestroyActorResult,
  EngineState,
  RecentLogs,
  TransformPatch,
)

LOGGER = logging.getLogger(__name__)
STATE_OPERATION = "get engine state"
ACTORS_OPERATION = "get world actors"
SPAWN_ACTOR_OPERATION = "spawn world actor"
RECENT_LOGS_OPERATION = "get recent engine logs"


class EngineClient:
  """Reusable client with a fixed localhost base URL."""

  def __init__(
    Self,
    Config: BridgeConfig,
    *,
    Transport: httpx.BaseTransport | None = None,
  ) -> None:
    Self.Config = Config
    Timeout = httpx.Timeout(
      connect=Config.ConnectTimeoutSeconds,
      read=Config.ReadTimeoutSeconds,
      write=Config.ReadTimeoutSeconds,
      pool=Config.ConnectTimeoutSeconds,
    )
    Self.HttpClient = httpx.Client(
      base_url=Config.BaseUrl,
      headers={"Accept": "application/json"},
      timeout=Timeout,
      transport=Transport,
      trust_env=False,
    )

  def __enter__(Self) -> EngineClient:
    return Self

  def __exit__(Self, ExceptionType: object, Exception: object, Traceback: object) -> None:
    Self.close()

  def close(Self) -> None:
    Self.HttpClient.close()

  def get_state(Self) -> EngineState:
    Data = Self._request_json("GET", "state", Operation=STATE_OPERATION)
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=STATE_OPERATION,
      )
    return EngineState.from_mapping(Data, Operation=STATE_OPERATION)

  def get_actors(Self) -> ActorList:
    Data = Self._request_json("GET", "world/actors", Operation=ACTORS_OPERATION)
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=ACTORS_OPERATION,
      )
    return ActorList.from_mapping(Data, Operation=ACTORS_OPERATION)

  def get_actor(Self, ActorId: int) -> ActorInfo:
    Self._validate_actor_id(ActorId)
    Operation = f"get world actor {ActorId}"
    Data = Self._request_json("GET", f"world/actors/{ActorId}", Operation=Operation)
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=Operation,
      )
    return ActorInfo.from_mapping(Data, Operation=Operation)

  def get_actor_methods(Self, ActorId: int) -> ActorMethodList:
    Self._validate_actor_id(ActorId)
    Operation = f"get world actor {ActorId} methods"
    Data = Self._request_json(
      "GET",
      f"world/actors/{ActorId}/methods",
      Operation=Operation,
    )
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=Operation,
      )
    Result = ActorMethodList.from_mapping(Data, Operation=Operation)
    if Result.ActorId != ActorId:
      raise InvalidEngineResponse(
        "Actor method list field 'actorId' does not match the request.",
        Operation=Operation,
      )
    return Result

  def invoke_actor_method(
    Self,
    ActorId: int,
    MethodName: str,
    Arguments: Mapping[str, Any] | None = None,
  ) -> ActorMethodResult:
    Self._validate_actor_id(ActorId)
    Self._validate_operation_name(MethodName, "MethodName")
    if Arguments is not None and not isinstance(Arguments, Mapping):
      raise ValueError("Arguments must be an object.")
    Body = {} if Arguments is None else dict(Arguments)
    Operation = f"invoke world actor {ActorId} method {MethodName}"
    Data = Self._request_json(
      "POST",
      f"world/actors/{ActorId}/methods/{MethodName}",
      Operation=Operation,
      JsonBody=Body,
    )
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=Operation,
      )
    return ActorMethodResult.from_mapping(
      Data,
      Operation=Operation,
      ExpectedActorId=ActorId,
      ExpectedMethodName=MethodName,
    )

  def spawn_actor(
    Self,
    ClassName: str,
    *,
    LocationX: float = 0.0,
    LocationY: float = 0.0,
    Rotation: float = 0.0,
    Scale: float = 1.0,
    InstanceName: str | None = None,
  ) -> ActorInfo:
    Self._validate_name(ClassName, "ClassName")
    if InstanceName is not None:
      Self._validate_name(InstanceName, "InstanceName")
    Transform = TransformPatch(
      LocationX=LocationX,
      LocationY=LocationY,
      Rotation=Rotation,
      Scale=Scale,
    )
    Body: dict[str, Any] = {
      "className": ClassName,
      "transform": Transform.to_dict(),
    }
    if InstanceName is not None:
      Body["instanceName"] = InstanceName

    Data = Self._request_json(
      "POST",
      "world/actors",
      Operation=SPAWN_ACTOR_OPERATION,
      JsonBody=Body,
    )
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=SPAWN_ACTOR_OPERATION,
      )
    return ActorInfo.from_mapping(Data, Operation=SPAWN_ACTOR_OPERATION)

  def destroy_actor(Self, ActorId: int) -> DestroyActorResult:
    Self._validate_actor_id(ActorId)
    Operation = f"destroy world actor {ActorId}"
    Data = Self._request_json("DELETE", f"world/actors/{ActorId}", Operation=Operation)
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=Operation,
      )
    return DestroyActorResult.from_mapping(Data, Operation=Operation)

  def set_actor_transform(
    Self,
    ActorId: int,
    *,
    LocationX: float | None = None,
    LocationY: float | None = None,
    Rotation: float | None = None,
    Scale: float | None = None,
  ) -> ActorInfo:
    Self._validate_actor_id(ActorId)
    Patch = TransformPatch(
      LocationX=LocationX,
      LocationY=LocationY,
      Rotation=Rotation,
      Scale=Scale,
    )
    Operation = f"set world actor {ActorId} transform"
    Data = Self._request_json(
      "PATCH",
      f"world/actors/{ActorId}/transform",
      Operation=Operation,
      JsonBody=Patch.to_dict(),
    )
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=Operation,
      )
    return ActorInfo.from_mapping(Data, Operation=Operation)

  def get_recent_logs(
    Self,
    *,
    Limit: int = 100,
    Level: str | None = None,
    AfterSequence: int | None = None,
  ) -> RecentLogs:
    if isinstance(Limit, bool) or not isinstance(Limit, int) or not 1 <= Limit <= 1000:
      raise ValueError("Limit must be an integer between 1 and 1000.")
    Params: dict[str, str | int] = {"limit": Limit}
    if Level is not None:
      if not isinstance(Level, str) or Level.lower() not in LOG_LEVELS:
        raise ValueError("Level must be debug, info, warning, or error.")
      Params["level"] = Level.lower()
    if AfterSequence is not None:
      if (
        isinstance(AfterSequence, bool)
        or not isinstance(AfterSequence, int)
        or not 0 <= AfterSequence <= MAX_ACTOR_ID
      ):
        raise ValueError("AfterSequence must be an unsigned 64-bit integer.")
      Params["afterSequence"] = AfterSequence

    Data = Self._request_json(
      "GET",
      "logs/recent",
      Operation=RECENT_LOGS_OPERATION,
      Params=Params,
    )
    if not isinstance(Data, Mapping):
      raise InvalidEngineResponse(
        "The successful response 'data' field must be an object.",
        Operation=RECENT_LOGS_OPERATION,
      )
    return RecentLogs.from_mapping(Data, Operation=RECENT_LOGS_OPERATION)

  def _request_json(
    Self,
    Method: str,
    Path: str,
    *,
    Operation: str,
    JsonBody: Mapping[str, Any] | None = None,
    Params: Mapping[str, str | int] | None = None,
  ) -> Any:
    try:
      with Self.HttpClient.stream(Method, Path, json=JsonBody, params=Params) as Response:
        HttpStatus = Response.status_code
        ContentType = Response.headers.get("content-type", "")
        ResponseContent = bytearray()
        for Chunk in Response.iter_bytes():
          ResponseContent.extend(Chunk)
          if len(ResponseContent) > Self.Config.MaxResponseBytes:
            raise InvalidEngineResponse(
              "The engine response exceeds the bridge size limit.",
              Operation=Operation,
            )
    except InvalidEngineResponse:
      raise
    except httpx.TimeoutException:
      LOGGER.warning("%s timed out", Operation)
      raise EngineTimeout(Operation=Operation) from None
    except (httpx.ConnectError, httpx.NetworkError):
      LOGGER.warning("%s could not connect to the engine", Operation)
      raise EngineUnavailable(
        Self.Config.Host,
        Self.Config.Port,
        Operation=Operation,
      ) from None
    except httpx.HTTPError:
      LOGGER.exception("%s failed in the HTTP client", Operation)
      raise BridgeInternalError(Operation=Operation) from None

    if ContentType.split(";", 1)[0].strip().lower() != "application/json":
      raise InvalidEngineResponse(
        "The engine response Content-Type is not application/json.",
        Operation=Operation,
      )
    if not ResponseContent:
      raise InvalidEngineResponse("The engine response body is empty.", Operation=Operation)

    try:
      Body = json.loads(ResponseContent)
    except (UnicodeDecodeError, json.JSONDecodeError):
      raise InvalidEngineResponse(
        "The engine response body is not valid JSON.",
        Operation=Operation,
      ) from None

    if not isinstance(Body, dict):
      raise InvalidEngineResponse(
        "The engine response root must be an object.",
        Operation=Operation,
      )
    Success = Body.get("success")
    if not isinstance(Success, bool):
      raise InvalidEngineResponse(
        "The engine response 'success' field must be a boolean.",
        Operation=Operation,
      )

    if not Success:
      Self._raise_api_error(Body, HttpStatus, Operation)

    if not 200 <= HttpStatus < 300:
      raise EngineApiError(
        f"HTTP_{HttpStatus}",
        f"The engine returned unexpected HTTP status {HttpStatus}.",
        Operation=Operation,
        HttpStatus=HttpStatus,
      )
    if "data" not in Body:
      raise InvalidEngineResponse(
        "The successful engine response is missing 'data'.",
        Operation=Operation,
      )
    return Body["data"]

  @staticmethod
  def _validate_actor_id(ActorId: int) -> None:
    if (
      isinstance(ActorId, bool) or not isinstance(ActorId, int) or not 1 <= ActorId <= MAX_ACTOR_ID
    ):
      raise ValueError("ActorId must be an unsigned 64-bit integer greater than zero.")

  @staticmethod
  def _validate_name(Value: str, FieldName: str) -> None:
    if not isinstance(Value, str) or not 1 <= len(Value.encode("utf-8")) <= 128:
      raise ValueError(f"{FieldName} must contain between 1 and 128 UTF-8 bytes.")

  @staticmethod
  def _validate_operation_name(Value: str, FieldName: str) -> None:
    if not isinstance(Value, str) or not AUTOMATION_NAME_PATTERN.fullmatch(Value):
      raise ValueError(f"{FieldName} must match ^[a-z][a-z0-9_]{{0,127}}$.")

  @staticmethod
  def _raise_api_error(
    Body: dict[str, Any],
    HttpStatus: int,
    Operation: str,
  ) -> None:
    ErrorData = Body.get("error")
    if not isinstance(ErrorData, dict):
      raise InvalidEngineResponse(
        "The failed engine response 'error' field must be an object.",
        Operation=Operation,
      )
    ErrorCode = ErrorData.get("code")
    ErrorMessage = ErrorData.get("message")
    if not isinstance(ErrorCode, str) or not ErrorCode:
      raise InvalidEngineResponse(
        "The engine error 'code' field must be a non-empty string.",
        Operation=Operation,
      )
    if not isinstance(ErrorMessage, str) or not ErrorMessage:
      raise InvalidEngineResponse(
        "The engine error 'message' field must be a non-empty string.",
        Operation=Operation,
      )
    LOGGER.warning("%s rejected by engine: %s (HTTP %d)", Operation, ErrorCode, HttpStatus)
    raise EngineApiError(
      ErrorCode,
      ErrorMessage,
      Operation=Operation,
      HttpStatus=HttpStatus,
    )
