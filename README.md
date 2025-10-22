# grem

grem : GRaph EMbedding. <br>
Fast graph visualisation based on Electrical Spring Embedding.

## Requirements

Python3, pip, gcc.

## Installation

cd pkg

Good practice: create a dedicated virtual environment <br>
python -m venv .venv && source .venv/bin/activate

Release:

pip install . [--use-pep517]

Dev mode:

pip install -e . --no-build-isolation <br>
python setup.py build\_ext --inplace #recompile wrapper

## Usage

From Python:

import grem

gr = grem.make\_random\_graph(100, 0.2, 150) <br>
grem.plot\_graph(g=gr)

grem.spring\_layout(gr, 10, 300) <br>
grem.plot\_graph(g=gr)

See also doc/usage.ipynb
