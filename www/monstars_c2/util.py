"""utility functions"""
import os
import uuid


def new_prize(filename):
    """generates a new prize filepath in the static file directory"""
    prize_uri = os.path.sep.join(("prizes", str(uuid.uuid4())))
    dest_dir = os.path.sep.join((os.path.dirname(__file__), prize_uri))
    os.makedirs(dest_dir)
    dest_path = os.path.sep.join((dest_dir, filename))
    return dest_path, prize_uri


def get_prize(prize_id):
    """retrives the path to a prize in the prize directory"""
    prize_dir = os.path.sep.join((os.path.dirname(__file__), "prizes", prize_id))
    prize = os.listdir(prize_dir)[0]
    return os.path.sep.join((prize_dir, prize))
