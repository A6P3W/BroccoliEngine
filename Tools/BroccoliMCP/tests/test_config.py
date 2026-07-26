from __future__ import annotations

import math

import pytest

from broccoli_mcp.config import BridgeConfig, load_config
from broccoli_mcp.errors import BridgeConfigurationError


def test_default_config() -> None:
  Config = load_config([], {})

  assert Config.Host == "127.0.0.1"
  assert Config.Port == 39100
  assert Config.ConnectTimeoutSeconds == 1.0
  assert Config.ReadTimeoutSeconds == 4.0


def test_command_line_overrides_environment() -> None:
  Config = load_config(
    ["--port", "39102", "--log-level", "debug"],
    {"BROCCOLI_MCP_PORT": "39101"},
  )

  assert Config.Port == 39102
  assert Config.LogLevel == "DEBUG"


@pytest.mark.parametrize("Host", ["localhost", "0.0.0.0", "192.168.0.1"])
def test_external_or_alias_host_is_rejected(Host: str) -> None:
  with pytest.raises(BridgeConfigurationError):
    BridgeConfig(Host=Host)


@pytest.mark.parametrize("Port", [0, 65536, -1, True])
def test_invalid_port_is_rejected(Port: int) -> None:
  with pytest.raises(BridgeConfigurationError):
    BridgeConfig(Port=Port)


@pytest.mark.parametrize("Timeout", [0.0, -1.0, math.inf, math.nan, True])
def test_invalid_timeout_is_rejected(Timeout: float) -> None:
  with pytest.raises(BridgeConfigurationError):
    BridgeConfig(ConnectTimeoutSeconds=Timeout)
