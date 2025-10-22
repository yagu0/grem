#include <stdlib.h>
#include <limits.h>
#include "graph_dist.h"

// Code ChatGPT pour calculer les distances de graphe (poids = 1)
void bfs(Graph* g, int start, int* dist) {
  for (int i = 0; i < g->n; i++)
    dist[i] = INT_MAX;
  dist[start] = 0;
  int* queue = malloc(g->n * sizeof(int));
  int front = 0, back = 0;
  queue[back++] = start;

  while (front < back) {
    int u = queue[front++];
    for (int j = 0; j < g->nodes[u].degree; j++) {
      int v = g->nodes[u].neighbors[j];
      if (dist[v] == INT_MAX) {
        dist[v] = dist[u] + 1;
        queue[back++] = v;
      }
    }
  }

  free(queue);
}
