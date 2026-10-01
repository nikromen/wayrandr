BUILD_DIR ?= build
NPROC ?= $(shell nproc)
DEBUG_LOGS ?= ON
BUILD_TESTING ?= ON
CONTAINER_IMAGE ?= localhost/wayrandr:latest
CONTAINER_BUILD_VOLUME ?= wayrandr-build
CONTAINER_PRE_COMMIT_VOLUME ?= wayrandr-precommit-cache
CONTAINER_ARTIFACT_DIR ?= $(BUILD_DIR)/from-container
CONTAINER_WORKSPACE = -v "$(CURDIR):/workspace:z" \
                     -v "$(CONTAINER_BUILD_VOLUME):/workspace/build:z" -w /workspace
CONTAINER_BUILD_ENV = -e BUILD_DIR=build -e NPROC="$(NPROC)" \
                      -e DEBUG_LOGS="$(DEBUG_LOGS)" -e BUILD_TESTING="$(BUILD_TESTING)"
CONTAINER_DISPLAY = -e DISPLAY -e WAYLAND_DISPLAY -e XDG_RUNTIME_DIR \
                    $(if $(XDG_RUNTIME_DIR),-v "$(XDG_RUNTIME_DIR):$(XDG_RUNTIME_DIR)") \
                    $(if $(wildcard /tmp/.X11-unix),-v /tmp/.X11-unix:/tmp/.X11-unix:ro)

# Normal build targets

build:
	@cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_DEBUG_LOGS=$(DEBUG_LOGS) \
		-DBUILD_TESTING=$(BUILD_TESTING) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@cmake --build "$(BUILD_DIR)" -j$(NPROC)

clean:
	@rm -rf "$(BUILD_DIR)"

clean-cache:
	@rm -rf -- "$(CURDIR)/.cache"

run:
	@"$(BUILD_DIR)/wayrandr"

test:
	@$(MAKE) build BUILD_TESTING=ON
	@ctest --test-dir "$(BUILD_DIR)" --output-on-failure

rebuild:
	@$(MAKE) clean
	@$(MAKE) build

# Container targets
# CMake caches contain absolute paths; keep container builds separate from host builds.

# The project cache is shared through the workspace bind mount.
container-clean-cache: clean-cache

container-build-image:
	@podman build -t "$(CONTAINER_IMAGE)" -f Containerfile .

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

container-build:
	@mkdir -p "$(CONTAINER_ARTIFACT_DIR)"
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-v "$(abspath $(CONTAINER_ARTIFACT_DIR)):/container-artifacts:z" \
		"$(CONTAINER_IMAGE)" sh -c 'make build && \
			cp build/wayrandr /container-artifacts/wayrandr.tmp && \
			mv -f /container-artifacts/wayrandr.tmp /container-artifacts/wayrandr'

run-from-container-locally:
	@if [ ! -x "$(CONTAINER_ARTIFACT_DIR)/wayrandr" ]; then \
		echo "Run make container-build first to export the executable." >&2; exit 1; \
	fi
	@"$(CONTAINER_ARTIFACT_DIR)/wayrandr"

container-test:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		"$(CONTAINER_IMAGE)" make test

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

container-pre-commit:
	@podman run --rm --pull=never $(CONTAINER_WORKSPACE) $(CONTAINER_BUILD_ENV) \
		-v "$(CONTAINER_PRE_COMMIT_VOLUME):/root/.cache/pre-commit:z" \
		--network host \
		"$(CONTAINER_IMAGE)" sh -c 'make build && pre-commit run --all-files'

.PHONY: build clean clean-cache run test rebuild container-clean-cache \
        container-build-image container-remove-image container-clean container-build container-test \
        run-from-container-locally container-run container-shell container-pre-commit
