"""
grem : interface Python pour les graphes C (avec RAII)
"""

from . import _native

def make_random_graph(n, p, width, seed = -1):
    """
    make_random_graph(n: int, p: float, width: float, seed: int = -1) -> Graph
    Build a random Erdos–Rényi graph.

    Parameters
    ----------
    n : int
        Number of nodes.
    p : float
        Connexion probability.
    size : float
        Width of the square area.
    seed : int
        Random if unspecified or < 0.

    Returns
    -------
    Graph
        The generated graph object.
    """
    return _native.make_random_graph(n, p, width, seed)

def make_random_tree(n, mode, width, seed = -1):
    """
    make_random_tree(n: int, mode: int, width: float, seed: int = -1) -> Graph
    Build a random tree (RND or PA mode).

    Parameters
    ----------
    n : int
        Number of nodes.
    mode : int
        0 for random( RND), 1 for preferential attachement (PA).
    size : float
        Width of the square area.
    seed : int
        Random if unspecified or < 0.

    Returns
    -------
    Graph
        The generated graph object.
    """
    return _native.make_random_tree(n, mode, width, seed)

def make_random_binary_tree(n, width, seed = -1):
    """
    make_random_binary_tree(n: int, width: float, seed: int = -1) -> Graph
    Build a random (uniform) binary tree.

    Parameters
    ----------
    n : int
        Number of nodes.
    size : float
        Width of the square area.
    seed : int
        Random if unspecified or < 0.

    Returns
    -------
    Graph
        The generated graph object.
    """
    return _native.make_random_binary_tree(n, width, seed)

def make_random_nary_tree(n, alpha, width, seed = -1):
    """
    make_random_nary_tree(n: int, alpha: float, width: float, seed: int = -1) -> Graph
    Build a random (uniform) n-ary tree.

    Parameters
    ----------
    n : int
        Number of nodes.
    alpha : float
        User parameter (TODO: in ]0,1[ ?).
    size : float
        Width of the square area.
    seed : int
        Random if unspecified or < 0.

    Returns
    -------
    Graph
        The generated graph object.
    """
    return _native.make_random_nary_tree(n, alpha, width, seed)

def spring_layout(g, max_iter, d = 2, grav_strength = 0.01):
    """
    spring_layout(g: Graph, max_iter: int, d: int, grav_strength: float) -> None
    Rearrange the positions of nodes in the graph based on attractive and
    repulsive forces applied on nodes through edges.

    Parameters
    ----------
    g : Graph
        Graph object (output of make_random_graph() for example).
    max_iter : int
        Maximum number of iterations.
    d : int
        Exponent for topological modulation. Default to 2
    grav_strength : float
        Gravity strength. Default to 0.01

    Returns
    -------
    None
    """
    _native.spring_layout(g, max_iter, d, grav_strength)

from ._native import (
    Graph,
    Node,
)

from .viz import plot_graph

__all__ = [
    "Graph",
    "Node",
    "make_random_graph",
    "make_random_tree",
    "make_random_binary_tree",
    "make_random_nary_tree",
    "spring_layout",
    "plot_graph",
]
