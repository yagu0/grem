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
#define MIN_FILL_RATIO 0.55
#define DEFAULT_NODE_EDGE_CUTOFF_FACTOR 0.55
#define DEFAULT_NODE_EDGE_REPULSION 0.15

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

static void apply_node_edge_repulsion(Graph* g, double k,
                                      double cutoff_factor,
                                      double repulsion_strength)
{
  if (cutoff_factor <= 0.0 || repulsion_strength <= 0.0)
    return;

  double cutoff = cutoff_factor * k;
  double cutoff2 = cutoff * cutoff;
  double strength = repulsion_strength * k * k;

  for (int u = 0; u < g->n; u++) {
    Node* nu = &g->nodes[u];
    for (int jj = 0; jj < nu->degree; jj++) {
      int v = nu->neighbors[jj];
      if (v <= u)
        continue; // each edge once

      Node* nv = &g->nodes[v];
      double ex = nv->x - nu->x;
      double ey = nv->y - nu->y;
      double edge_len2 = ex * ex + ey * ey;
      if (edge_len2 < DIST_EPS)
        continue;

      for (int p = 0; p < g->n; p++) {
        if (p == u || p == v)
          continue;

        Node* np = &g->nodes[p];
        double px = np->x - nu->x;
        double py = np->y - nu->y;
        double alpha = (px * ex + py * ey) / edge_len2;
        if (alpha <= 0.0 || alpha >= 1.0)
          continue; // closest point outside segment

        double qx = nu->x + alpha * ex;
        double qy = nu->y + alpha * ey;
        double dx = np->x - qx;
        double dy = np->y - qy;
        double d2 = dx * dx + dy * dy;
        if (d2 >= cutoff2)
          continue;

        double dist = sqrt(d2);
        if (dist < DIST_EPS)
          dist = DIST_EPS;

        // Strong local repulsion when a node gets close to an edge.
        double f = strength * (1.0 / dist - 1.0 / cutoff) / dist;
        double fx = (dx / dist) * f;
        double fy = (dy / dist) * f;

        np->dx += fx;
        np->dy += fy;

        // Split opposite reaction on edge endpoints by barycentric weights.
        nu->dx -= fx * (1.0 - alpha);
        nu->dy -= fy * (1.0 - alpha);
        nv->dx -= fx * alpha;
        nv->dy -= fy * alpha;
      }
    }
  }
}

void spring_layout(Graph* g, int max_iter, int d, double grav_strength,
                   double node_edge_repulsion, double node_edge_cutoff_factor)
{
  if (g == NULL || g->n <= 1)
    return;

  // Negative values mean: use internal defaults.
  if (node_edge_repulsion < 0.0)
    node_edge_repulsion = DEFAULT_NODE_EDGE_REPULSION;
  if (node_edge_cutoff_factor < 0.0)
    node_edge_cutoff_factor = DEFAULT_NODE_EDGE_CUTOFF_FACTOR;

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

    // Keep target square size adaptive (no geometric forcing on points).
    double desired_size = occupied * 1.20;
    if (desired_size < base_target_size)
      desired_size = base_target_size;
    double max_size = base_target_size * MAX_GLOBAL_GROWTH;
    if (desired_size > max_size)
      desired_size = max_size;
    if (desired_size > target_size)
      target_size = fmin(target_size * MAX_GROWTH_PER_ITER, desired_size);
    else
      target_size = fmax(target_size / MAX_GROWTH_PER_ITER, desired_size);

    // Forces répulsives via Barnes-Hut
    double k = target_size / sqrt(g->n);
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

    // Anti-crossing term: keep nodes away from non-incident edges.
    apply_node_edge_repulsion(g, k, node_edge_cutoff_factor, node_edge_repulsion);

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

    // If cloud collapses too much, very slightly reheat by slowing cooling.
    double occupancy_ratio = occupied / target_size;
    if (occupancy_ratio < MIN_FILL_RATIO)
      t /= COOLING;

    t *= COOLING;
    free_quadtree(qt);
    if (maxDelta < EPS)
      break;
  }

  for (int i = 0; i < g->n; i++)
    free(graph_dist[i]);
  free(graph_dist);
}
