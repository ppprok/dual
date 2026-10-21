Closes #

## Description of changes

<!-- As a list: what changed in the code; in plain language: what changes in the behavior of the algorithms/API. -->

## Manual verification instructions

<!-- Unneeded parts can be removed. -->

### 1. Build

1. `cmake -S . -B build && cmake --build build -j`

### 2. Test cases and how to reproduce them

1. `cd build/src/tests && ./TestDual && ./TestUtils`

### 3. Expected results

1.

---

## 📋 Author checklist

**PR:**

- [ ] Title `[#N] - type - Title`, `Closes #N` in the body
- [ ] Branch `type/N_description`, commits `[#N] type: Description`
- [ ] Task label switched to `In review`
- [ ] Issue, PR, commits and docs are in English

**Code:**

- [ ] The project builds without errors
- [ ] `TestDual` and `TestUtils` pass
- [ ] New/changed files are added to the corresponding `CMakeLists.txt`
- [ ] Changed C++ is run through `clang-format`
- [ ] No debug output and no TODO/FIXME without a task number
- [ ] If the C ABI of `librunc.so` changed — `src/pyrunc/runc.py` is updated

**Documentation:**

- [ ] Complex logic has comments
- [ ] `README.md` / `CLAUDE.md` are updated (if needed)
