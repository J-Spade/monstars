import io
import uuid

from django.core.files.storage import FileSystemStorage


def save_loot(filename: str, file_data: bytes, media_subdir: str = ""):
    storage = FileSystemStorage()
    file_id = uuid.uuid4()
    pathname = f"{media_subdir}{file_id}/{filename}"
    stored_name = storage.save(pathname, io.BytesIO(file_data))
    return storage.url(stored_name)


def delete_loot(uri: str):
    storage = FileSystemStorage()
    storage.delete(uri.lstrip(storage.base_url))
