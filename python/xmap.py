import xmap_ext


class MemoryMap(xmap_ext.MemoryMap):
    """High-level wrapper for Memory-Mapped files with Zero-Copy support."""
    pass


class SharedMemory(xmap_ext.SharedMemory):
    """High-level wrapper for Inter-Process Communication (IPC) memory segments."""
    pass
