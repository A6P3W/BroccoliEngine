"""Configuration loading and validation for the bridge."""

from __future__ import annotations

import argparse
import math
import os
from collections.abc import Mapping, Sequence
from dataclasses import dataclass

from .errors import BridgeConfigurationError

ENGINE_HOST = "127.0.0.1"
DEFAULT_PORT = 39100
DEFAULT_CONNECT_TIMEOUT_SECONDS = 1.0
DEFAULT_READ_TIMEOUT_SECONDS = 4.0
DEFAULT_MAX_RESPONSE_BYTES = 1024 * 1024
DEFAULT_LOG_LEVEL = "INFO"
VALID_LOG_LEVELS = frozenset({"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"})


@dataclass(frozen=True, slots=True)
class BridgeConfig:
  """Validated runtime settings."""

  Host: str = ENGINE_HOST
  Port: int = DEFAULT_PORT
  ConnectTimeoutSeconds: float = DEFAULT_CONNECT_TIMEOUT_SECONDS
  ReadTimeoutSeconds: float = DEFAULT_READ_TIMEOUT_SECONDS
  MaxResponseBytes: int = DEFAULT_MAX_RESPONSE_BYTES
  LogLevel: str = DEFAULT_LOG_LEVEL

  def __post_init__(Self) -> None:
    if Self.Host != ENGINE_HOST:
      raise BridgeConfigurationError("Host must be 127.0.0.1.")
    if isinstance(Self.Port, bool) or not 1 <= Self.Port <= 65535:
      raise BridgeConfigurationError("Port must be between 1 and 65535.")
    _validate_timeout(Self.ConnectTimeoutSeconds, "Connect timeout")
    _validate_timeout(Self.ReadTimeoutSeconds, "Read timeout")
    if isinstance(Self.MaxResponseBytes, bool) or Self.MaxResponseBytes <= 0:
      raise BridgeConfigurationError("Maximum response size must be positive.")
    NormalizedLevel = Self.LogLevel.upper()
    if NormalizedLevel not in VALID_LOG_LEVELS:
      raise BridgeConfigurationError(
        f"Log level must be one of: {', '.join(sorted(VALID_LOG_LEVELS))}."
      )
    object.__setattr__(Self, "LogLevel", NormalizedLevel)

  @property
  def BaseUrl(Self) -> str:
    return f"http://{Self.Host}:{Self.Port}/api/v1/"


def _validate_timeout(Value: float, Name: str) -> None:
  if isinstance(Value, bool) or not math.isfinite(Value) or Value <= 0:
    raise BridgeConfigurationError(f"{Name} must be a positive finite number.")


def _environment_value(Environment: Mapping[str, str], Name: str, Default: object) -> str:
  return Environment.get(Name, str(Default))


def load_config(
  Arguments: Sequence[str] | None = None,
  Environment: Mapping[str, str] | None = None,
) -> BridgeConfig:
  """Load command-line values over environment values over defaults."""

  ActiveEnvironment = os.environ if Environment is None else Environment
  Parser = argparse.ArgumentParser(description="BROCCOLI ENGINE MCP stdio bridge")
  Parser.add_argument(
    "--host",
    default=_environment_value(ActiveEnvironment, "BROCCOLI_MCP_HOST", ENGINE_HOST),
  )
  Parser.add_argument(
    "--port",
    type=int,
    default=_environment_value(ActiveEnvironment, "BROCCOLI_MCP_PORT", DEFAULT_PORT),
  )
  Parser.add_argument(
    "--connect-timeout",
    type=float,
    default=_environment_value(
      ActiveEnvironment,
      "BROCCOLI_MCP_CONNECT_TIMEOUT",
      DEFAULT_CONNECT_TIMEOUT_SECONDS,
    ),
  )
  Parser.add_argument(
    "--read-timeout",
    type=float,
    default=_environment_value(
      ActiveEnvironment,
      "BROCCOLI_MCP_READ_TIMEOUT",
      DEFAULT_READ_TIMEOUT_SECONDS,
    ),
  )
  Parser.add_argument(
    "--log-level",
    default=_environment_value(
      ActiveEnvironment,
      "BROCCOLI_MCP_LOG_LEVEL",
      DEFAULT_LOG_LEVEL,
    ),
  )
  try:
    Parsed = Parser.parse_args(Arguments)
    return BridgeConfig(
      Host=Parsed.host,
      Port=Parsed.port,
      ConnectTimeoutSeconds=Parsed.connect_timeout,
      ReadTimeoutSeconds=Parsed.read_timeout,
      LogLevel=Parsed.log_level,
    )
  except (TypeError, ValueError) as Error:
    if isinstance(Error, BridgeConfigurationError):
      raise
    raise BridgeConfigurationError("Configuration contains an invalid value.") from None
