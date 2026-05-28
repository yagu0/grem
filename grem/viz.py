import igraph as ig
import matplotlib.collections as mc
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.animation import FuncAnimation
from matplotlib.colors import Normalize


def _load_graph_data(*, g=None, path=None):
    if (g is None) == (path is None):
        raise ValueError("Specify either 'g' or 'path', but not both.")

    if path:
        with open(path, encoding="utf-8") as f:
            lines = f.read().strip().splitlines()
        if not lines:
            raise ValueError(f"Empty file: {path}")

        n, m = map(int, lines[0].split())
        cc = [tuple(map(float, lines[i + 1].split())) for i in range(n)]
        coords = [(c[0], c[1]) for c in cc]
        arrival = [c[2] for c in cc]
        edges = [tuple(map(int, lines[n + 1 + j].split())) for j in range(m)]
    else:
        n = len(g.nodes)
        coords = [(nd["x"], nd["y"]) for nd in g.nodes]
        arrival = [nd["color"] for nd in g.nodes]
        edges = []
        for i, nd in enumerate(g.nodes):
            for j in nd["neighbors"]:
                if i < j:
                    edges.append((i, j))

    return n, coords, arrival, edges


def _arrival_norm(arrival):
    amin = min(arrival)
    amax = max(arrival)
    if amax <= amin:
        return [0.5 for _ in arrival], Normalize(vmin=0.0, vmax=1.0)
    scaled = [(a - amin) / (amax - amin) for a in arrival]
    return scaled, Normalize(vmin=0.0, vmax=1.0)


def plot_graph(*, g=None, path=None, width=600, height=600, vertex_size=8, cmap="viridis"):
    """
    Draw a graph from a `grem.Graph` object or a graph file.
    Node colors represent arrival order (early->late via colormap).
    """
    n, coords, arrival, edges = _load_graph_data(g=g, path=path)
    arrival_scaled, _ = _arrival_norm(arrival)

    graph = ig.Graph(n=n, edges=edges, directed=False)
    palette = plt.colormaps[cmap]
    vertex_colors = [palette(v) for v in arrival_scaled]

    return ig.plot(
        graph,
        layout=coords,
        vertex_size=vertex_size,
        vertex_color=vertex_colors,
        edge_color="grey",
        bbox=(width, height),
        target=None,
    )


def animate_graph(
    *,
    g=None,
    path=None,
    inter_node_time=0.1,
    total_time=None,
    width=8,
    height=8,
    vertex_size=80,
    cmap="viridis",
    show_colorbar=True,
    show=True,
    blit=False,
    inline_html=True,
):
    """
    Animate graph growth node by node.

    Timing controls:
      - inter_node_time: seconds between two node arrivals.
      - total_time: total duration in seconds; if provided, it overrides
        inter_node_time by using total_time / n.
    """
    n, coords, arrival, edges = _load_graph_data(g=g, path=path)
    if n == 0:
        raise ValueError("Graph has no nodes.")

    if total_time is not None:
        if total_time <= 0:
            raise ValueError("'total_time' must be > 0.")
        inter_node_time = total_time / n
    if inter_node_time <= 0:
        raise ValueError("'inter_node_time' must be > 0.")

    interval_ms = max(1, int(inter_node_time * 1000))

    arrival_scaled, norm = _arrival_norm(arrival)
    all_x = [c[0] for c in coords]
    all_y = [c[1] for c in coords]
    dx = max(all_x) - min(all_x)
    dy = max(all_y) - min(all_y)
    margin = max(1.0, 0.05 * max(dx, dy, 1.0))

    fig, ax = plt.subplots(figsize=(width, height))
    ax.set_xlim(min(all_x) - margin, max(all_x) + margin)
    ax.set_ylim(min(all_y) - margin, max(all_y) + margin)
    ax.set_aspect("equal")
    ax.axis("off")

    line_coll = mc.LineCollection([], colors="grey", alpha=0.4, linewidths=1, zorder=1)
    ax.add_collection(line_coll)

    scatter = ax.scatter(
        np.array([]),
        np.array([]),
        s=vertex_size,
        c=np.array([]),
        cmap=cmap,
        norm=norm,
        edgecolors="black",
        linewidths=0.3,
        zorder=2,
    )

    if show_colorbar:
        mappable = plt.cm.ScalarMappable(norm=norm, cmap=cmap)
        mappable.set_array([])
        cbar = fig.colorbar(mappable, ax=ax, fraction=0.046, pad=0.04)
        cbar.set_label("Arrival order")

    def update(visible_count):
        current_points = np.array(coords[:visible_count], dtype=float)
        current_arrival = np.array(arrival_scaled[:visible_count], dtype=float)
        current_edges = [
            (coords[u], coords[v])
            for u, v in edges
            if u < visible_count and v < visible_count
        ]

        scatter.set_offsets(current_points)
        scatter.set_array(current_arrival)
        line_coll.set_segments(current_edges)
        return scatter, line_coll

    # Show first node immediately and avoid blank window on fragile backends.
    update(1)

    ani = FuncAnimation(
        fig,
        update,
        frames=range(1, n + 1),
        interval=interval_ms,
        blit=blit,
        repeat=False,
    )

    if show:
        backend = plt.get_backend().lower()
        is_inline = "inline" in backend
        if inline_html and is_inline:
            try:
                from IPython.display import HTML, display
                display(HTML(ani.to_jshtml()))
                plt.close(fig)
            except Exception:
                plt.show()
        else:
            plt.show()
    return ani

