#!/bin/bash
set -euo pipefail

# Auto-activate local virtualenv if no Python environment is active.
if [[ -z "${VIRTUAL_ENV:-}" && -z "${CONDA_PREFIX:-}" ]]; then
  if [[ ! -d .venv ]]; then
    python -m venv .venv
  fi
  . ".venv/bin/activate"
fi

[[ -d ../grem.egg-info ]] && rm -r ../grem.egg-info
[[ -d ./build ]] && rm -r ./build
[[ -f ../c_project/bin/libgrem.a ]] && rm ../c_project/bin/libgrem.a
rm -f ../c_project/obj/*.o
pip install .
