#include "../src/graph.h"
#include "../src/spring_embed.h"
#include <stdio.h>

// Compile with :
// gcc -fsanitize=address,undefined -fno-omit-frame-pointer -g debug.c -L. -lgrem -lm
// ( cd .. && make cclean && make debug && cd bin )
int main(int argc, char** argv) {
  //Graph g = make_random_tree(10, 0, 100, 32);
  //Graph g = make_random_graph(10, 0.2, 100, 32);
  Graph g = make_random_nary_tree(10, 1.5, 100, 32); //alpha dans ]1,2[
  //spring_layout(&g, 100, 100);
  free_graph(g);
  return 0;
}
