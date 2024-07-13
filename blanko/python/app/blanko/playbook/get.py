"""Have Blanko grab a file"""
import pathlib
from typing import Any

from pydantic import Field

from blanko.controller.commands import cmd_get
from .base import BlankoPlay


class Get(BlankoPlay):
    """GET"""
    name: str = "get"
    description: str = "Have Blanko get a file"
    return_type: Any = bytes

    filepath: pathlib.PurePosixPath = Field(
        required=True,
        description="File to get",
    )

    def slam(self) -> bytes:
        file_data = cmd_get(
            dest_ip=self.dest_ip,
            dest_port=self.dest_port,
            listen_port=self.listen_port,
            path=self.filepath,
        )
        if self.out_path:
            with self.out_path.open("wb") as out:
                out.write(file_data)
        return file_data
