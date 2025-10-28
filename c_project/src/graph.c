#include "graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// Initialize a graph with only nodes (no edges)
void init_nodes(Graph* g, int n, double width, int seed)
{
  if (seed >= 0)
    srand(seed);
  else
    srand(time(NULL));
  g->n = n;
  g->nodes = malloc(n * sizeof(Node));

  // Positions aléatoires initiales
  for (int i = 0; i < n; i++) {
    g->nodes[i].id = i;
    g->nodes[i].x = ((double) rand() / RAND_MAX) * width;
    g->nodes[i].y = ((double) rand() / RAND_MAX) * width;
    g->nodes[i].dx = g->nodes[i].dy = 0;
    g->nodes[i].degree = 0;
    g->nodes[i].capacity = 4; //arbitrary small 2^k
    g->nodes[i].neighbors = malloc(g->nodes[i].capacity * sizeof(int));
  }
}

// Reallocate space for an overflowed vector
void tryRealloc(Graph* g, int i) {
  if (g->nodes[i].capacity <= g->nodes[i].degree) {
    int new_capacity = g->nodes[i].capacity *= 2;
    g->nodes[i].neighbors = realloc(g->nodes[i].neighbors, new_capacity * sizeof(int));
  }
}

// Crée un graphe aléatoire G(n,p) avec positions initiales aléatoires
Graph make_random_graph(int n, double p, double width, int seed)
{
  Graph g;
  init_nodes(&g, n, width, seed);

  for (int i = 0; i < n; i++) {
    for (int j = i+1; j < n; j++) {
      if (((double) rand() / RAND_MAX) < p) {
        tryRealloc(&g, i);
        tryRealloc(&g, j);
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
  Graph g;
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
    tryRealloc(&g, i);
    tryRealloc(&g, M);
    g.nodes[i].neighbors[g.nodes[i].degree] = M;
    g.nodes[M].neighbors[g.nodes[M].degree] = i;
    g.nodes[i].degree++;
    g.nodes[M].degree++;
  }

  return g;
}

// TODO
grow_binary_tree()
{
  // realloc nodes if needed (+1) --> missing capacity ?
}

// TODO: need to output Graph + array of sizes left/right (a, b ?)
// sizes updated when calling grow_binary_tree(Graph g, int** sizes, double width).
// Used only for construction, but can be in graph.h ? Or no need.
Graph make_random_binary_tree(int n, int** sizes, double width, int seed)
{
  Graph g;
  if (seed >= 0)
    srand(seed);
  else
    srand(time(NULL));
  g->n = 1;
  g->nodes = malloc(4 * sizeof(Node));
  //for (int i = 0; i < n; i++) {
    // TODO: next lines in one function; call for 0
    g->nodes[0].id = i;
    g->nodes[0].x = ((double) rand() / RAND_MAX) * width;
    g->nodes[0].y = ((double) rand() / RAND_MAX) * width;
    g->nodes[i].dx = g->nodes[i].dy = 0;
    g->nodes[i].degree = 0;
    g->nodes[i].capacity = 4; //arbitrary small 2^k
    g->nodes[i].neighbors = malloc(g->nodes[i].capacity * sizeof(int));
  //}
  for (int i=1; i<n; i++) {
    grow_binary_tree(g, sizes, with);
  }
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
    nd->capacity = 4; // capacité initiale arbitraire
    nd->neighbors = (int*)malloc(nd->capacity * sizeof(int));
    fscanf(f, "%lf %lf", &nd->x, &nd->y); //==2
  }

  // Lecture des arêtes
  for (int j = 0; j < m; j++) {
    int u, v;
    fscanf(f, "%d %d", &u, &v); //==2

    // Ajoute les voisins symétriquement (graphe non orienté)
    Node* nu = &g.nodes[u];
    Node* nv = &g.nodes[v];

    if (nu->degree >= nu->capacity) {
      nu->capacity *= 2;
      nu->neighbors = (int*)realloc(nu->neighbors, nu->capacity * sizeof(int));
    }
    if (nv->degree >= nv->capacity) {
      nv->capacity *= 2;
      nv->neighbors = (int*)realloc(nv->neighbors, nv->capacity * sizeof(int));
    }

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
