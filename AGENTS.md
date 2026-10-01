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
- Preserve the ordering of Apply, confirmation, and rollback. A timeout can mean
  a partially applied monitor change. Keep recovery state until confirmation or
  successful rollback, and prevent conflicting monitor operations from overlapping.
- Automated tests must use fake external programs and isolated configuration.
  Never change actual monitor settings or invoke real monitor/daemon tools in tests.

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
- In the final response, briefly state what changed, which container checks passed,
  and any remaining limitations or manual verification. Do not claim checks passed
  if they were skipped, failed, or blocked.
