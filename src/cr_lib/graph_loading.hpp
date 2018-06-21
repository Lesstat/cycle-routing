/*
  Cycle-routing does multi-criteria route planning for bicycles.
  Copyright (C) 2018  Florian Barth

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "graph.hpp"
#include <boost/filesystem.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

using ms = std::chrono::milliseconds;

Node createNode(std::ifstream& graph, std::ifstream& labels)
{
  size_t id, osmId, level;
  double lat, lng;
  double height;

  graph >> id >> osmId >> lat >> lng >> height >> level;
  labels >> level;

  Node n{ NodeId{ id }, Lat(lat), Lng(lng), height };
  n.assignLevel(level);
  return n;
}

Edge createEdge(std::ifstream& ch, std::ifstream& skips)
{

  size_t source, dest;
  double length, height, unsuitability;
  long edgeA, edgeB;

  ch >> source >> dest >> length >> height >> unsuitability;
  skips >> edgeA >> edgeB;

  Edge e{ NodeId(source), NodeId(dest) };
  if (edgeA > 0) {
    e.edgeA = EdgeId{ static_cast<size_t>(edgeA) };
    e.edgeB = EdgeId{ static_cast<size_t>(edgeB) };
  }

  Cost cost;
  cost.length = Length(length);
  cost.height = Height(height);
  cost.unsuitability = Unsuitability(unsuitability);

  e.setCost(cost);
  assert(e.cost.length >= 0);
  assert(e.cost.height >= 0);
  assert(e.cost.unsuitability >= 0);
  return e;
}

Graph readMultiFileGraph(std::string graphPath)
{
  using namespace boost::filesystem;

  auto directory = canonical(graphPath).parent_path();

  path chGraph{ directory };
  chGraph /= "ch_graph";

  path nodeLabels{ directory };
  nodeLabels /= "node_labels";

  path skips{ directory };
  skips /= "skips";

  std::ifstream graphFile{};
  graphFile.open(graphPath);

  std::ifstream chFile{};
  chFile.open(chGraph.string());
  std::ifstream nodeLabelsFile{ nodeLabels.string() };
  std::ifstream skipsFile{ skips.string() };

  std::cout << "Reading Graphdata" << '\n';
  auto start = std::chrono::high_resolution_clock::now();

  std::string line;
  std::getline(graphFile, line);
  while (line.front() == '#') {
    std::getline(graphFile, line);
  }
  size_t nodeCountGraph = 0;
  size_t nodeCountCh = 0;

  graphFile >> nodeCountGraph;
  chFile >> nodeCountCh;

  if (nodeCountGraph != nodeCountCh) {
    throw std::invalid_argument("node counts of ch and graph do not match");
  }

  size_t edgeCount = 0;
  graphFile >> edgeCount;
  chFile >> edgeCount;

  std::vector<Node> nodes;
  nodes.reserve(nodeCountGraph);
  std::vector<Edge> edges;
  edges.reserve(edgeCount);

  for (size_t i = 0; i < nodeCountGraph; ++i) {
    nodes.push_back(createNode(graphFile, nodeLabelsFile));
  }

  for (size_t i = 0; i < edgeCount; ++i) {
    edges.push_back(createEdge(chFile, skipsFile));
  }
  Graph g{ std::move(nodes), std::move(edges) };
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "creating the graph took " << std::chrono::duration_cast<ms>(end - start).count()
            << "ms" << '\n';
  return g;
}

Graph loadGraphFromTextFile(std::string& graphPath)
{
  const size_t N = 256 * 1024;
  char buffer[N];
  std::ifstream graphFile{};
  graphFile.rdbuf()->pubsetbuf((char*)buffer, N);
  graphFile.open(graphPath);

  std::cout << "Reading Graphdata" << '\n';
  auto start = std::chrono::high_resolution_clock::now();
  Graph g = Graph::createFromStream(graphFile);
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "creating the graph took " << std::chrono::duration_cast<ms>(end - start).count()
            << "ms" << '\n';
  return g;
}

Graph loadGraphFromBinaryFile(std::string& graphPath)
{
  std::ifstream binFile{ graphPath };
  boost::archive::binary_iarchive bin{ binFile };

  std::cout << "Reading Graphdata" << '\n';
  auto start = std::chrono::high_resolution_clock::now();
  Graph g = Graph::createFromBinaryFile(bin);
  auto end = std::chrono::high_resolution_clock::now();

  std::cout << "creating the graph took " << std::chrono::duration_cast<ms>(end - start).count()
            << "ms" << '\n';
  return g;
}
