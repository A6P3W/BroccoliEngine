from __future__ import annotations

from broccoli_mcp.errors import (
  BridgeInternalError,
  EngineApiError,
  EngineUnavailable,
  format_mcp_error,
)


def test_engine_api_error_keeps_engine_details() -> None:
  Error = EngineApiError(
    "REQUEST_TIMEOUT",
    "The main thread timed out.",
    Operation="get engine state",
    HttpStatus=504,
  )

  Text = format_mcp_error(Error)

  assert "REQUEST_TIMEOUT" in Text
  assert "The main thread timed out." in Text
  assert Error.Retryable


def test_unavailable_error_explains_how_to_start_engine() -> None:
  Error = EngineUnavailable("127.0.0.1", 39100, Operation="get engine state")

  Text = format_mcp_error(Error)

  assert "BROCCOLI ENGINE" in Text
  assert "-automation" in Text
  assert "127.0.0.1:39100" in Text


def test_internal_error_does_not_expose_exception_details() -> None:
  Error = BridgeInternalError(Operation="read game://state")

  Text = format_mcp_error(Error)

  assert "BRIDGE_INTERNAL_ERROR" in Text
  assert "Traceback" not in Text
