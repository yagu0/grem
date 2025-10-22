// python/wrapper.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>   // pour std::shared_ptr
#include <vector>

extern "C" {
  #include "../c_project/src/graph.h"
  #include "../c_project/src/graph_dist.h"
  #include "../c_project/src/spring_embed.h"
}

namespace py = pybind11;

/////////////////////////////
// Utilitaires de conversion
/////////////////////////////

static std::vector<int> node_neighbors(const Node& node) {
  std::vector<int> neigh;
  if (node.degree <= 0) return neigh;
  if (node.neighbors == nullptr)
    throw std::runtime_error("node.neighbors is null despite degree > 0");

  neigh.reserve(node.degree);
  for (int i = 0; i < node.degree; ++i)
    neigh.push_back(node.neighbors[i]);
  return neigh;
}

static py::list graph_to_list(const Graph& g) {
  py::list L;
  for (int i = 0; i < g.n; ++i) {
    const Node& nd = g.nodes[i];
    py::dict d;
    d["id"] = nd.id;
    d["x"] = nd.x;
    d["y"] = nd.y;
    d["degree"] = nd.degree;
    d["neighbors"] = node_neighbors(nd);
    L.append(d);
  }
  return L;
}

/////////////////////////////
// RAII Memory Management
/////////////////////////////

// A "deleter" for std::shared_ptr<Graph> which call free_graph()
struct GraphDeleter {
  void operator()(Graph* g) const noexcept {
    if (g) free_graph(*g);
  }
};

// Small helper building a std::shared_ptr<Graph>
inline std::shared_ptr<Graph> make_graph(Graph g) {
  // We make a local copy of the Graph returned by the C function
  // then pass its address to a shared_ptr with a custom deleter
  Graph* heap_copy = (Graph*)malloc(sizeof(Graph));
  *heap_copy = g;
  return std::shared_ptr<Graph>(heap_copy, GraphDeleter());
}

/////////////////////////////
// Pybind11 Module
/////////////////////////////

PYBIND11_MODULE(_native, m) {
  m.doc() = "Python interface to grem C methods, with automatic memory management (RAII).";

  // -----------------
  // Struct Node
  // -----------------
  py::class_<Node>(m,
    "Node",
    R"pbdoc(
    Node data structure.

    This class represents a node in a graph.
    It has a position and a list of neighbors.

    Attributes
    ----------
    id : int
      Identifier (number) of the node.

    x : float
      Abscissa of the node

    y : float
      Ordinate of the node

    degree : int
      Number of neighbors (adjacent edges).

    nodes : List[Node]
            The list of neighbor nodes.
    )pbdoc")
    .def_property_readonly("id", [](const Node& n){ return n.id; })
    .def_readwrite("x", &Node::x)
    .def_readwrite("y", &Node::y)
    .def_property_readonly("degree", [](const Node& n){ return n.degree; })
    .def_property_readonly("neighbors", [](const Node& n){ return node_neighbors(n); })
    .def("__repr__", [](const Node& n){
      return "<Node id=" + std::to_string(n.id) +
             " (" + std::to_string(n.x) + "," + std::to_string(n.y) + ")>";
    });

  // -----------------
  // Struct Graph (handled by shared_ptr)
  // -----------------
  py::class_<Graph, std::shared_ptr<Graph>>(m,
    "Graph",
    R"pbdoc(
    Graph data structure.

    This class represents a graph with nodes and edges.
    Each node has a position and a list of neighbors.

    Attributes
    ----------
    n : Int
        The number of nodes.

    nodes : List[Node]
            The list of nodes in the graph.
    )pbdoc")
    .def_property_readonly("n", [](const Graph& g){ return g.n; })
    .def_property_readonly("nodes", [](const Graph& g){ return graph_to_list(g); })
    .def("__repr__", [](const Graph& g){
      return "<Graph n=" + std::to_string(g.n) + ">";
    });

  // -----------------
  // Exposed functions
  // -----------------

  // Génération de graphes
  m.def(
    "make_random_graph",
    [](int n, double p, double width, int seed) {
      return make_graph(make_random_graph(n, p, width, seed));
    },
    py::arg("n"), py::arg("p"), py::arg("width"), py::arg("seed") = -1);

  m.def(
    "make_random_tree",
    [](int n, int mode, double width, int seed) {
      return make_graph(make_random_tree(n, mode, width, seed));
    },
    py::arg("n"), py::arg("mode"), py::arg("width"), py::arg("seed") = -1);

  // Spring layout
  m.def(
    "spring_layout",
    [](std::shared_ptr<Graph> g, int max_iter, double width) {
      spring_layout(g.get(), max_iter, width);
    },
    py::arg("graph"), py::arg("max_iter"), py::arg("width"));
}
