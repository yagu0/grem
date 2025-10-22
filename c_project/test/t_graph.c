#include <stdio.h>
#include "utest.h"
#include "../src/graph.h"

UTEST(graph, read_write_graph1) {
  Graph g = make_random_graph(100, 0.2, 150, 32);
  write_graph(g, "tmpgraph");
  Graph h = read_graph("tmpgraph");
  ASSERT_EQ(g.n, h.n);
  for (int i = 0; i < g.n; i++) {
    ASSERT_EQ(g.nodes[i].degree, h.nodes[i].degree);
    for (int j = 0; j < g.nodes[i].degree; j++)
      ASSERT_EQ(g.nodes[i].neighbors[j], h.nodes[i].neighbors[j]);
  }
  free_graph(g);
  free_graph(h);
  remove("tmpgraph");
}

UTEST(graph, read_write_graph2) {
  Graph g = make_random_tree(150, 1, 200, 42);
  write_graph(g, "tmpgraph");
  Graph h = read_graph("tmpgraph");
  ASSERT_EQ(g.n, h.n);
  for (int i = 0; i < g.n; i++) {
    ASSERT_EQ(g.nodes[i].degree, h.nodes[i].degree);
    for (int j = 0; j < g.nodes[i].degree; j++)
      ASSERT_EQ(g.nodes[i].neighbors[j], h.nodes[i].neighbors[j]);
  }
  free_graph(g);
  free_graph(h);
  remove("tmpgraph");
}
