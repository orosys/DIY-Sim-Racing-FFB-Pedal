#!/usr/bin/env python3
"""Compile the real V3 Fanatec implementation against a simulated UART and GPIO."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

here = Path(__file__).resolve().parent
project = here.parents[1] / 'Firmware_for_V3' / 'BridgeFirmware'
compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
if not compiler:
    raise SystemExit('Set CXX to a C++ compiler')
with tempfile.TemporaryDirectory(prefix='fanatec-connection-test-') as tmp:
    executable = str(Path(tmp) / 'connection_test')
    subprocess.run([compiler, '-std=c++17', '-g', '-fsanitize=address,undefined',
                    '-I'+str(here/'stubs'), '-I'+str(project/'include'),
                    str(here/'connection_test.cpp'), str(project/'src/FanatecInterface.cpp'),
                    '-o', executable], check=True)
    subprocess.run([executable], check=True)
