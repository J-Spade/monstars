"""base play parameters for blanko"""
import ipaddress
import os
import pathlib
from typing import Any, Optional

from pydantic import Field

from monstars.model.types import Play


class BlankoPlay(Play):
    """plays available to Blanko"""

    dest_ip: ipaddress.IPv4Address = Field(
        required=True,
        description="IPv4 address of the host to send the play to",
    )

    dest_port: int = Field(
        default=53,
        description="UDP port on the host to send the play to",
    )

    listen_port: int = Field(
        default=1337,
        description="TCP port to use when listening for a response",
    )

    out_path: Optional[pathlib.Path] = Field(
        default=None,
        description="On-disk location to write response data",
    )
