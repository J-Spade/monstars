#!/usr/bin/env python
"""Django's command-line utility for administrative tasks."""
import os
import sys


def generate_secret_key():
    """Generates a secret key for Django to use"""
    try:
        from django.core.management.utils import get_random_secret_key
        from .settings import SECRET_KEY_FILE
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc

    with open(SECRET_KEY_FILE, "w") as key:
        key.write(get_random_secret_key())


def main():
    """Run administrative tasks."""
    os.environ.setdefault("DJANGO_SETTINGS_MODULE", "swackhammer.settings")
    try:
        from django.core.management import execute_from_command_line
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc
    execute_from_command_line(sys.argv)


if __name__ == "__main__":
    main()
