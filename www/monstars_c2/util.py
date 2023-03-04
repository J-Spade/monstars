"""utility functions"""
import os
import uuid


def new_prize(filename):
    """generates a new prize filepath in the static file directory"""
    static_dir = os.path.sep.join(("prizes", str(uuid.uuid4())))
    dest_dir = os.path.sep.join((os.path.dirname(__file__), "static", static_dir))
    os.mkdir(dest_dir)
    dest_path = os.path.sep.join((dest_dir, filename))
    dest_uri = os.path.sep.join((static_dir, filename))
    return dest_path, dest_uri
