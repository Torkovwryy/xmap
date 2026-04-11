import numpy as np
import os
import pytest
from pathlib import Path

from xmap import MemoryMap, Mode, Flags


@pytest.fixture
def temp_file(tmp_path):
    file_path = tmp_path / "test_data.bin"
    # Pré-aloca 1MB de dados
    file_path.write_bytes(b'\x00' * (1024 * 1024))
    yield file_path
    if file_path.exists():
        file_path.unlink()


def test_memory_map_read_write(temp_file):
    with MemoryMap(temp_file, Mode.ReadWrite) as m:
        assert m.size == 1024 * 1024
        assert m.is_valid is True

        arr = m.as_array()
        arr[0:4] = [222, 173, 190, 239]
        m.flush()

    with open(temp_file, "rb") as f:
        magic = f.read(4)
        assert magic == bytes([222, 173, 190, 239])


def test_readonly_protection(temp_file):
    with MemoryMap(temp_file, Mode.ReadOnly) as m:
        arr = m.as_array()
        with pytest.raises(ValueError, match="is read-only"):
            arr[0] = 255
