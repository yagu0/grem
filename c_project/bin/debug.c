#include "../src/graph.h"
#include "../src/spring_embed.h"
#include <stdio.h>

// Compile with :
// gcc -g debug.c -L. -lgrem -lm
// ( cd .. && make debug && cd bin )
int main(int argc, char** argv) {
  //Graph g = make_random_tree(10, 0, 100, 32);
  //Graph g = make_random_graph(10, 0.2, 100, 32);
  Graph g = make_random_binary_tree(100, 100, 32);
  spring_layout(&g, 100, 100);
  free_graph(g);
  return 0;
}
