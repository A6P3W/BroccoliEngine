"""Shared control error types for the Automation API."""

from __future__ import annotations


class ControlError(Exception):
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


class EngineUnavailable(ControlError):
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


class EngineTimeout(ControlError):
  """The engine did not respond before the configured timeout."""

  def __init__(Self, *, Operation: str) -> None:
    super().__init__(
      "ENGINE_TIMEOUT",
      "BROCCOLI ENGINE did not respond before the bridge timeout.",
      Operation=Operation,
      Retryable=True,
    )


class EngineApiError(ControlError):
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


class InvalidEngineResponse(ControlError):
  """The engine response does not satisfy the Automation API contract."""

  def __init__(Self, Message: str, *, Operation: str) -> None:
    super().__init__(
      "INVALID_ENGINE_RESPONSE",
      Message,
      Operation=Operation,
      Retryable=False,
    )


class ControlConfigurationError(ControlError):
  """Bridge configuration is invalid."""

  def __init__(Self, Message: str) -> None:
    super().__init__(
      "CONTROL_CONFIGURATION_ERROR",
      Message,
      Operation="control configuration",
      Retryable=False,
    )


class ControlInternalError(ControlError):
  """An unexpected bridge failure represented without internal details."""

  def __init__(Self, *, Operation: str) -> None:
    super().__init__(
      "CONTROL_INTERNAL_ERROR",
      "The control client encountered an unexpected internal error.",
      Operation=Operation,
      Retryable=False,
    )
