"""Tell Blanko to run a shell command"""
from typing import Any, Tuple

from pydantic import Field

from blanko.controller.commands import cmd_exec
from .base import BlankoPlay


class Exec(BlankoPlay):
    """EXEC"""
    name: str = "exec"
    description: str = "Have Blanko run a command"
    return_type: Any = Tuple[int, str]

    commandline: str = Field(
        required=True,
        description="Commandline to run",
    )

    def slam(self) -> Tuple[int, str]:
        errno, output = cmd_exec(
            dest_ip=self.dest_ip,
            dest_port=self.dest_port,
            listen_port=self.listen_port,
            cmd=self.commandline,
        )
        if self.out_path:
            with self.out_path.open("w") as out:
                out.write(output)
        return errno, output
