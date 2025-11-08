BUILD_DIR = build
NPROC = $(shell nproc)
DEBUG_LOGS ?= ON

# Normal build targets

build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug \
		-DENABLE_DEBUG_LOGS=$(DEBUG_LOGS) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	@cd $(BUILD_DIR) && cmake --build . -j$(NPROC)

clean:
	@rm -rf $(BUILD_DIR)

run:
	@$(BUILD_DIR)/wayrandr

rebuild: clean build

# Container targets

container-build-image:
	@podman build -t wayrandr:latest -f Containerfile .

container-remove-image:
	@podman rmi wayrandr:latest

container-build:
	@podman run --rm -v .:/workspace:z -w /workspace \
		wayrandr:latest make build DEBUG_LOGS=$(DEBUG_LOGS)

container-run:
	@podman run --rm -it -v .:/workspace:z -w /workspace \
		-e DISPLAY=$${DISPLAY} -e WAYLAND_DISPLAY=$${WAYLAND_DISPLAY} \
		-e XDG_RUNTIME_DIR=$${XDG_RUNTIME_DIR} \
		-v /tmp/.X11-unix:/tmp/.X11-unix:ro \
		-v $${XDG_RUNTIME_DIR}:$${XDG_RUNTIME_DIR} \
		--network host --security-opt label=disable \
		wayrandr:latest sh -c "make build DEBUG_LOGS=$(DEBUG_LOGS) && make run"

container-shell:
	@podman run --rm -it -v .:/workspace:z -w /workspace \
		-e DISPLAY=$${DISPLAY} -e WAYLAND_DISPLAY=$${WAYLAND_DISPLAY} \
		-e XDG_RUNTIME_DIR=$${XDG_RUNTIME_DIR} \
		-v /tmp/.X11-unix:/tmp/.X11-unix:ro \
		-v $${XDG_RUNTIME_DIR}:$${XDG_RUNTIME_DIR} \
		--network host --security-opt label=disable \
		wayrandr:latest /bin/bash

container-pre-commit:
	@podman run --rm -v .:/workspace:z -w /workspace \
		-v wayrandr-precommit-cache:/root/.cache/pre-commit:z \
		--network host \
		wayrandr:latest sh -c "make build && pre-commit run --all-files"

.PHONY: build clean run rebuild \
        container-build-image container-remove-image container-build container-run container-shell container-pre-commit
