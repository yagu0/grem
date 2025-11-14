#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph_dist.h"
#include "spring_embed.h"

#define COOLING 0.95
#define MIN_DELTA 0.01
#define THETA 0.5
#define EPS 0.1

QuadTree* new_quadtree(double cx, double cy, double width)
{
  QuadTree* qt = malloc(sizeof(QuadTree));
  qt->cx = cx;
  qt->cy = cy;
  for (int dir=0; dir<4; dir++)
    qt->subtree[dir] = NULL;
  qt->size = width;
  qt->mass = 0;
  qt->node = NULL;
  return qt;
}

// Condition : pas deux points de mêmes coordonnées (TODO: détecter)
void insert_quadtree(QuadTree* qt, Node* p)
{
  if (qt->mass == 0) {
    qt->node = p;
    qt->mx = p->x;
    qt->my = p->y;
    qt->mass = 1;
    return;
  }

  // Mettre à jour le centre de masse
  qt->mx = (qt->mx * qt->mass + p->x) / (qt->mass + 1.0);
  qt->my = (qt->my * qt->mass + p->y) / (qt->mass + 1.0);
  qt->mass++;

  double hs = qt->size / 2.0,
         half = hs / 2.0;
  int dir = 0;
  for (int i=-1; i<=1; i+=2) {
    for (int j=-1; j<=1; j+=2) {
      double cx = qt->cx + i * half, cy = qt->cy + j * half;
      if (qt->node != NULL) {
        double nx = qt->node->x, ny = qt->node->y;
        if (
          nx >= cx - half && nx < cx + half &&
          ny >= cy - half && ny < cy + half
        ) {
          if (qt->subtree[dir] == NULL)
            qt->subtree[dir] = new_quadtree(cx, cy, hs);
          insert_quadtree(qt->subtree[dir], qt->node);
          qt->node = NULL;
        }
      }
      if (p != NULL) {
        double px = p->x, py = p->y;
        if (
          px >= cx - half && px < cx + half &&
          py >= cy - half && py < cy + half
        ) {
          if (qt->subtree[dir] == NULL)
            qt->subtree[dir] = new_quadtree(cx, cy, hs);
          insert_quadtree(qt->subtree[dir], p);
          p = NULL;
        }
      }
      dir++;
    }
  }
}

void compute_force(Node* target, QuadTree* qt, double theta, double k, int* topo_dist_row, int d) {
  if (!qt || qt->mass == 0 || qt->node == target)
    return;

  double dx = qt->mx - target->x;
  double dy = qt->my - target->y;
  double dist = sqrt(dx*dx + dy*dy) + 1e-4;

  if ((qt->size / dist) < theta || qt->node != NULL) {
    // Pondération topologique (si node != NULL seulement)
    int topo_dist = 1;
    if (qt->node != NULL) {
      topo_dist = topo_dist_row[qt->node->id];
      if (topo_dist <= 0)
        topo_dist = 1; //éviter division par 0
    }

    double factor = 1.0 / pow(topo_dist, d); //topological modulation
    double f = k*k*qt->mass * factor / dist;

    target->dx -= dx/dist * f;
    target->dy -= dy/dist * f;
  }
  else {
    for (int dir = 0; dir < 4; dir++)
      compute_force(target, qt->subtree[dir], theta, k, topo_dist_row, d);
  }
}

void free_quadtree(QuadTree* qt)
{
  if (!qt)
    return;
  for (int dir = 0; dir < 4; dir++)
    free_quadtree(qt->subtree[dir]);
  free(qt);
}

void spring_layout(Graph* g, int max_iter, double width)
{
  double area = width * width;
  double k = sqrt(area / g->n);
  double t = width / 10.0;

  // --- pré-calcul distances de graphe ---
  int** graph_dist = malloc(g->n * sizeof(int*));
  for (int i = 0; i < g->n; i++) {
    graph_dist[i] = malloc(g->n * sizeof(int));
    bfs(g, i, graph_dist[i]);
  }

  for (int iter=0; iter < max_iter; iter++) {
    double maxDelta = 0.0;
    for (int i=0; i < g->n; i++)
      g->nodes[i].dx = g->nodes[i].dy = 0;

    // Construire le quadtree
    QuadTree* qt = new_quadtree(width/2, width/2, width);

    for (int i=0; i < g->n; i++)
      insert_quadtree(qt, &g->nodes[i]);

    // Forces répulsives via Barnes-Hut
    int d = 2; //exposant pour la pondération topologique
    for (int i = 0; i < g->n; i++)
      compute_force(&g->nodes[i], qt, THETA, k, graph_dist[i], d);

    // Forces attractives (classiques)
    for (int i = 0; i < g->n; i++) {
      for (int j = 0; j < g->nodes[i].degree; j++) {
        Node* u = &g->nodes[i];
        Node* v = &g->nodes[g->nodes[i].neighbors[j]];
        double dx = v->x - u->x;
        double dy = v->y - u->y;
        double dist = sqrt(dx*dx + dy*dy) + 1e-4;
        double f = dist*dist/k;
        u->dx += dx/dist*f; u->dy += dy/dist*f;
        v->dx -= dx/dist*f; v->dy -= dy/dist*f;
      }
    }

    // Gravité vers le centre
    for (int i = 0; i < g->n; i++) {
      double cx = width / 2.0;
      double cy = width / 2.0;
      double gx = cx - g->nodes[i].x;
      double gy = cy - g->nodes[i].y;
      double g_dist = sqrt(gx*gx + gy*gy) + 1e-4;

      double grav_strength = 0.01; //ajustable
      g->nodes[i].dx += gx/g_dist * grav_strength * g_dist;
      g->nodes[i].dy += gy/g_dist * grav_strength * g_dist;
    }

    // Appliquer déplacements
    for (int i=0; i < g->n; i++) {
      double dx = g->nodes[i].dx,
             dy = g->nodes[i].dy;
      double disp = sqrt(dx*dx + dy*dy);
      if (disp > MIN_DELTA) {
        double deltaX = dx/disp * fmin(disp, t),
               deltaY = dy/disp * fmin(disp, t);
        g->nodes[i].x += deltaX;
        g->nodes[i].y += deltaY;
        if (maxDelta < EPS && sqrt(deltaX*deltaX + deltaY*deltaY) >= EPS)
          maxDelta = EPS;
      }
    }

    t *= COOLING;

    free_quadtree(qt);

    if (maxDelta < EPS)
      break;
  }

  for (int i = 0; i < g->n; i++)
    free(graph_dist[i]);
  free(graph_dist);
}
