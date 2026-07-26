"""Bridge-specific error types and safe user-facing error conversion."""

from __future__ import annotations


class BridgeError(Exception):
  """Base class for errors that may safely be reported to an MCP client."""

  def __init__(
    Self,
    Code: str,
    Message: str,
    *,
    Operation: str,
    Retryable: bool = False,
  ) -> None:
    super().__init__(Message)
    Self.Code = Code
    Self.Message = Message
    Self.Operation = Operation
    Self.Retryable = Retryable

  def __str__(Self) -> str:
    RetryHint = " Retry the request after checking the engine." if Self.Retryable else ""
    return f"{Self.Operation} failed [{Self.Code}]: {Self.Message}{RetryHint}"


class EngineUnavailable(BridgeError):
  """The engine Automation Server cannot be reached."""

  def __init__(Self, Host: str, Port: int, *, Operation: str) -> None:
    super().__init__(
      "ENGINE_UNAVAILABLE",
      (
        f"Could not connect to BROCCOLI ENGINE Automation Server at "
        f"{Host}:{Port}. Start BROCCOLI ENGINE with -automation."
      ),
      Operation=Operation,
      Retryable=True,
    )


class EngineTimeout(BridgeError):
  """The engine did not respond before the configured timeout."""

  def __init__(Self, *, Operation: str) -> None:
    super().__init__(
      "ENGINE_TIMEOUT",
      "BROCCOLI ENGINE did not respond before the bridge timeout.",
      Operation=Operation,
      Retryable=True,
    )


class EngineApiError(BridgeError):
  """The engine returned a valid failure response."""

  def __init__(
    Self,
    Code: str,
    Message: str,
    *,
    Operation: str,
    HttpStatus: int,
  ) -> None:
    super().__init__(Code, Message, Operation=Operation, Retryable=HttpStatus >= 500)
    Self.HttpStatus = HttpStatus


class InvalidEngineResponse(BridgeError):
  """The engine response does not satisfy the Automation API contract."""

  def __init__(Self, Message: str, *, Operation: str) -> None:
    super().__init__(
      "INVALID_ENGINE_RESPONSE",
      Message,
      Operation=Operation,
      Retryable=False,
    )


class BridgeConfigurationError(BridgeError):
  """Bridge configuration is invalid."""

  def __init__(Self, Message: str) -> None:
    super().__init__(
      "BRIDGE_CONFIGURATION_ERROR",
      Message,
      Operation="bridge configuration",
      Retryable=False,
    )


class BridgeInternalError(BridgeError):
  """An unexpected bridge failure represented without internal details."""

  def __init__(Self, *, Operation: str) -> None:
    super().__init__(
      "BRIDGE_INTERNAL_ERROR",
      "The bridge encountered an unexpected internal error.",
      Operation=Operation,
      Retryable=False,
    )


def format_mcp_error(Error: BridgeError) -> str:
  """Return the bounded, safe error text exposed through MCP."""

  return str(Error)
