import igraph as ig

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
