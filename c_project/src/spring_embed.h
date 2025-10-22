#ifndef GREM_SPRING_EMBED_H
#define GREM_SPRING_EMBED_H

#include "graph.h"

typedef struct QuadTree {
  double cx, cy; //centre du carré
  double size; //longueur du côté
  int mass; //masse totale (nb de points)
  double mx, my; //centre de masse
  Node* node; //point contenu (ou NULL)
  struct QuadTree* subtree[4]; //NW, NE, SW, SE
} QuadTree;

// Fonction principale
void spring_layout(Graph* g, int max_iter, double width);

#endif
