#!/usr/bin/python3
import os
import pathlib
import sys
import time

root = pathlib.Path(os.environ['WAYRANDR_TEST_DIR'])
(root / 'grim_pid').write_text(str(os.getpid()))
if (root / 'grim_mode').read_text().strip() == 'hang':
    time.sleep(60)
sys.stdout.buffer.write(b'frame')
