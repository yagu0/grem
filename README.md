# grem

grem : GRaph EMbedding. <br>
Fast graph visualisation based on Electrical Spring Embedding.

## Requirements

Python3, pip, gcc.

## Installation

cd pkg
./rebuild.sh
nbstripout --install

## Usage

From Python:

import grem

gr = grem.make\_random\_graph(100, 0.2, 150) <br>
grem.plot\_graph(g=gr)

grem.spring\_layout(gr, 200, 300) <br>
grem.plot\_graph(g=gr)

See also doc/usage.ipynb
