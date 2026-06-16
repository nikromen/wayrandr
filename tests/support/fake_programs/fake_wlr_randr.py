#!/usr/bin/python3
"""Test-only CLI: all state lives in WAYRANDR_TEST_DIR; no compositor access."""
import json
import os
import pathlib
import sys
import time

root = pathlib.Path(os.environ["WAYRANDR_TEST_DIR"])
args = sys.argv[1:]
with (root / "calls").open("a") as log:
    log.write(json.dumps(args) + "\n")
if args == ["--json"]:
    if (root / "query_failure").exists():
        sys.exit(1)
    print((root / "state").read_text())
    sys.exit(0)
previous = root / "mutator_pid"
if previous.exists():
    try:
        os.kill(int(previous.read_text()), 0)
        (root / "overlap").write_text("concurrent mutations")
        sys.exit(3)
    except ProcessLookupError:
        pass
previous.write_text(str(os.getpid()))
state = json.loads((root / "state").read_text())
# Model partial application even when the process reports failure.
output = None
arguments = iter(args)
for arg in arguments:
    if arg == "--output":
        name = next(arguments)
        output = next((m for m in state if m["name"] == name), None)
        if output is None:
            sys.exit(2)
    elif arg == "--on":
        output["enabled"] = True
    elif arg == "--off":
        output["enabled"] = False
    elif arg == "--mode":
        dimensions, refresh = next(arguments).split("@")
        width, height = map(int, dimensions.split("x"))
        refresh = float(refresh.removesuffix("Hz"))
        matched = False
        for mode in output["modes"]:
            mode["current"] = (mode["width"] == width and mode["height"] == height
                               and abs(mode["refresh"] - refresh) < 0.001)
            matched = matched or mode["current"]
        if not matched:
            sys.exit(2)
    elif arg == "--pos":
        x, y = map(int, next(arguments).split(","))
        output["position"] = {"x": x, "y": y}
    elif arg == "--transform":
        output["transform"] = next(arguments)
    elif arg == "--adaptive-sync":
        output["adaptive_sync"] = next(arguments) == "enabled"
    elif arg == "--scale":
        output["scale"] = float(next(arguments))
(root / "state").write_text(json.dumps(state))
# A timeout is deliberately after a partial mutation.
hangs = root / "hangs"
if hangs.exists() and int(hangs.read_text()):
    hangs.write_text(str(int(hangs.read_text()) - 1))
    (root / "pid").write_text(str(os.getpid()))
    time.sleep(60)
failures = root / "failures"
count = 0
if failures.exists():
    count = int(failures.read_text())
if count:
    failures.write_text(str(count - 1))
    error = root / "failure_stderr"
    if error.exists():
        print(error.read_text(), file=sys.stderr)
    sys.exit(1)
