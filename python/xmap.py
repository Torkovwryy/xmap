import numpy as np
from pathlib import Path

import xmap_ext

Mode = xmap_ext.Mode
Flags = xmap_ext.Flags
IpcFlags = xmap_ext.IpcFlags


class MemoryMap(xmap_ext.MemoryMap):
    """High-level wrapper for Memory-Mapped files with Zero-Copy support."""

    def __init__(self, filepath, mode=Mode.ReadOnly, flags=Flags.None_):
        str_path = str(filepath) if isinstance(filepath, Path) else filepath
        super().__init__(str_path, mode, flags)

    def as_array(self, dtype=np.uint8):
        """Returns a Zero-Copy NumPy array backed by the memory map."""
        return np.frombuffer(self, dtype=dtype)


class SharedMemory(xmap_ext.SharedMemory):
    """High-level wrapper for Inter-Process Communication (IPC) memory segments."""

    def __init__(self, name, size, mode=Mode.ReadWrite, flags=IpcFlags.CreateIfMissing):
        super().__init__(name, size, mode, flags)

    def as_array(self, dtype=np.uint8):
        """Returns a Zero-Copy NumPy array backed by the shared memory segment."""
        return np.frombuffer(self, dtype=dtype)
