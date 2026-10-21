---
title: CppFormatting
type: guide
permalink: betterspray/cpp-formatting
---

# C/C++ formatting

Formatting notes for this repository. The aggregator keeps the same note as
`metahooksv/cpp-formatting`; this copy carries the standalone paths.

## Toolchain and style

Formatting uses
[MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation) and
clang-format **23.1.3**, with the DiligentCore style (4 spaces, preserved include
order). Install the formatter for the Python interpreter used by CMake:

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

The format-only configuration needs CMake 3.21+, Git, Python 3.9+ (CI uses 3.12),
and a build generator; `-G Ninja` works without Visual Studio. It prepares no
native SDK or game dependencies. Formatting targets are explicit and are not part
of a normal DLL build. With a Visual Studio generator, add `--config Debug` or
`--config Release` when building a formatting target.

`format-check` and `format` are the top-level aliases; this component also exposes
`BetterSpray-format-check` and `BetterSpray-format`.

## Source path resolution

`cmake/FormatValidation.cmake` accepts `FORMAT_VALIDATION_SOURCE_PATH` as a CMake
variable or environment variable. When it is empty, FetchContent downloads the
retained tooling commit `13c9fabe058e1f887ad1b03bb6884de911192c6a` (`v1.0.0`), which must not move.

Quote relative paths, for example
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`.
Configuration generates the gitignored root `.clang-format`; change the shared
style rather than that generated copy. An optional
`FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE` selects an explicit formatter, whose
version must still match the pin.

## Checked scope

Checks cover owned C/C++ files under `src/`, `include/` and `tests/`, including
non-ignored new files. Repository-relative exclusions live in
`.clang-format-ignore`; external SDK trees, third-party sources and build artifacts
are excluded. `.github/workflows/clang-format.yml` runs the same full-scope check
on pushes, pull requests and manual runs.

## Known constraints

- Keep MSVC inline assembly inside explicit blocks; formatting unbraced assembly
  can absorb the following C++ statements.
- Do not hand-edit the generated `.clang-format`; change the shared FormatValidation
  style instead.
