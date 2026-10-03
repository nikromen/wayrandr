#!/usr/bin/python3
import os
import pathlib
import signal
import sys
import time

mode = sys.argv[1]
if mode == 'success':
    print('success')
elif mode == 'empty':
    pass
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
else:
    sys.exit(99)
