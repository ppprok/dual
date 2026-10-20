---
name: build-and-test
description: Сборка и тесты этого репо (CMake presets dev/release/asan, ctest, doctest, санитайзеры, проверка find_package). Триггеры — «собери», «прогони тесты», «запусти тест», «проверь под санитайзерами», «ASan», «почему не собирается», «проверь перед PR», «compile_commands», «clangd не видит».
---

# Сборка и тесты

Инструменты — `cmake`, `ninja`, `clang-format`, `clang-tidy`, `clangd` — стоят через `uv tool` в `~/.local/bin`.
Нет какого-то — `uv tool install <name>`, не `apt`. Компилятор — g++ 11 (clang-компилятора нет).
Внешних зависимостей нет (Boost и spdlog не нужны); doctest скачивается `FetchContent` при первом configure — нужна сеть.

## Основной цикл

```bash
cmake --preset dev && cmake --build --preset dev && ctest --preset dev   # build/dev, Debug
ctest --preset dev -R DualizeRUNC                                        # подмножество по regex
ctest --preset dev --output-on-failure -R 'TestUtils\.read_bit_matrix'   # один тест с выводом
```

Presets: `dev` (Debug), `release`, `asan` (RelWithDebInfo + ASan/UBSan, `DUAL_ENABLE_SANITIZERS=ON`).
Каждый doctest-кейс — отдельный CTest-тест `TestDual.<case>` / `TestUtils.<case>`.

Вручную, с опциями doctest (`-tc`, `-s`, `-d`): **только из каталога тестов** — бинарники открывают
`data/...` по относительному пути, данные туда копирует таргет `dual_test_data`:

```bash
cd build/dev/src/tests && ./TestDual -tc="DualizeRUNCTest4" -s
```

В git-worktree preset кладёт сборку в `<worktree>/build/<preset>` — у каждого worktree своя сборка,
`build/` в `.gitignore`.

## Известное состояние (обновлять при исправлении)

- `TestUtils.load_CSV_and_create_columns` падает — давний дефект, разбирается в аудите #11.
- Под `asan` падают тесты RUNC и IO `bit_matrix` (heap-buffer-overflow / use-after-free) — давние дефекты.
- С `-Wall -Wextra` много предупреждений, поэтому `DUAL_WARNINGS_AS_ERRORS` выключен.

Сравнивайте с этим списком: новое падение — ваше, старое — не чинить попутно, а упомянуть в PR.

## Перед PR

1. `ctest --preset dev` — падает не больше, чем в списке выше.
2. Трогали алгоритмы или bit-контейнеры — ещё `cmake --preset asan && cmake --build --preset asan && ctest --preset asan`
   и сравнить число падений с `master`.
3. `clang-format -i` на изменённые `.cpp/.h` (конфиг `.clang-format`; `legacy/` не форматировать).
4. Меняли CMake-экспорт или публичные заголовки — проверить потребителя:
   `cmake --install build/release --prefix /tmp/<scratch>/prefix`, затем мини-проект с
   `find_package(dual REQUIRED)` + `target_link_libraries(app PRIVATE dual::dual)` и `-DCMAKE_PREFIX_PATH=<prefix>`.
5. Добавили `.cpp/.h` — внесите в список исходников своего `CMakeLists.txt`, иначе файл не собирается
   и clangd его не видит.

## Корректность алгоритмов

Ожидаемое число минимальных покрытий зашито в имя файла `src/tests/data/<rows>_<cols>_<count>.hg`.
Варианты `OPT`, `RUNC`, `RUNCM` обязаны давать одно и то же множество покрытий — при правке алгоритма
сравнивайте их между собой на случайных матрицах и с перебором на малых `n`.

## clangd

`.clangd` указывает на `build/dev/compile_commands.json` — достаточно один раз `cmake --preset dev`.
