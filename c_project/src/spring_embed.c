#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph_dist.h"
#include "spring_embed.h"

#define COOLING 0.95
#define MIN_DELTA 0.01
#define THETA 0.5
#define EPS 0.1
#define DIST_EPS 1e-6
#define QUAD_MIN_SIZE 1e-3
#define INIT_TEMP_FACTOR 5.0
#define MAX_GROWTH_PER_ITER 1.01
#define MAX_GLOBAL_GROWTH 1.5
#define TARGET_FILL_RATIO 0.92

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

static int pick_dir(const QuadTree* qt, const Node* p)
{
  int east = (p->x >= qt->cx) ? 1 : 0;
  int north = (p->y >= qt->cy) ? 1 : 0;
  return east + 2 * north;
}

static void child_center(const QuadTree* qt, int dir, double* ccx, double* ccy)
{
  double quarter = qt->size / 4.0;
  double sx = (dir & 1) ? 1.0 : -1.0;
  double sy = (dir & 2) ? 1.0 : -1.0;
  *ccx = qt->cx + sx * quarter;
  *ccy = qt->cy + sy * quarter;
}

// Robust insertion (supports very close or identical positions)
void insert_quadtree(QuadTree* qt, Node* p)
{
  if (qt->size < QUAD_MIN_SIZE) {
    // Stop splitting tiny cells; keep only aggregated mass/center.
    if (qt->mass == 0) {
      qt->node = p;
      qt->mx = p->x;
      qt->my = p->y;
      qt->mass = 1;
      return;
    }
    qt->mx = (qt->mx * qt->mass + p->x) / (qt->mass + 1.0);
    qt->my = (qt->my * qt->mass + p->y) / (qt->mass + 1.0);
    qt->mass++;
    qt->node = NULL;
    return;
  }

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

  double hs = qt->size / 2.0;

  if (qt->node != NULL) {
    Node* old = qt->node;
    qt->node = NULL;
    int dir_old = pick_dir(qt, old);
    double ccx, ccy;
    child_center(qt, dir_old, &ccx, &ccy);
    if (qt->subtree[dir_old] == NULL)
      qt->subtree[dir_old] = new_quadtree(ccx, ccy, hs);
    insert_quadtree(qt->subtree[dir_old], old);
  }

  int dir_new = pick_dir(qt, p);
  double ccx, ccy;
  child_center(qt, dir_new, &ccx, &ccy);
  if (qt->subtree[dir_new] == NULL)
    qt->subtree[dir_new] = new_quadtree(ccx, ccy, hs);
  insert_quadtree(qt->subtree[dir_new], p);
}

// Compute repulsive forces (k^2 / dist)
void compute_force(Node* target, QuadTree* qt, double theta,
                   double k, int* topo_dist_row, int d)
{
  if (!qt || qt->mass == 0 || qt->node == target)
    return;

  double dx = qt->mx - target->x,
         dy = qt->my - target->y;
  double dist = sqrt(dx*dx + dy*dy);
  if (dist < DIST_EPS)
    dist = DIST_EPS;

  if ((qt->size / dist) < theta || qt->node != NULL) {
    // Topological modulation (only if node != NULL)
    int topo_dist = (qt->node != NULL ? topo_dist_row[qt->node->id] : 1);
    if (topo_dist <= 0)
      topo_dist = 1;
    double factor = 1.0 / pow(topo_dist, d);
    double f = k*k*qt->mass * factor / dist;
    target->dx += - dx/dist * f;
    target->dy += - dy/dist * f;
  }
  else {
    for (int dir = 0; dir < 4; dir++)
      compute_force(target, qt->subtree[dir], theta, k, topo_dist_row, d);
  }
}

void free_quadtree(QuadTree* qt)
{
  for (int dir = 0; dir < 4; dir++) {
    if (qt->subtree[dir])
      free_quadtree(qt->subtree[dir]);
  }
  free(qt);
}

void spring_layout(Graph* g, int max_iter, int d, double grav_strength)
{
  if (g == NULL || g->n <= 1)
    return;

  // Pre-compute all graph distances
  int** graph_dist = malloc(g->n * sizeof(int*));
  for (int i = 0; i < g->n; i++) {
    graph_dist[i] = malloc(g->n * sizeof(int));
    bfs(g, i, graph_dist[i]);
  }

  double t = -1.0; //will be set later

  // Initial target square from current positions.
  double minx0 = +INFINITY, maxx0 = -INFINITY,
         miny0 = +INFINITY, maxy0 = -INFINITY;
  for (int i = 0; i < g->n; i++) {
    if (g->nodes[i].x < minx0)
      minx0 = g->nodes[i].x;
    if (g->nodes[i].x > maxx0)
      maxx0 = g->nodes[i].x;
    if (g->nodes[i].y < miny0)
      miny0 = g->nodes[i].y;
    if (g->nodes[i].y > maxy0)
      maxy0 = g->nodes[i].y;
  }
  double target_cx = 0.5 * (minx0 + maxx0);
  double target_cy = 0.5 * (miny0 + maxy0);
  double target_size = fmax(maxx0 - minx0, maxy0 - miny0);
  if (target_size < 1.0)
    target_size = 1.0;
  target_size *= 1.10; // initial margin
  const double base_target_size = target_size;

  for (int iter=0; iter < max_iter; iter++) {
    double maxDelta = 0.0;
    for (int i=0; i < g->n; i++)
      g->nodes[i].dx = g->nodes[i].dy = 0;

    // Current occupied box
    double minx = +INFINITY, maxx = -INFINITY,
           miny = +INFINITY, maxy = -INFINITY;
    for (int i = 0; i < g->n; i++) {
      if (g->nodes[i].x < minx)
        minx = g->nodes[i].x;
      if (g->nodes[i].x > maxx)
        maxx = g->nodes[i].x;
      if (g->nodes[i].y < miny)
        miny = g->nodes[i].y;
      if (g->nodes[i].y > maxy)
        maxy = g->nodes[i].y;
    }
    double deltax = maxx - minx, deltay = maxy - miny;
    double occupied = fmax(deltax, deltay);
    if (occupied < 1.0)
      occupied = 1.0;

    // Barnes-Hut square follows current cloud (stable approximation).
    double width = occupied * 1.10; // local margin
    double centerx = 0.5 * (minx + maxx);
    double centery = 0.5 * (miny + maxy);

    // Construire le quadtree
    QuadTree* qt = new_quadtree(centerx, centery, width);
    for (int i=0; i < g->n; i++)
      insert_quadtree(qt, &g->nodes[i]);

    // Forces répulsives via Barnes-Hut
    double k = occupied / sqrt(g->n);
    for (int i = 0; i < g->n; i++)
      compute_force(&g->nodes[i], qt, THETA, k, graph_dist[i], d);

    // Forces attractives (each undirected edge once)
    for (int i = 0; i < g->n; i++) {
      for (int j = 0; j < g->nodes[i].degree; j++) {
        int nb = g->nodes[i].neighbors[j];
        if (nb <= i)
          continue;
        Node* u = &g->nodes[i];
        Node* v = &g->nodes[nb];
        double dx = v->x - u->x;
        double dy = v->y - u->y;
        double dist = sqrt(dx*dx + dy*dy);
        if (dist < DIST_EPS)
          dist = DIST_EPS;
        double f = dist*dist / k;
        u->dx += dx/dist*f; u->dy += dy/dist*f;
        v->dx -= dx/dist*f; v->dy -= dy/dist*f;
      }
    }

    // Gravité vers le centre
    for (int i = 0; i < g->n; i++) {
      double gx = target_cx - g->nodes[i].x;
      double gy = target_cy - g->nodes[i].y;
      g->nodes[i].dx += gx * grav_strength * k;
      g->nodes[i].dy += gy * grav_strength * k;
    }

    // Appliquer déplacements
    if (t < 0.0)
      t = INIT_TEMP_FACTOR * k;
    for (int i=0; i < g->n; i++) {
      double dx = g->nodes[i].dx,
             dy = g->nodes[i].dy;
      double disp = sqrt(dx*dx + dy*dy);
      if (disp > MIN_DELTA) {
        double deltaX = dx/disp * fmin(disp, t),
               deltaY = dy/disp * fmin(disp, t);
        g->nodes[i].x += deltaX;
        g->nodes[i].y += deltaY;
        double delta = sqrt(deltaX*deltaX + deltaY*deltaY);
        if (delta > maxDelta)
          maxDelta = delta;
      }

    }

    // Keep cloud inside the target square without hard clamping points
    // (avoids "all points stuck on border" artifacts).
    double post_minx = +INFINITY, post_maxx = -INFINITY,
           post_miny = +INFINITY, post_maxy = -INFINITY;
    for (int i = 0; i < g->n; i++) {
      if (g->nodes[i].x < post_minx)
        post_minx = g->nodes[i].x;
      if (g->nodes[i].x > post_maxx)
        post_maxx = g->nodes[i].x;
      if (g->nodes[i].y < post_miny)
        post_miny = g->nodes[i].y;
      if (g->nodes[i].y > post_maxy)
        post_maxy = g->nodes[i].y;
    }
    double post_occ = fmax(post_maxx - post_minx, post_maxy - post_miny);
    if (post_occ > target_size * TARGET_FILL_RATIO) {
      double scale = (target_size * TARGET_FILL_RATIO) / post_occ;
      for (int i = 0; i < g->n; i++) {
        g->nodes[i].x = target_cx + (g->nodes[i].x - target_cx) * scale;
        g->nodes[i].y = target_cy + (g->nodes[i].y - target_cy) * scale;
      }
    }

    // Allow a mild square growth when the cloud approaches boundaries.
    double occupancy_ratio = occupied / target_size;
    if (occupancy_ratio > 0.96 && target_size < base_target_size * MAX_GLOBAL_GROWTH)
      target_size = fmin(target_size * MAX_GROWTH_PER_ITER,
                         base_target_size * MAX_GLOBAL_GROWTH);

    t *= COOLING;
    free_quadtree(qt);
    if (maxDelta < EPS)
      break;
  }

  for (int i = 0; i < g->n; i++)
    free(graph_dist[i]);
  free(graph_dist);
}
