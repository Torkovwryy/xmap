from enum import Enum
from typing import Any, Optional, Type, Union


class Mode(Enum):
    ReadOnly: Mode
    ReadWrite: Mode


class Flags(Enum):
    None_: Flags
    HugePages: Flags
    Populate: Flags


class IpcFlags(Enum):
    CreateIfMissing: IpcFlags
    OpenExisting: IpcFlags


class MemoryMap:
    def __init__(self, filepath: str, mode: Mode = ..., flags: Flags = ...) -> None: ...

    def close(self) -> None: ...

    def flush(self, async_flush: bool = ...) -> bool: ...

    @property
    def size(self) -> int: ...

    @property
    def is_valid(self) -> bool: ...

    def __enter__(self) -> 'MemoryMap': ...

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None: ...


class SharedMemory:
    def __init__(self, name: str, size: int, mode: Mode = ..., flags: IpcFlags = ...) -> None: ...

    @staticmethod
    def unlink(name: str) -> bool: ...

    def close(self) -> None: ...

    @property
    def size(self) -> int: ...

    @property
    def is_valid(self) -> bool: ...
