/*
  Cycle-routing does multi-criteria route planning for bicycles.
  Copyright (C) 2018  Florian Barth

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useul,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "ndijkstra.hpp"
#include <fstream>
#include <unordered_set>
template <int Dim>
NormalDijkstra<Dim>::NormalDijkstra(GraphD* g, size_t nodeCount, bool unpack)
    : cost(nodeCount, std::numeric_limits<double>::max())
    , paths(nodeCount, 0)
    , previousEdge(nodeCount)
    , pathCount(0)
    , usedConfig(std::vector<double>(Dim, 0.0))
    , graph(g)
    , heap(BiggerPathCost {})
    , unpack(unpack)
{
}

template <int Dim>
std::optional<RouteWithCount<Dim>> NormalDijkstra<Dim>::findBestRoute(
    NodePos from, NodePos to, const ConfigD& config)
{

  usedConfig = config;
  this->from = from;
  this->to = to;
  clearState();
  heap.push(std::make_tuple(from, 0));
  touched.push_back(from);
  cost[from] = 0;
  paths[from] = 1;

  while (true) {
    if (heap.empty()) {
      return {};
    }
    auto [node, pathCost] = heap.top();
    heap.pop();
    ++pqPops;
    if (node == to) {
      return buildRoute(from, to);
    }
    if (pathCost > cost[node]) {
      continue;
    }

    const auto& outEdges = graph->getOutgoingEdgesOf(node);
    for (const auto& edge : outEdges) {
      if (unpack && EdgeD::getEdgeA(edge.id)) {
        continue;
      }
      const NodePos& nextNode = edge.end;
      double nextCost = pathCost + edge.costByConfiguration(config);
      if (nextCost < cost[nextNode]) {
        QueueElem next = std::make_tuple(nextNode, nextCost);
        if (cost[nextNode] == std::numeric_limits<double>::max()) {
          touched.push_back(nextNode);
        }
        cost[nextNode] = nextCost;
        paths[nextNode] = paths[node];
        previousEdge[nextNode] = { edge };
        heap.push(next);
      } else if (std::abs(nextCost - cost[nextNode]) < 0.001) {
        paths[nextNode] += paths[node];
        previousEdge[nextNode].push_back(edge);
      }
    }
  }
}
template <int Dim> void NormalDijkstra<Dim>::clearState()
{
  for (const auto& pos : touched) {
    cost[pos] = std::numeric_limits<double>::max();
    paths[pos] = 0;
    previousEdge[pos].clear();
  }
  while (!heap.empty()) {
    heap.pop();
  }
  touched.clear();
  pathCost = CostD {};
  pathCount = 0;
  pqPops = 0;
}
template <int Dim> void insertUnpackedEdge(const EdgeId& e, std::deque<EdgeId>& route, bool front);
template <int Dim>
RouteWithCount<Dim> NormalDijkstra<Dim>::buildRoute(const NodePos& from, const NodePos& to)
{

  RouteWithCountD route;
  route.pathCount = paths[to];
  auto currentNode = to;
  while (currentNode != from) {
    auto& half_edge = previousEdge.at(currentNode).front();
    route.costs = route.costs + EdgeD::getCost(half_edge.id);
    if (unpack) {
      insertUnpackedEdge<Dim>(half_edge.id, route.edges, true);
    } else {
      route.edges.push_front(half_edge.id);
    }
    currentNode = EdgeD::sourcePos(half_edge.id);
  }

  pathCost = route.costs;
  pathCount = route.pathCount;
  return route;
}
template <int Dim> RouteIterator<Dim> NormalDijkstra<Dim>::routeIter(NodePos from, NodePos to)
{
  return RouteIterator(this, from, to);
}
template <int Dim>
RouteIterator<Dim>::RouteIterator(
    NormalDijkstraD* dijkstra, NodePos from, NodePos to, size_t maxHeapSize)
    : dijkstra(dijkstra)
    , maxHeapSize(maxHeapSize)
    , from(from)
    , to(to)
    , heap(BiggerRouteCost(dijkstra->usedConfig))
{
  RouteWithCountD route;
  route.pathCount = dijkstra->pathCount;
  heap.emplace(route, to);
}

template <int Dim> bool RouteIterator<Dim>::finished()
{
  return outputCount >= dijkstra->pathCount || outputCount >= 150;
}
template <int Dim> void RouteIterator<Dim>::doubleHeapsize() { maxHeapSize *= 2; }

template <int Dim> std::optional<RouteWithCount<Dim>> RouteIterator<Dim>::next()
{
  using EdgeD = Edge<Dim>;

  while (!heap.empty()) {
    if (finished()) {
      return {};
    }
    if (heap.size() > maxHeapSize) {
      return {};
    }

    auto [hRoute, hTo] = heap.top();
    heap.pop();

    if (hTo == from) {
      outputCount++;
      return hRoute;
    }
    for (const auto& edge : dijkstra->previousEdge[hTo]) {
      const auto& source = EdgeD::sourcePos(edge.id);
      if (std::find_if(hRoute.edges.begin(), hRoute.edges.end(),
              [&source](const auto& e) { return source == EdgeD::sourcePos(e); })
          != hRoute.edges.end()) {
        continue;
      }
      RouteWithCount newRoute = hRoute;
      newRoute.costs = newRoute.costs + edge.cost;
      if (std::abs(dijkstra->cost[source] + edge.costByConfiguration(dijkstra->usedConfig)
              - dijkstra->cost[hTo])
              > 0.00000001
          || newRoute.costs * dijkstra->usedConfig > dijkstra->pathCost * dijkstra->usedConfig) {
        continue;
      }
      newRoute.edges.push_front(edge.id);
      heap.emplace(newRoute, source);
    }
  }
  outputCount = dijkstra->pathCount;
  return {};
}

template <int Dim> void NormalDijkstra<Dim>::saveDotGraph(const EdgeId& inId, const EdgeId& outId)
{

  std::ofstream dotFile { "/tmp/" + std::to_string(from) + "-" + std::to_string(to) + ".dot" };
  dotFile << "digraph G{" << '\n';
  dotFile << "rankdir=LR;" << '\n';
  dotFile << "size=8;" << '\n';
  dotFile << "node[ shape = doublecircle color = red]; " << from << " " << to << ";" << '\n';
  dotFile << "node[ shape = circle color = black]; " << '\n';

  for (auto& node : touched) {
    for (auto& edge : graph->getOutgoingEdgesOf(node)) {
      dotFile << node << " -> " << edge.end << " [label = \"";
      bool first = true;
      for (auto& c : edge.cost.values) {
        if (!first)
          dotFile << ", ";
        dotFile << c;
        first = false;
      }
      dotFile << " | " << edge.cost * usedConfig << "\"";
      dotFile << " color = ";
      if (edge.id == inId || edge.id == outId) {
        dotFile << "blue";
      } else {
        dotFile << "black";
      }
      dotFile << " ]" << '\n';
    }
  }
  dotFile << "}" << '\n';
}
