# Contributing to libxmap

Thank you for your interest in contributing! We aim to keep `libxmap` fast, dependency-free, and stable.

## Development Workflow

1. Fork the repository.
2. Create a new branch: `git checkout -b feature/your-feature-name` or `fix/issue-description`.
3. Write your code. **Ensure C core remains pure C11 and the wrapper C++20.**
4. Write or update tests in the `tests/` directory.
5. Run the test suite:
    ```bash
      cmake -B build -S . -DXMAP_BUILD_TESTS=ON
      cmake --build build
      ctest --test-dir build
    ```
6. Format your code using clang-format (we use the LLVM style provided in [.clang-format](.clang-format)).
7. Commit using Conventional Commits (e.g., feat: add hugepages support).
8. Push and open a Pull Request against the main branch.

## Rules

* **No external dependencies:** We will no accept PRs that introduce third-party libraries (except for test frameworks).
* **ABI Stability:** Changes to [`xmap.h`](xmap.h) must not break binary compatibility.