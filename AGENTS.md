# Instructions for agents working on wayrandr

These instructions apply to the entire repository. Read them before making changes.
Follow any more specific `AGENTS.md` in the directory you are editing as well.

## Scope and existing work

- Keep changes focused on the requested task. Avoid unrelated refactoring and new
  abstractions when a straightforward solution is sufficient.
- Target the latest stable Fedora and modern hardware. Assume current kernels,
  libraries and tools, and use their APIs directly. Do not add compatibility layers
  or fallbacks for obsolete environments unless explicitly requested. Prefer small,
  direct solutions over speculative backward compatibility.
- Inspect the existing diff before editing. Preserve the user's unfinished changes;
  do not overwrite, revert, or format unrelated work.
- Do not stage, commit, or push unless the user explicitly requests it. Leave the
  resulting diff ready for review.
- Keep this document limited to durable, project-wide guidance for agents.
  Record individual changes in commit history, not here.

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
  Program/argument strings must be UTF-8 without NUL. Preserve each daemon client's
  argument boundaries and option handling.
- Preserve the ordering of Apply, confirmation, and rollback. A timeout can mean
  a partially applied monitor change. Keep recovery state until confirmation or
  successful rollback, and prevent conflicting monitor operations from overlapping.

## Configuration editing

- Keep shared profile operations in `ProfileEditorBackend` and native format conversions
  at loading/saving boundaries. Preserve backend-specific matching semantics.
- Use the daemon's native grammar and validation. Reject unsupported or ambiguous input
  rather than silently normalizing it. Preserve supported directives, order and unset
  optional values through unrelated edits; disclose formatting/comment changes on save.
- Loading/editing must not expand includes or execute hooks. Saving may reload a daemon
  and trigger configured exec hooks; keep disk-save and daemon-reload results separate.
- Carry the original loaded file snapshot through edits and asynchronous saves. Never
  replace the expected snapshot with a fresh disk read to bypass conflict detection.
- Use the shared saver in `src/utils/config_file.hpp`; preserve atomic replacement,
  conflict checks, symlink targets and file permissions/ownership, including ACLs.
  Validate content before writing. Preserve newer drafts when applying save results.

## Container-only development and verification

- Run builds, tests, pre-commit, formatters, linters, and application execution
  inside the project's Podman container. Do not install dependencies or run these
  tools directly on the host. Host file editing and Git inspection are allowed.
  Use `make container-*` entry points rather than direct Podman commands.
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
- Check new, untracked files explicitly with
  `make container-pre-commit PRE_COMMIT_FILES="<paths>"`. Do not stage them for checks.
- Container builds use a separate named volume. Do not reuse the host CMake cache
  or switch to the exported binary via `run-from-container-locally` for verification.
- Consult `Makefile`, `Containerfile` and `CMakeLists.txt` for target options,
  installation settings and dependencies; avoid duplicating their inventories here.
- In the final response, briefly state what changed, which container checks passed,
  and any remaining limitations or manual verification. Do not claim checks passed
  if they were skipped, failed, or blocked.

## Test infrastructure

- Keep test additions proportional to the behavioral change and its risk. Inspect existing
  coverage first; prefer extending existing cases and reusing fixtures/fakes. Add the smallest
  regression that fails before the fix and verifies the observable result afterward.
- Each additional scenario, backend variant or test layer must cover a distinct failure risk.
  Avoid repeating the same workflow across C++ and QML or adding speculative edge-case matrices.
  Use real QML interactions when the defect concerns UI behavior.
- Keep test setup direct; do not build general test frameworks for individual fixes.
  Documentation, formatting and changes without behavioral impact need no new tests.
- Use fake external programs and isolated files/environment. Never change actual
  monitor settings or invoke real monitor/daemon tools in automated tests.
- Only `container-config-check` may invoke real configuration parsers. Use its container
  without host display/runtime mounts; never apply monitor settings.
- Do not add placeholder tests or README inventories under `tests`.
- Link application code through `wayrandr_lib`. Register C++ sources explicitly in
  `tests/CMakeLists.txt`; QML scenarios use `tst_*.qml`. Fuzz targets are outside CTest.
- Each case owns isolated files/environment and drains asynchronous work before
  fixture destruction. Use Qt Test assertions only on the GUI thread; worker results
  and errors must be delivered there. Shutdown tests require a separate executable.
- Filter with `TEST_LABEL` / `TEST_REGEX`; use `container-coverage`,
  `container-sanitizers` and `container-fuzz-build` for separate instrumentation caches.
- Keep Qt Test/Quick Test hooks and libFuzzer entry points visible to their runtimes
  and retain their required names when applying style fixes.
