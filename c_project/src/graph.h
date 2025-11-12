#ifndef GREM_GRAPH_H
#define GREM_GRAPH_H

typedef struct Node {
  int id;
  double x, y;
  double dx, dy;
  int degree;
  int* neighbors;
  int size; //for binary trees only
} Node;

typedef struct Graph {
  int n;
  Node* nodes;
} Graph;

// Build Erdos-Renyi graph
Graph make_random_graph(int n, double p, double width, int seed);

enum {RND=0, PA};

// Build random tree, at random or preferential attachment
Graph make_random_tree(int n, int mode, double width, int seed);

// Random binary tree following https://arxiv.org/pdf/2401.07891
Graph make_random_binary_tree(int n, double width, int seed);

// Read/write functions (from/to file)
void write_graph(Graph g, char* path);
Graph read_graph(char* path);

void free_graph(Graph g);

#endif
