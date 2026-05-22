#!/usr/bin/python3
"""Daemon control stub; logs only to the isolated test directory."""
import json
import os
import pathlib
import sys
import time

root = pathlib.Path(os.environ['WAYRANDR_TEST_DIR'])
with (root / 'daemon_calls').open('a') as log:
    log.write(json.dumps([pathlib.Path(sys.argv[0]).name, *sys.argv[1:]]) + '\n')
mode = (root / 'daemon_mode').read_text().strip()
if mode == 'exit' or (mode == 'reload_exit' and sys.argv[1] == 'reload'):
    print('daemon failure detail', file=sys.stderr)
    sys.exit(17)
if mode == 'empty':
    sys.exit(0)
if mode == 'delay':
    time.sleep(0.1)
active_file = root / 'active_profile'
if sys.argv[1] == 'switch':
    profile = sys.argv[2]
    if pathlib.Path(sys.argv[0]).name == 'auto-wlr-randrctl':
        args = sys.argv[2:]
        profile = args[args.index('--') + 1]
    active_file.write_text(profile)
active = 'test'
if active_file.exists():
    active = active_file.read_text()
if sys.argv[1] == 'status':
    reply = root / 'status_reply'
    if reply.exists():
        print(reply.read_text(), end='')
        sys.exit(0)
    if pathlib.Path(sys.argv[0]).name == 'kanshictl':
        print('Current profile: ' + active)
    else:
        print(json.dumps({'active_profile': active, 'connected_outputs': ['DP-1']}))
