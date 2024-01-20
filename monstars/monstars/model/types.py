"""Abstract types for Monstars"""

from typing import Any, List, Optional

from pydantic import BaseModel, Field


class Play(BaseModel):
    """A play in the Monstars playbook"""
    name: str
    description: str
    return_type: Any

    def slam(self) -> Any:
        """Run the play!"""
        pass


class Player(BaseModel):
    """A player on the Monstars basketball team"""
    name: str
    playbook: List[Play]
