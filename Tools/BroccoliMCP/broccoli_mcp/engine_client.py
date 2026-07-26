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
from .models import EngineState

LOGGER = logging.getLogger(__name__)
STATE_OPERATION = "get engine state"


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

  def _request_json(
    Self,
    Method: str,
    Path: str,
    *,
    Operation: str,
  ) -> Any:
    try:
      with Self.HttpClient.stream(Method, Path) as Response:
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
