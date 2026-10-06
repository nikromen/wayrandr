# Instructions for agents working on wayrandr

These instructions apply to the entire repository. Read them before making changes.
Follow any more specific `AGENTS.md` in the directory you are editing as well.

## Scope and existing work

- Keep changes focused on the requested task. Avoid unrelated refactoring and new
  abstractions when a straightforward solution is sufficient.
- Inspect the existing diff before editing. Preserve the user's unfinished changes;
  do not overwrite, revert, or format unrelated work.
- Do not stage, commit, or push unless the user explicitly requests it. Leave the
  resulting diff ready for review.
- If any new information due to some changes needs to be edited or added to this
  document, do so.

## Code style

- Follow `.clang-format`, `.clang-tidy`, and `.pre-commit-config.yaml`. Fix problems
  in the code rather than weakening checks to make them pass.
- Use explicit `if` branches instead of the ternary operator `?:` in C++ and QML.
  An early return is preferable to an unnecessary `else` after a return.
- Always use braces for `if`, `else`, `for`, `while`, and `do` bodies, including
  single-statement bodies.
- Use four spaces for C++ indentation, no tabs, and a 100-column limit. Let the
  configured formatter handle spacing and line wrapping.
- Use `CamelCase` for classes and structs, `snake_case` for C++ functions,
  namespaces, parameters, and variables, and `UPPER_CASE` for global constants.
  Follow the surrounding convention for members with a trailing underscore and
  for QML properties, Qt signals, and existing public interfaces.
- Prefer the existing `auto function(...) -> ReturnType` style for non-void
  functions. Keep code compatible with C++17, as configured in `CMakeLists.txt`.
- Include the headers directly needed by each file; do not rely on transitive
  includes. Use `const` and `[[nodiscard]]` where appropriate.
- Put helpers used only by one `.cpp` in its anonymous namespace. Reuse an existing
  anonymous namespace in that file. Do not add a generic utility for a helper
  specific to monitor data or one controller.
- Write code comments and identifiers in English. Explain contracts, ownership,
  threading, or non-obvious decisions rather than restating the code.

## Qt and external processes

- Keep GUI objects and their state on the GUI thread. Do not block that thread
  waiting for external processes. Use the existing `run_job()` / `run_command()`
  contracts documented in `src/utils/helpers.hpp`.
- Worker jobs must own or copy the data they need. Account for destroyed receivers
  and late results before updating GUI state.
- Pass the executable and arguments separately; do not introduce shell
  interpretation. Retain timeout, output limits, error reporting, and process cleanup.
- Availability and execution resolve programs with `QStandardPaths::findExecutable`.
  Program/argument strings must be UTF-8 without NUL. Keep daemon CLI argument
  boundaries: auto-wlr-randrctl uses `switch [--force] -- <profile>`; kanshictl
  consumes the profile directly and must not receive an extra `--`.
- Kanshi serialization targets libscfg syntax (verified with Kanshi 1.9.0).
  Quoted data cannot contain NUL or LF. Exec entries are individual scfg directives,
  not arbitrary config fragments; validate their boundaries with libscfg >= 0.2.0
  without filtering shell operators. The final validation parameter must remain
  separate to detect incomplete escapes and empty child blocks. Validate before
  opening the destination for writing. Saving reloads a
  running daemon and can intentionally trigger exec hooks; loading/editing does not.
- Configuration documents carry the exact loaded file snapshot through conversions and edits.
  Saving compares it under a lock on the resolved target and again before `QSaveFile::commit()`;
  never adopt a newly loaded snapshot as the editor's expected version during saving. Disable
  direct-write fallback. Symlinks keep pointing to their existing target; dangling links fail
  explicitly. Preserve ownership, group, mode and POSIX ACLs or fail; create new files with 0600.
  This is atomic replacement, not guaranteed power-loss durability, and non-cooperating writers
  can still race the final check/rename. Keep disk-save results separate from daemon reload
  results, and advance only the disk snapshot when newer edits exist at asynchronous completion.
- Kanshi parsing uses libscfg, as Kanshi 1.9.0 does. Reject unknown directives, duplicate
  profile names/outputs, repeated output options and includes following main-file definitions
  rather than silently normalizing potentially significant order/conflicts. Preserve anonymous
  profiles, `...output`, global defaults and includes through editor conversions. Never expand
  includes or execute commands while loading. Support only the latest auto-wlr-randr release;
  TOML schema checks were verified against 1.2.0. Do not add compatibility with older releases;
  reject unknown items and wrong types, and preserve profile declaration order. Preserve unset
  optional values during unrelated UI edits. Saving normalizes formatting/comments and scfg
  exec quoting; formatting and comment changes are disclosed in the editor.
- Profile list operations and live snapshots are shared in `ProfileEditorBackend`; native
  configuration conversions belong at loading/saving boundaries. Matching uses explicit
  `InvalidGlobBehavior`: Kanshi uses `LITERAL_NAME`, auto-wlr-randr uses `NO_MATCH`.
  Live-position lookup retains `NO_MATCH` for both backends.
  Auto-wlr-randr profile previews use the first unused connected output for each selector,
  in returned output order, without reassigning earlier matches (as in version 1.2.0).
- Preserve the ordering of Apply, confirmation, and rollback. A timeout can mean
  a partially applied monitor change. Keep recovery state until confirmation or
  successful rollback, and prevent conflicting monitor operations from overlapping.
- Automated tests must use fake external programs and isolated configuration.
  Never change actual monitor settings or invoke real monitor/daemon tools in tests.
- The separate `container-config-check` is authorized to invoke real config parsers:
  `auto-wlr-randrctl validate` and Kanshi with an absolute nonexistent Wayland socket.
  Keep it in the container without host display/runtime mounts; never apply monitor settings.

## Container-only development and verification

- Run builds, tests, pre-commit, formatters, linters, and application execution
  inside the project's Podman container. Do not install dependencies or run these
  tools directly on the host. Reading/editing files and inspecting Git state on
  the host is allowed; Do not run podman command directly if not necessary! Use
  `make container-*` commands launch the container tools.
- Use `Makefile` and `Containerfile`. If the image is missing, first run
  `make container-build-image`. Do not substitute a local build if Podman is unavailable;
  report the blocker and any checks that could not run.
- Before declaring a change complete, run all three commands in this order:

  ```sh
  make container-pre-commit
  make container-build
  make container-test
  ```

- Pre-commit can modify files. Review its changes and rerun it until it passes
  without further changes, then build and test the final version again.
- `container-pre-commit` uses `--all-files`, which only includes tracked files.
  Also check new, untracked source/test files with `pre-commit run --files <paths>`
  inside the container, using `make container-shell` or the same Podman mounts from
  the Makefile. Do not stage files just to include them in checks.
- Container builds use a separate named volume. Do not reuse the host CMake cache
  or switch to the exported binary via `run-from-container-locally` for verification.
- `container-install` installs an already built configuration with CMake and forwards
  `DESTDIR`. Set `CMAKE_INSTALL_PREFIX` through `CMAKE_FLAGS` when building. For isolated
  Release installation verification, use a separate `CONTAINER_BUILD_VOLUME`, build with
  `CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/wayrandr"`
  and `DEBUG_LOGS=OFF`, then run `container-install` with the same volume and
  `DESTDIR=/workspace/build/install-stage`. QML is embedded in the executable; runtime
  still requires Qt6 libraries/QML modules, libscfg >= 0.2.0 and wlr-randr. Kanshi,
  auto-wlr-randr and grim are optional tools for their respective features. Development
  headers, compilers, CMake and fetched header/static libraries are build requirements.
- In the final response, briefly state what changed, which container checks passed,
  and any remaining limitations or manual verification. Do not claim checks passed
  if they were skipped, failed, or blocked.

## Test infrastructure

- C++ component and integration tests share the `integration` label.
  Do not add placeholder tests or README inventories under `tests`.
- Link application code through `wayrandr_lib`. Register C++ sources explicitly in
  `tests/CMakeLists.txt`; QML scenarios use `tst_*.qml`. Fuzz targets are outside CTest.
- Each case owns isolated files/environment and drains asynchronous work before
  fixture destruction. Use Qt Test assertions only on the GUI thread; worker results
  and errors must be delivered there. Shutdown tests require a separate executable.
- Filter with `TEST_LABEL` / `TEST_REGEX`; use `container-coverage`,
  `container-sanitizers` and `container-fuzz-build` for separate instrumentation caches.
- Check untracked files with `container-pre-commit PRE_COMMIT_FILES="<paths>"`
  without staging. Export logs/reports with `container-export-artifacts`.
- Replay checked-in fuzz seeds with `container-fuzz-replay FUZZ_TARGET=kanshi_config`.
  Keep libFuzzer's required `LLVMFuzzerTestOneInput` symbol and Qt Quick Test setup
  hooks visible to their runtimes; use `Q_SLOT` on setup hooks rather than a redundant
  access section that clang-tidy can remove.
- `make container-test-all` removes the project image, build/pre-commit volumes, dependency
  cache and exported artifacts, rebuilds the image without layer cache, then runs all checks.
  `make test-all` provides the equivalent local workflow; agents must use the container variant.
  Full workflows ignore test filters and stop at the first failed check.
