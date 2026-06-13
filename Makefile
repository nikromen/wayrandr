BUILD_DIR ?= build
NPROC ?= $(shell nproc)
DEBUG_LOGS ?= ON
BUILD_TESTING ?= ON
TEST_LABEL ?=
TEST_REGEX ?=
TEST_JOBS ?= 1
PRE_COMMIT_FILES ?=
export PRE_COMMIT_HOME ?= $(CURDIR)/.cache/pre-commit
CMAKE_FLAGS ?=
FUZZ_TARGET ?=
FUZZ_SECONDS ?= 60
ARTIFACT_DIR ?= build/artifacts
CONTAINER_IMAGE ?= localhost/wayrandr:latest
CONTAINER_BUILD_FLAGS ?=
CONTAINER_BUILD_VOLUME ?= wayrandr-build
CONTAINER_PRE_COMMIT_VOLUME ?= wayrandr-precommit-cache
CONTAINER_ARTIFACT_DIR ?= $(BUILD_DIR)/from-container
CONTAINER_WORKSPACE = -v "$(CURDIR):/workspace:z" \
                     -v "$(CONTAINER_BUILD_VOLUME):/workspace/build:z" -w /workspace
CONTAINER_BUILD_ENV = -e BUILD_DIR=build -e NPROC="$(NPROC)" \
                      -e DEBUG_LOGS="$(DEBUG_LOGS)" -e BUILD_TESTING="$(BUILD_TESTING)" \
                      -e TEST_LABEL="$(TEST_LABEL)" -e TEST_REGEX="$(TEST_REGEX)" \
                      -e TEST_JOBS="$(TEST_JOBS)" -e CMAKE_FLAGS="$(CMAKE_FLAGS)"
CONTAINER_DISPLAY = -e DISPLAY -e WAYLAND_DISPLAY -e XDG_RUNTIME_DIR \
                    $(if $(XDG_RUNTIME_DIR),-v "$(XDG_RUNTIME_DIR):$(XDG_RUNTIME_DIR)") \
                    $(if $(wildcard /tmp/.X11-unix),-v /tmp/.X11-unix:/tmp/.X11-unix:ro)

# Normal build targets

build:
	@cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_DEBUG_LOGS=$(DEBUG_LOGS) \
		-DBUILD_TESTING=$(BUILD_TESTING) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_FLAGS)
	@cmake --build "$(BUILD_DIR)" -j$(NPROC)

install:
	@cmake --install "$(BUILD_DIR)"

clean:
	@rm -rf -- "$(BUILD_DIR)" build/coverage build/sanitizers build/fuzz "$(ARTIFACT_DIR)"

clean-cache:
	@rm -rf -- "$(CURDIR)/.cache"

run:
	@"$(BUILD_DIR)/wayrandr"

test:
	@$(MAKE) build BUILD_TESTING=ON
	@mkdir -p "$(ARTIFACT_DIR)"
	@ctest --test-dir "$(BUILD_DIR)" --output-on-failure -j$(TEST_JOBS) \
		$(if $(TEST_LABEL),-L "$(TEST_LABEL)") $(if $(TEST_REGEX),-R "$(TEST_REGEX)") \
		--no-tests=error --output-log "$(abspath $(ARTIFACT_DIR))/ctest.log" \
		--output-junit "$(abspath $(ARTIFACT_DIR))/ctest.xml"

rebuild:
	@$(MAKE) clean
	@$(MAKE) build

# Keep steps sequential even when the caller uses make -j; full runs ignore filters.
test-all:
	@$(MAKE) clean
	@$(MAKE) clean-cache
	@$(MAKE) pre-commit BUILD_TESTING=ON CMAKE_FLAGS= PRE_COMMIT_FILES=
	@$(MAKE) build BUILD_TESTING=ON CMAKE_FLAGS=
	@$(MAKE) test TEST_LABEL= TEST_REGEX= CMAKE_FLAGS=
	@$(MAKE) config-check CMAKE_FLAGS=
	@$(MAKE) coverage TEST_LABEL= TEST_REGEX=
	@$(MAKE) sanitizers TEST_LABEL= TEST_REGEX=
	@$(MAKE) fuzz-replay FUZZ_TARGET=kanshi_config
	@$(MAKE) fuzz FUZZ_TARGET=kanshi_config

pre-commit:
	@$(MAKE) build
	@cmake --build "$(BUILD_DIR)" --target tst_config_compatibility -j$(NPROC)
	@if [ -n "$(PRE_COMMIT_FILES)" ]; then \
		pre-commit run --files $(PRE_COMMIT_FILES); \
	else \
		pre-commit run --all-files && \
		git ls-files --others --exclude-standard -z | xargs -0 -r pre-commit run --files; \
	fi

run-from-container-locally:
	@if [ ! -x "$(CONTAINER_ARTIFACT_DIR)/wayrandr" ]; then \
		echo "Run make container-build first to export the executable." >&2; exit 1; \
	fi
	@"$(CONTAINER_ARTIFACT_DIR)/wayrandr"

config-check:
	@cmake -S . -B "$(BUILD_DIR)" -DBUILD_TESTING=ON
	@cmake --build "$(BUILD_DIR)" --target tst_config_compatibility -j$(NPROC)
	@mkdir -p "$(ARTIFACT_DIR)/config-compatibility"
	@rpm -q kanshi > "$(ARTIFACT_DIR)/config-compatibility/versions.txt"
	@auto-wlr-randrctl --version >> "$(ARTIFACT_DIR)/config-compatibility/versions.txt"
	@QT_QPA_PLATFORM=offscreen "$(BUILD_DIR)/tests/tst_config_compatibility" \
		-o "$(ARTIFACT_DIR)/config-compatibility/results.xml",junitxml \
		-o "$(ARTIFACT_DIR)/config-compatibility/results.txt",txt -o -,txt

coverage:
	@$(MAKE) build BUILD_DIR=build/coverage CMAKE_FLAGS="-DWAYRANDR_COVERAGE=ON" BUILD_TESTING=ON
	@find build/coverage -name '*.gcda' -delete
	@$(MAKE) test BUILD_DIR=build/coverage CMAKE_FLAGS="-DWAYRANDR_COVERAGE=ON" \
		ARTIFACT_DIR=build/artifacts/coverage
	@gcovr --root . --filter 'src/' --exclude '.*/.*_autogen/.*' \
		--exclude '.*/qrc_.*' --exclude '.*/_deps/.*' \
		--html-details build/artifacts/coverage/index.html \
		--cobertura build/artifacts/coverage/coverage.xml --print-summary build/coverage

sanitizers:
	@$(MAKE) test BUILD_DIR=build/sanitizers \
		CMAKE_FLAGS="-DCMAKE_CXX_COMPILER=clang++ -DWAYRANDR_SANITIZERS=ON" \
		ARTIFACT_DIR=build/artifacts/sanitizers

fuzz-build:
	@$(MAKE) build BUILD_DIR=build/fuzz BUILD_TESTING=ON \
		CMAKE_FLAGS="-DCMAKE_CXX_COMPILER=clang++ -DWAYRANDR_FUZZING=ON"

fuzz: fuzz-build
	@if [ -z "$(FUZZ_TARGET)" ] || ! grep -Fxq -- "$(FUZZ_TARGET)" build/fuzz/fuzz-targets.txt; then \
		echo "Select a registered FUZZ_TARGET. Registered targets:" >&2; \
		cat build/fuzz/fuzz-targets.txt >&2; exit 1; \
	fi
	@case "$(FUZZ_SECONDS)" in ''|*[!0-9]*|0) \
		echo "FUZZ_SECONDS must be a positive integer" >&2; exit 1;; esac
	@test "$(FUZZ_SECONDS)" -gt 0
	@mkdir -p "build/artifacts/fuzz/$(FUZZ_TARGET)/corpus" \
		"build/artifacts/fuzz/$(FUZZ_TARGET)/findings"
	@set -- "build/artifacts/fuzz/$(FUZZ_TARGET)/corpus"; \
		if [ -d "tests/fuzz/corpus/$(FUZZ_TARGET)" ]; then \
			set -- "$$@" "tests/fuzz/corpus/$(FUZZ_TARGET)"; \
		fi; \
		ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		"build/fuzz/tests/$(FUZZ_TARGET)" "$$@" -max_total_time="$(FUZZ_SECONDS)" \
			-max_len=65536 -timeout=2 -rss_limit_mb=512 \
			-artifact_prefix="build/artifacts/fuzz/$(FUZZ_TARGET)/findings/" \
			>> "build/artifacts/fuzz/$(FUZZ_TARGET)/fuzz.log" 2>&1

# Replay checked-in seeds individually; generated findings stay in the artifact volume.
fuzz-replay: fuzz-build
	@if [ -z "$(FUZZ_TARGET)" ] || ! grep -Fxq -- "$(FUZZ_TARGET)" build/fuzz/fuzz-targets.txt; then \
		echo "Select a registered FUZZ_TARGET" >&2; exit 1; \
	fi
	@test -d "tests/fuzz/corpus/$(FUZZ_TARGET)"
	@mkdir -p "build/artifacts/fuzz/$(FUZZ_TARGET)/findings"
	@ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		"build/fuzz/tests/$(FUZZ_TARGET)" tests/fuzz/corpus/$(FUZZ_TARGET)/* \
		-max_len=65536 -timeout=2 -rss_limit_mb=512 \
		-artifact_prefix="build/artifacts/fuzz/$(FUZZ_TARGET)/findings/" \
		>> "build/artifacts/fuzz/$(FUZZ_TARGET)/replay.log" 2>&1

# Container targets
# CMake caches contain absolute paths; keep container builds separate from host builds.

# The project cache is shared through the workspace bind mount.
container-clean-cache: clean-cache

container-build-image:
	@podman build $(CONTAINER_BUILD_FLAGS) -t "$(CONTAINER_IMAGE)" -f Containerfile .

container-test-all:
	@$(MAKE) container-clean
	@$(MAKE) container-clean-cache
	@$(MAKE) container-build-image CONTAINER_BUILD_FLAGS="--no-cache --pull=always"
	@$(MAKE) container-pre-commit BUILD_TESTING=ON CMAKE_FLAGS= PRE_COMMIT_FILES=
	@$(MAKE) container-build BUILD_TESTING=ON CMAKE_FLAGS=
	@$(MAKE) container-test TEST_LABEL= TEST_REGEX= CMAKE_FLAGS=
	@$(MAKE) container-config-check CMAKE_FLAGS=
	@$(MAKE) container-coverage TEST_LABEL= TEST_REGEX=
	@$(MAKE) container-sanitizers TEST_LABEL= TEST_REGEX=
	@$(MAKE) container-fuzz-replay FUZZ_TARGET=kanshi_config
	@$(MAKE) container-fuzz FUZZ_TARGET=kanshi_config
	@$(MAKE) container-export-artifacts

container-remove-image:
	@podman rmi "$(CONTAINER_IMAGE)"

container-clean:
	@podman rmi --ignore "$(CONTAINER_IMAGE)"
	@for volume in "$(CONTAINER_BUILD_VOLUME)" "$(CONTAINER_PRE_COMMIT_VOLUME)"; do \
		if podman volume exists "$$volume"; then \
			podman volume rm "$$volume" || exit $$?; \
		else \
			# exit code 1 means the volume is absent; propagate other podman errors \
			status=$$?; [ "$$status" -eq 1 ] || exit "$$status"; \
		fi; \
	done
	@rm -rf -- "$(CONTAINER_ARTIFACT_DIR)"

container-build:
	@mkdir -p "$(CONTAINER_ARTIFACT_DIR)"
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-v "$(abspath $(CONTAINER_ARTIFACT_DIR)):/container-artifacts:z" \
		"$(CONTAINER_IMAGE)" sh -c 'make build && \
			cp build/wayrandr /container-artifacts/wayrandr.tmp && \
			mv -f /container-artifacts/wayrandr.tmp /container-artifacts/wayrandr'

container-test:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make test

container-install:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-e DESTDIR="$(DESTDIR)" "$(CONTAINER_IMAGE)" make install

container-run:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		$(CONTAINER_DISPLAY) \
		--network host --security-opt label=disable \
		"$(CONTAINER_IMAGE)" sh -c 'make build && make run'

container-shell:
	@podman run --rm --pull=never -it $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		$(CONTAINER_DISPLAY) \
		--network host --security-opt label=disable \
		"$(CONTAINER_IMAGE)" /bin/bash

# No display sockets or host runtime directories are mounted for validation.
container-config-check:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make config-check

container-pre-commit:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-v "$(CONTAINER_PRE_COMMIT_VOLUME):/root/.cache/pre-commit:z" \
		-e PRE_COMMIT_HOME=/root/.cache/pre-commit \
		-e PRE_COMMIT_FILES="$(PRE_COMMIT_FILES)" \
		"$(CONTAINER_IMAGE)" make pre-commit

# Independent configurations share the container volume, never a CMake cache.
container-coverage:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make coverage

container-sanitizers:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make sanitizers

container-fuzz-build:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make fuzz-build

container-fuzz:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-e FUZZ_TARGET="$(FUZZ_TARGET)" -e FUZZ_SECONDS="$(FUZZ_SECONDS)" \
		"$(CONTAINER_IMAGE)" make fuzz

# Export logs/reports from the named build volume for local review and CI upload.
container-export-artifacts:
	@mkdir -p "$(CONTAINER_ARTIFACT_DIR)/reports"
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) \
		-v "$(abspath $(CONTAINER_ARTIFACT_DIR))/reports:/reports:z" \
		"$(CONTAINER_IMAGE)" sh -c 'if [ -d build/artifacts ]; then cp -r build/artifacts/. /reports/; fi'

container-fuzz-replay:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-e FUZZ_TARGET="$(FUZZ_TARGET)" "$(CONTAINER_IMAGE)" make fuzz-replay

.PHONY: build install clean clean-cache run test rebuild test-all pre-commit run-from-container-locally \
        config-check coverage sanitizers fuzz-build fuzz fuzz-replay \
        container-clean-cache container-build-image container-remove-image container-clean \
        container-build container-install container-test container-test-all container-run container-shell \
        container-config-check \
        container-pre-commit container-coverage container-sanitizers container-fuzz-build \
        container-fuzz container-export-artifacts container-fuzz-replay
