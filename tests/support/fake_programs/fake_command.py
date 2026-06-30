#!/usr/bin/python3
import os
import json
import pathlib
import signal
import sys
import time

mode = sys.argv[1]
if mode.startswith('group-'):
    pidfile = pathlib.Path(sys.argv[2])
    ready_read, ready_write = os.pipe()
    child = os.fork()
    if child == 0:
        os.close(ready_read)
        os.close(1)
        os.close(2)
        os.write(ready_write, b'1')
        os.close(ready_write)
        time.sleep(60)
        os._exit(0)
    os.close(ready_write)
    assert os.read(ready_read, 1) == b'1'
    os.close(ready_read)
    pidfile.write_text(str(os.getpid()))
    pathlib.Path(str(pidfile) + '.child').write_text(str(child))
    pathlib.Path(str(pidfile) + '.ready').write_text('1')
    release = pathlib.Path(str(pidfile) + '.release')
    while not release.exists():
        time.sleep(0.005)
    if mode == 'group-exit':
        print('failure detail', file=sys.stderr, flush=True)
        sys.exit(17)
    if mode == 'group-crash':
        os.kill(os.getpid(), signal.SIGKILL)
    if mode == 'group-success':
        sys.exit(0)
    if mode == 'group-flood':
        while True:
            os.write(1, b'x' * 65536)
            os.write(2, b'e' * 65536)
    time.sleep(60)
elif mode == 'success':
    print('success')
elif mode == 'empty':
    pass
elif mode == 'wait-children':
    child = os.fork()
    if child == 0:
        os._exit(0)
    while True:
        try:
            os.waitpid(-1, 0)
        except ChildProcessError:
            break
    print('success')
elif mode == 'stderr':
    print('diagnostic', file=sys.stderr)
elif mode == 'exit':
    print('failure detail', file=sys.stderr)
    sys.exit(17)
elif mode == 'crash':
    os.kill(os.getpid(), signal.SIGKILL)
elif mode == 'child':
    child = os.fork()
    if child == 0:
        time.sleep(60)
        sys.exit(0)
    pathlib.Path(sys.argv[2]).write_text(str(os.getpid()))
    pathlib.Path(sys.argv[2] + '.child').write_text(str(child))
    time.sleep(60)
elif mode == 'hang':
    pathlib.Path(sys.argv[2]).write_text(str(os.getpid()))
    time.sleep(60)
elif mode == 'flood':
    while True:
        os.write(1, b'x' * 65536)
        os.write(2, b'e' * 65536)
elif mode == 'args':
    print(sys.argv[2])
elif mode == 'argv':
    print(json.dumps(sys.argv[2:]))
else:
    sys.exit(99)
