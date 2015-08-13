#!/usr/bin/python
#

import os
import subprocess
import sys

_LIBUSB_DIR = os.path.dirname(os.path.realpath(__file__)) + '/external/libusb'

_PREFIX = sys.argv[1]

subprocess.check_call(['./bootstrap.sh'], cwd=_LIBUSB_DIR)
subprocess.check_call(
    ['./configure', 'CFLAGS=-fPIC', 'CXXFLAGS=-fPIC',
     '--prefix=' + _PREFIX], cwd=_LIBUSB_DIR)

