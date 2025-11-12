#include "graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

void init_rand_gen(int seed)
{
  if (seed >= 0)
    srand(seed);
  else
    srand(time(NULL));
}

// Reallocate space for an overflowed vector
void tryRealloc(void** v, int cell_size, int size, int nb)
{
  // Capacity = closest power of 2 above current size
  int capacity = 0;
  if (size >= 1)
    capacity = 1 << (int)ceil(log2(size));
  if (size + nb > capacity) {
    int new_capacity = 1 << (int)ceil(log2(size + nb));
    *v = realloc(*v, new_capacity * cell_size);
  }
}

// Initialize a graph with only nodes (no edges)
void re_init_nodes(Graph* g, int n, double width)
{
  if (g->n > 0)
    tryRealloc((void**)&g->nodes, sizeof(Node), g->n, n);

  // Positions aléatoires initiales
  for (int i = 0; i < n; i++) {
    g->nodes[g->n + i].id = i;
    g->nodes[g->n + i].x = ((double) rand() / RAND_MAX) * width;
    g->nodes[g->n + i].y = ((double) rand() / RAND_MAX) * width;
    g->nodes[g->n + i].dx = g->nodes[g->n + i].dy = 0;
    g->nodes[g->n + i].degree = 0;
    g->nodes[g->n + i].neighbors = NULL;
    g->nodes[g->n + i].size = 0;
  }
  g->n += n;
}

// Crée un graphe aléatoire G(n,p) avec positions initiales aléatoires
Graph make_random_graph(int n, double p, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.n = 0;
  re_init_nodes(&g, n, width);

  for (int i = 0; i < n; i++) {
    for (int j = i+1; j < n; j++) {
      if (((double) rand() / RAND_MAX) < p) {
        tryRealloc((void**)&g.nodes[i].neighbors, sizeof(int),
                   g.nodes[i].degree, 1);
        tryRealloc((void**)&g.nodes[j].neighbors, sizeof(int),
                   g.nodes[j].degree, 1);
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
  re_init_nodes(&g, n, width);

  for (int i = 1; i < n; i++) {
    int M = 0;
    // tirer au hasard M dans [0, i-1] : rattacher i à M, continuer
    if (mode == RND)
      M = rand() % i;
    else {
      // mode == PA
      M = i - 1; //default to last
      int sumDegs = 0;
      for (int j = 0; j < i; j++)
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
    tryRealloc((void**)g.nodes[i].neighbors, sizeof(int),
               g.nodes[i].degree, 1);

printf("%i %i %p\n", M, g.nodes[M].degree, g.nodes[M].neighbors);

    tryRealloc((void**)g.nodes[M].neighbors, sizeof(int),
               g.nodes[M].degree, 1);
    g.nodes[i].neighbors[g.nodes[i].degree] = M;
    g.nodes[M].neighbors[g.nodes[M].degree] = i;
    g.nodes[i].degree++;
    g.nodes[M].degree++;
  }

  return g;
}

// Assume that g is an output of make_random_binary_tree() below
void grow_binary_tree(Graph* g, double width)
{
  int i = 0;
  while (g->nodes[i].degree >= 2) {
    g->nodes[i].size++; //augment sizes on the path
    int a_idx = g->nodes[i].neighbors[1 - (i == 0 ?  1 : 0)],
        b_idx = g->nodes[i].neighbors[2 - (i == 0 ?  1 : 0)];
    int a = g->nodes[a_idx].size,
        b = g->nodes[b_idx].size;
    double Cab = ( (a + 1) * (2*a + 1) * (a + 3*b + 3) ) /
      ( (a + b + 1) * (a + b + 2) * (2 * (a + b) + 3) );
    double lr = (double)rand() / RAND_MAX;
    i = (lr < Cab ? a_idx : b_idx);
  }
  // Grow one cherry from current leaf.
  int old_n = g->n;
  re_init_nodes(g, 2, width);
  tryRealloc((void**)g->nodes[i].neighbors, sizeof(int), g->nodes[i].degree, 2);
  for (int j = 0; j < 2; j++) {
    g->nodes[i].neighbors[g->nodes[i].degree + j] = old_n + j;
    g->nodes[old_n + j].degree = 1;
    g->nodes[old_n + j].neighbors[0] = i;
  }
  g->nodes[i].degree += 2;
}

Graph make_random_binary_tree(int n, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.n = 0;
  re_init_nodes(&g, 1, width);
  for (int i=1; i<n; i++)
    grow_binary_tree(&g, width);
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
    nd->neighbors = NULL;
    fscanf(f, "%lf %lf", &nd->x, &nd->y); //==2
  }

  // Lecture des arêtes
  for (int j = 0; j < m; j++) {
    int u, v;
    fscanf(f, "%d %d", &u, &v); //==2

    // Ajoute les voisins symétriquement (graphe non orienté)
    Node* nu = &g.nodes[u];
    Node* nv = &g.nodes[v];
    tryRealloc((void**)&nu->neighbors, sizeof(int), nu->degree, 1);
    tryRealloc((void**)&nv->neighbors, sizeof(int), nv->degree, 1);
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
