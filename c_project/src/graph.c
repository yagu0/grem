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
  tryRealloc((void**)&g->nodes, sizeof(Node), g->n, n);
  // Positions aléatoires initiales
  for (int i = g->n; i < g->n + n; i++) {
    g->nodes[i].id = i;
    g->nodes[i].x = ((double) rand() / RAND_MAX) * width;
    g->nodes[i].y = ((double) rand() / RAND_MAX) * width;
    g->nodes[i].dx = g->nodes[i].dy = 0;
    g->nodes[i].degree = 0;
    g->nodes[i].neighbors = NULL;
    g->nodes[i].size = 0;
  }
  g->n += n;
}

// Crée un graphe aléatoire G(n,p) avec positions initiales aléatoires
Graph make_random_graph(int n, double p, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.nodes = NULL;
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
  g.nodes = NULL;
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
    tryRealloc((void**)&g.nodes[i].neighbors, sizeof(int),
               g.nodes[i].degree, 1);
    tryRealloc((void**)&g.nodes[M].neighbors, sizeof(int),
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
    g->nodes[i].size += 2; //augment sizes on the path
    int a_idx = g->nodes[i].neighbors[1 - (i == 0 ? 1 : 0)],
        b_idx = g->nodes[i].neighbors[2 - (i == 0 ? 1 : 0)];
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
  tryRealloc((void**)&g->nodes[i].neighbors, sizeof(int),
             g->nodes[i].degree, 2);
  for (int j = 0; j < 2; j++) {
    g->nodes[i].neighbors[g->nodes[i].degree + j] = old_n + j;
    g->nodes[old_n + j].degree = 1;
    g->nodes[old_n + j].neighbors = malloc(sizeof(int));
    g->nodes[old_n + j].neighbors[0] = i;
  }
  g->nodes[i].degree += 2;
}

Graph make_random_binary_tree(int n, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.nodes = NULL;
  g.n = 0;
  re_init_nodes(&g, 1, width);
  while (g.n < n)
    grow_binary_tree(&g, width);
  return g;
}

// Assume that g is an output of make_random_nary_tree() below
void grow_nary_tree(Graph* g, double alpha, double width)
{
  int i = 0,
      pos = 0,
      n = g->nodes[0].size; //leaves count
loopBegin:
  int start_idx = (i == 0 ? 0 : 1);
  double loc = (double)rand() / RAND_MAX;
  int k = g->nodes[i].degree - start_idx; //"up" neighbors count
  if (k == 0)
    goto afterLoop;
  double pnj = 1.0 / 3.0; //n == 1 -> one main branch only => 3 choices
  if (n >= 2) {
    int sumNi2 = 0;
    for (int jj = start_idx; jj < g->nodes[i].degree; jj++) {
      int ns = g->nodes[ g->nodes[i].neighbors[jj] ].size;
      sumNi2 += ns * ns;
    }
    pnj = (k-alpha)*(n*n-sumNi2) / ((alpha*n-1)*(k+1)*((n-1)*n));
  }
  double where = 0.0;
  for (int j = 0; j <= k; j++) {
    where += pnj;
    if (where >= loc) {
      pos = start_idx + j;
      goto afterLoop;
    }
  }
  // From here we know will recurse in some sub-tree:
  if (k == 1) { //true in particular if n == 1
    i = g->nodes[i].neighbors[start_idx];
    goto loopBegin;
  }
  for (int j = 0; j < k; j++) {
    int nj = g->nodes[ g->nodes[i].neighbors[j+start_idx] ].size;
    int sumNij = 0;
    for (int ii=0; ii<k; ii++) {
      if (ii == j)
        continue;
      for (int jj=0; jj<k; jj++) {
        if (jj == j || jj == ii)
          continue;
        sumNij += g->nodes[ g->nodes[i].neighbors[ii+start_idx] ].size *
          g->nodes[ g->nodes[i].neighbors[jj+start_idx] ].size;
      }
    }
    pnj = (alpha*nj-1) * ((nj-1)*nj*(nj+1)+3*nj*(nj+1)*(n-nj)+sumNij*(1+nj))
      / ((alpha*n-1)*(1+nj)*(n >= 2 ? n-1 : 1)*n);
    where += pnj;
    if (where >= loc) {
      i = g->nodes[i].neighbors[j+start_idx];
      goto loopBegin;
    }
  }
afterLoop:
  // Add one leaf from current leaf, at position pos.
  int old_n = g->n;
  re_init_nodes(g, 1, width);
  tryRealloc((void**)&g->nodes[i].neighbors, sizeof(int),
             g->nodes[i].degree, 1);
  for (int ii = g->nodes[i].degree; ii > pos; ii--)
    g->nodes[i].neighbors[ii] = g->nodes[i].neighbors[ii-1];
  g->nodes[i].neighbors[pos] = old_n;
  g->nodes[old_n].degree = 1;
  g->nodes[old_n].neighbors = malloc(sizeof(int));
  g->nodes[old_n].neighbors[0] = i;
  g->nodes[i].degree++;
  if (k > 0 || i == 0) {
    // Update size from here to root
    while (i > 0) {
      i = g->nodes[i].neighbors[0];
      g->nodes[i].size++;
    }
    g->nodes[0].size++;
  }
}

Graph make_random_nary_tree(int n, double alpha, double width, int seed)
{
  init_rand_gen(seed);
  Graph g;
  g.nodes = NULL;
  g.n = 0;
  re_init_nodes(&g, 1, width);
  while (g.n < n)
    grow_nary_tree(&g, alpha, width);
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
