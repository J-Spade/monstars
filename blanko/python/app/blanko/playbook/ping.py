"""Check in on Blanko"""
from typing import Any

from blanko.controller.commands import cmd_ping

from .base import BlankoPlay


class Ping(BlankoPlay):
    """PING"""
    name: str = "ping"
    description: str = "Check in on Blanko"
    return_type: Any = bool

    def slam(self) -> bool:
        return cmd_ping(
            dest_ip=self.dest_ip,
            dest_port=self.dest_port,
            listen_port=self.listen_port,
        )
