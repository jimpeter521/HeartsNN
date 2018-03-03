#!/usr/bin/env python3

import numpy as np
import sys

file_path = 'test.npy' if len(sys.argv)==1 else sys.argv[1]

np.set_printoptions(linewidth=300)
a = np.load(file_path)


print(np.array_repr(a, max_line_width=1000, suppress_small=True))
