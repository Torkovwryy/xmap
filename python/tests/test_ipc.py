import numpy as np
import pytest

from python.xmap import SharedMemory, Mode, IpcFlags


@pytest.fixture
def shm_name():
    name = "/xmap_python_test_ipc"
    yield name
    try:
        SharedMemory.unlink(name)
    except Exception:
        pass


def test_ipc_zero_copy_communication(shm_name):
    size = 4096

    # Processo/Instância A cria a memória
    mem_a = SharedMemory(shm_name, size, Mode.ReadWrite, IpcFlags.CreateIfMissing)
    arr_a = mem_a.as_array()

    mem_b = SharedMemory(shm_name, size, Mode.ReadWrite, IpcFlags.OpenExisting)
    arr_b = mem_b.as_array()

    arr_a[0:5] = [72, 69, 76, 76, 79]

    assert list(arr_b[0:5]) == [72, 69, 76, 76, 79]

    mem_a.close()
    mem_b.close()


def test_ipc_unlink_behavior(shm_name):
    mem = SharedMemory(shm_name, 1024)
    mem.close()
    assert SharedMemory.unlink(shm_name) is True

    import xmap_ext
    with pytest.raises(RuntimeError):
        SharedMemory(shm_name, 1024, Mode.ReadWrite, IpcFlags.OpenExisting)
