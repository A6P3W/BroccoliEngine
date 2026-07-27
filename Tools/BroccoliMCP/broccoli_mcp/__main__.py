"""Command-line entry point for python -m broccoli_mcp."""

from __future__ import annotations

import logging
import sys

from .config import load_config
from .errors import BridgeConfigurationError
from .logging_config import configure_logging
from .server import run_server


def main() -> int:
  try:
    Config = load_config()
  except BridgeConfigurationError as Error:
    configure_logging("ERROR")
    logging.getLogger(__name__).error("%s", Error)
    return 2

  configure_logging(Config.LogLevel)
  try:
    run_server(Config)
  except KeyboardInterrupt:
    logging.getLogger(__name__).info("Bridge interrupted")
  except Exception:
    logging.getLogger(__name__).exception("Bridge terminated unexpectedly")
    return 1
  return 0


if __name__ == "__main__":
  sys.exit(main())
