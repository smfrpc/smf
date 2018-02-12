#!/bin/bash
set -evx
# csv output
toplev.py  -I 1000 -l3 -x, -o tlev.csv --core C1,C2 "$@"
