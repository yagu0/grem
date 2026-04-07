import igraph as ig
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.collections as mc

def plot_graph(*, g=None, path=None, width=600, height=600, vertex_size=8):
    """
    Draw a graph, given as an objet grem.Graph or read from a text file.

    Mutually exclusive parameters:
      g    : a graph object as output by make_random_tree()
      path : path to a graph text file in format:
             n m
             x y  (for each node)
             u v  (for each vertex)

    Other parameters
      width, height : size of the canvas in pixels
      vertex_size   : radius of a vertex

    Example :
        import grem
        g = grem.make_random_graph(30, 0.1, 100)
        grem.plot_graph(g=g)

        # or
        grem.plot_graph(path="graph.txt")

        # In interactive mode, use:
        fig = grem.plot_graph(...)
        # and then
        fig.save("file.png")
    """

    if (g is None) == (path is None):
        raise ValueError("Specify either 'g' or 'path', but not both.")

    if path:
        with open(path) as f:
            lines = f.read().strip().splitlines()

        if not lines:
            raise ValueError(f"Empty file: {path}")

        n, m = map(int, lines[0].split())
        cc = [tuple(map(float, lines[i + 1].split())) for i in range(n)]
        coords = [ [c[0], c[1]] for c in cc ]
        colors = [ c[2] for c in cc ]
        edges = [tuple(map(int, lines[n + 1 + j].split())) for j in range(m)]

    else:
        # g est un objet Graph (exposé par le wrapper C++)
        n = len(g.nodes)
        coords = [ (nd["x"], nd["y"]) for nd in g.nodes ]
        colors = [ nd["color"] for nd in g.nodes ]
        edges = []
        for i, nd in enumerate(g.nodes):
            for j in nd["neighbors"]:
                if i < j: #éviter les doublons dans un graphe non orienté
                    edges.append((i, j))

    graph = ig.Graph(edges=edges, directed=False)

    pal = ig.drawing.colors.RainbowPalette(n=n) #TODO: less than n?
    fig = ig.plot(
        graph,
        layout=coords,
        vertex_size=vertex_size,
        vertex_color=[pal.get(c) for c in colors],
        edge_color="grey",
        bbox=(width, height),
        target=None, #fenêtre interactive si possible
    )

    # Retourner le plot pour l'utilisateur
    return fig

def animate_graph(*, g=None, path=None, interval=50):
    """ TODO """

    if (g is None) == (path is None):
        raise ValueError("Specify either 'g' or 'path', but not both.")

    if path:
        with open(path) as f:
            lines = f.read().strip().splitlines()
        n, m = map(int, lines[0].split())
        cc = [tuple(map(float, lines[i + 1].split())) for i in range(n)]
        coords = [[c[0], c[1]] for c in cc]
        colors = [c[2] for c in cc]
        edges = [tuple(map(int, lines[n + 1 + j].split())) for j in range(m)]
    else:
        n = len(g.nodes)
        coords = [(nd["x"], nd["y"]) for nd in g.nodes]
        colors = [nd["color"] for nd in g.nodes]
        edges = []
        for i, nd in enumerate(g.nodes):
            for j in nd["neighbors"]:
                if i < j:
                    edges.append((i, j))

    # --- 2. Préparation du graphique ---
    fig, ax = plt.subplots(figsize=(8, 8))
    
    # Définition des limites de la vue
    all_x = [c[0] for c in coords]
    all_y = [c[1] for c in coords]
    ax.set_xlim(min(all_x) - 1, max(all_x) + 1)
    ax.set_ylim(min(all_y) - 1, max(all_y) + 1)
    ax.set_aspect('equal')
    ax.axis('off') # On cache les axes pour un look plus "graphe"

    # Création des objets graphiques vides
    # LineCollection est beaucoup plus performant que dessiner chaque ligne une par une
    line_coll = mc.LineCollection([], colors='grey', alpha=0.4, linewidths=1, zorder=1)
    ax.add_collection(line_coll)
    
    scatter = ax.scatter([], [], s=80, c=[], cmap='rainbow', edgecolors='black', zorder=2)

    # --- 3. Fonction d'animation ---
    def update(frame):
        """ frame va de 0 à n-1 """
        # Points à afficher : du premier jusqu'à 'frame'
        current_points = coords[:frame + 1]
        current_colors = colors[:frame + 1]
        
        # Filtrer les arêtes : on n'affiche que celles dont les deux sommets sont <= frame
        # (et qui incluent idéalement le nouveau point pour l'effet visuel)
        current_edges = [
            (coords[u], coords[v]) for u, v in edges 
            if u <= frame and v <= frame
        ]

        # Mise à jour des objets
        scatter.set_offsets(current_points)
        scatter.set_array(current_colors) # Utilise la colormap pour les couleurs
        line_coll.set_segments(current_edges)
        
        return scatter, line_coll

    # Création de l'animation
    ani = FuncAnimation(
        fig, 
        update, 
        frames=n, 
        interval=interval, 
        blit=True, 
        repeat=False
    )

    plt.show()
    return ani

# Exemple d'utilisation :
# ani = animate_graph(path="graph.txt", interval=50)

