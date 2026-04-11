from pathlib import Path

import xmap_ext


class MemoryMap(xmap_ext.MemoryMap):
    def __init__(self, filepath, mode=Mode.ReadOnly, flags=Flags.None_):
        # Converte Pathlib para string antes de enviar para o C++
        str_path = str(filepath) if isinstance(filepath, Path) else filepath
        super().__init__(str_path, mode, flags)

    """High-level wrapper for Memory-Mapped files with Zero-Copy support."""
    pass


class SharedMemory(xmap_ext.SharedMemory):
    """High-level wrapper for Inter-Process Communication (IPC) memory segments."""
    pass
