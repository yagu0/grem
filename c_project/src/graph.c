#include "graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define EPSILON 0.0000001

void init_rand_gen(int seed)
{
  if (seed >= 0)
    srand(seed);
  else
    srand(time(NULL));
}

// Reallocate space for an overflowed vector
void tryRealloc(int** v, int size, int nb)
{
  // Test if current size is a power of 2 (== max capacity reached)
  if (size >= 1 && log2(size) % 1 < EPSILON) {
    int total = size + nb;
    while (size < total)
      size << 1;
    *v = realloc(*v, size * sizeof(int));
  }
}

// Initialize a graph with only nodes (no edges)
void re_init_nodes(Graph* g, int n, double width)
{
  if (g->n == 0)
    g->nodes = malloc(n * sizeof(Node));
  else
    tryRealloc(&g->nodes, g->n, n);

  // Positions aléatoires initiales
  for (int i = 0; i < n; i++) {
    g->nodes[g->n + i].id = i;
    g->nodes[g->n + i].x = ((double) rand() / RAND_MAX) * width;
    g->nodes[g->n + i].y = ((double) rand() / RAND_MAX) * width;
    g->nodes[g->n + i].dx = g->nodes[i].dy = 0;
    g->nodes[g->n + i].degree = 0;
    g->nodes[g->n + i].neighbors = malloc(sizeof(int));
  }
  g->n = n;
}


// Crée un graphe aléatoire G(n,p) avec positions initiales aléatoires
Graph make_random_graph(int n, double p, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.n = 0;
  re_init_nodes(&g, n, width, seed);

  for (int i = 0; i < n; i++) {
    for (int j = i+1; j < n; j++) {
      if (((double) rand() / RAND_MAX) < p) {
        tryRealloc(&g->nodes[i].neighbors, g->nodes[i].degree, 1);
        tryRealloc(&g->nodes[j].neighbors, g->nodes[j].degree, 1);
        g.nodes[i].neighbors[g.nodes[i].degree] = j;
        g.nodes[j].neighbors[g.nodes[j].degree] = i;
        g.nodes[i].degree++;
        g.nodes[j].degree++;
      }
    }
  }

  return g;
}

// Crée un arbre aléatoire T(n) au hasard, ou preferential attachment
Graph make_random_tree(int n, int mode, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.n = 0;
  init_nodes(&g, n, width, seed);

  for (int i = 1; i < n; i++) {
    int M = 0;
    // tirer au hasard M dans [0, i-1] : rattacher i à M, continuer
    if (mode == RND)
      M = rand() % i;
    else {
      // mode == PA
      M = i-1; //default to last
      int sumDegs = 0;
      for (int j=0; j<i; j++)
        sumDegs += g.nodes[j].degree;
      if (sumDegs > 0) {
        double rn = (double) rand() / RAND_MAX;
        double cumSum = 0.0;
        for (int j=0; j<i; j++) {
          cumSum += (double) g.nodes[j].degree / sumDegs;
          if (rn < cumSum) {
            M = j;
            break;
          }
        }
      }
    }
    tryRealloc(g.nodes[i].neighbors, g.nodes[i].degree, 1);
    tryRealloc(g.nodes[M].neighbors, g.nodes[M].degree, 1);
    g.nodes[i].neighbors[g.nodes[i].degree] = M;
    g.nodes[M].neighbors[g.nodes[M].degree] = i;
    g.nodes[i].degree++;
    g.nodes[M].degree++;
  }

  return g;
}

// Assume that g is an output of make_random_binary_tree() below
void grow_binary_tree(Graph g, double width)
{
  // TODO: grow one cherry from root (index 0). Update g.sizes
  // g.nodes[n]...
  // Convention: except at root, neighbors[0] = parent,
  // neighbors[1, 2] if present = children.
}

Graph make_random_binary_tree(int n, double width, int seed)
{
  // TODO: init g.sizes ?
  init_rand_gen(seed);
  Graph g;
  re_init_nodes(g, 1, width);
  for (int i=1; i<n; i++)
    grow_binary_tree(g, width);
  return g;
}

void write_graph(Graph g, char* path)
{
  FILE* fptr = fopen(path, "w");
  int m = 0;
  for (int i = 0; i < g.n; i++)
    m += g.nodes[i].degree;
  m /= 2;
  fprintf(fptr, "%i %i\n", g.n, m);
  for (int i=0; i < g.n; i++)
    fprintf(fptr, "%f %f\n", g.nodes[i].x, g.nodes[i].y);
  for (int i=0; i < g.n; i++) {
    for (int j=0; j < g.nodes[i].degree; j++) {
      if (g.nodes[i].neighbors[j] > i)
        fprintf(fptr, "%i %i\n", i, g.nodes[i].neighbors[j]);
    }
  }
  fclose(fptr);
}

Graph read_graph(char* path)
{
  FILE* f = fopen(path, "r");

  int n, m;
  fscanf(f, "%d %d", &n, &m); //==2

  Graph g;
  g.n = n;
  g.nodes = (Node*)calloc(n, sizeof(Node));

  // Lecture des coordonnées
  for (int i = 0; i < n; i++) {
    Node* nd = &g.nodes[i];
    nd->id = i;
    nd->dx = nd->dy = 0.0;
    nd->degree = 0;
    nd->neighbors = (int*)malloc(sizeof(int));
    fscanf(f, "%lf %lf", &nd->x, &nd->y); //==2
  }

  // Lecture des arêtes
  for (int j = 0; j < m; j++) {
    int u, v;
    fscanf(f, "%d %d", &u, &v); //==2

    // Ajoute les voisins symétriquement (graphe non orienté)
    Node* nu = &g.nodes[u];
    Node* nv = &g.nodes[v];
    tryRealloc(&nu->neighbors, nu->degree, 1);
    tryRealloc(&nv->neighbors, nv->degree, 1);
    nu->neighbors[nu->degree++] = v;
    nv->neighbors[nv->degree++] = u;
  }

  fclose(f);
  return g;
}

void free_graph(Graph g)
{
  for (int i=0; i < g.n; i++)
    free(g.nodes[i].neighbors);
  free(g.nodes);
}
