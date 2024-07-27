from importlib import resources
import os
from typing import List

from .static.blanko import binaries

BLANKO_INSTALLER_DIR = resources.files(binaries)

