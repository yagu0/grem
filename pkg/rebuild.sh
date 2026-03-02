#!/bin/bash

[[ -d ../grem.egg-info ]] && rm -r ../grem.egg-info
[[ -d ./build ]] && rm -r ./build
[[ -f ../c_project/bin/libgrem.a ]] && rm ../c_project/bin/libgrem.a
rm -f ../c_project/obj/*.o
pip install .
