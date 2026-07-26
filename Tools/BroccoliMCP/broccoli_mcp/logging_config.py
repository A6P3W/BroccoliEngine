"""stderr-only diagnostic logging configuration."""

from __future__ import annotations

import logging
import sys


def configure_logging(LogLevel: str) -> None:
  """Configure diagnostics without writing to MCP's stdout channel."""

  Handler = logging.StreamHandler(sys.stderr)
  Handler.setFormatter(
    logging.Formatter(
      fmt="%(asctime)s %(levelname)s %(name)s %(message)s",
      datefmt="%Y-%m-%dT%H:%M:%S%z",
    )
  )
  RootLogger = logging.getLogger()
  RootLogger.handlers.clear()
  RootLogger.addHandler(Handler)
  RootLogger.setLevel(LogLevel)
