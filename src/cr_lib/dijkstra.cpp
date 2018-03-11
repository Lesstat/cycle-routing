/*
  Cycle-routing does multi-criteria route planning for bicycles.
  Copyright (C) 2017  Florian Barth

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
#include "dijkstra.hpp"
#include <queue>

const double dmax = std::numeric_limits<double>::max();
const short smax = std::numeric_limits<short>::max();
const Cost maxCost{ Length(dmax), Height(smax), Unsuitability(smax) };

Dijkstra::Dijkstra(Graph* g, size_t nodeCount)
    : costS(nodeCount, dmax)
    , costT(nodeCount, dmax)
    , graph(g)
{
}

void Dijkstra::clearState()
{
  for (auto nodeId : touchedS) {
    costS[nodeId] = dmax;
  }
  touchedS.clear();
  for (auto nodeId : touchedT) {
    costT[nodeId] = dmax;
  }
  touchedT.clear();
}

void insertUnpackedEdge(const Edge& e, std::deque<Edge>& route, bool front)
{
  const auto& edgeA = e.getEdgeA();
  const auto& edgeB = e.getEdgeB();

  if (edgeA) {
    if (front) {
      insertUnpackedEdge(Edge::getEdge(*edgeB), route, front);
      insertUnpackedEdge(Edge::getEdge(*edgeA), route, front);
    } else {
      insertUnpackedEdge(Edge::getEdge(*edgeA), route, front);
      insertUnpackedEdge(Edge::getEdge(*edgeB), route, front);
    }
  } else {
    if (front) {
      route.push_front(e);
    } else {
      route.push_back(e);
    }
  }
}

Route Dijkstra::buildRoute(NodePos node, NodeToEdgeMap previousEdgeS, NodeToEdgeMap previousEdgeT,
    NodePos from, NodePos to)
{

  Route route{};
  auto curNode = node;
  while (curNode != from) {
    const auto& edge = previousEdgeS[curNode];
    route.costs = route.costs + edge.cost;
    insertUnpackedEdge(Edge::getEdge(edge.id), route.edges, true);
    curNode = edge.begin;
  }

  curNode = node;
  while (curNode != to) {
    const auto& edge = previousEdgeT[curNode];
    route.costs = route.costs + edge.cost;
    insertUnpackedEdge(Edge::getEdge(edge.id), route.edges, false);
    curNode = edge.begin;
  }

  return route;
}

bool Dijkstra::QueueComparator::operator()(QueueElem left, QueueElem right)
{
  auto leftCost = std::get<double>(left);
  auto rightCost = std::get<double>(right);
  return leftCost > rightCost;
};

std::optional<Route> Dijkstra::findBestRoute(NodePos from, NodePos to, Config config)
{

  clearState();
  this->config = config;
  Dijkstra::Queue heapS{ QueueComparator{} };

  heapS.push(std::make_pair(from, 0));
  touchedS.push_back(from);
  costS[from] = 0;

  NodeToEdgeMap previousEdgeS{};

  Queue heapT{ QueueComparator{} };

  heapT.push(std::make_pair(to, 0));
  touchedT.push_back(to);
  costT[to] = 0;

  NodeToEdgeMap previousEdgeT{};

  bool sBigger = false;
  bool tBigger = false;
  double minCandidate = dmax;
  std::optional<NodePos> minNode = {};

  while (true) {
    // Quit if both are empty or one is empty and the other is bigger than minCandidate
    if ((heapS.empty() && heapT.empty()) || (heapS.empty() && tBigger)
        || (heapT.empty() && sBigger)) {
      if (minNode.has_value()) {
        return buildRoute(minNode.value(), previousEdgeS, previousEdgeT, from, to);
      }
      return {};
    }
    if (sBigger && tBigger) {
      return buildRoute(minNode.value(), previousEdgeS, previousEdgeT, from, to);
    }

    if (!heapS.empty() && !sBigger) {
      auto[node, cost] = heapS.top();
      heapS.pop();
      if (cost > costS[node]) {
        continue;
      }
      if (stallOnDemand(node, cost, Direction::S)) {
        continue;
      }

      if (cost > minCandidate) {
        sBigger = true;
        continue;
      }
      if (costT[node] != dmax) {
        double candidate = costT[node] + costS[node];
        if (candidate < minCandidate) {
          minCandidate = candidate;
          minNode = node;
        }
      }

      relaxEdges(node, cost, Direction::S, heapS, previousEdgeS);
    }

    if (!heapT.empty() && !tBigger) {
      auto[node, cost] = heapT.top();
      heapT.pop();
      if (cost > costT[node]) {
        continue;
      }
      if (stallOnDemand(node, cost, Direction::T)) {
        continue;
      }

      if (cost > minCandidate) {
        tBigger = true;
        continue;
      }
      if (costS[node] != dmax) {
        double candidate = costS[node] + costT[node];
        if (candidate < minCandidate) {
          minCandidate = candidate;
          minNode = node;
        }
      }
      relaxEdges(node, cost, Direction::T, heapT, previousEdgeT);
    }
  }
}

void Dijkstra::relaxEdges(
    const NodePos& node, double cost, Direction dir, Queue& heap, NodeToEdgeMap& previousEdge)
{
  std::vector<double>& costs = dir == Direction::S ? costS : costT;
  std::vector<NodePos>& touched = dir == Direction::S ? touchedS : touchedT;

  auto myLevel = graph->getLevelOf(node);
  auto edges
      = dir == Direction::S ? graph->getOutgoingEdgesOf(node) : graph->getIngoingEdgesOf(node);
  for (const auto& edge : edges) {
    NodePos nextNode = edge.end;
    if (graph->getLevelOf(nextNode) < myLevel) {
      return;
    }
    double nextCost = cost + edge.costByConfiguration(config);
    if (nextCost < costs[nextNode]) {
      costs[nextNode] = nextCost;
      touched.push_back(nextNode);
      previousEdge[nextNode] = edge;
      QueueElem next = std::make_pair(nextNode, nextCost);
      heap.push(next);
    }
  }
}

bool Dijkstra::stallOnDemand(const NodePos& node, double cost, Direction dir)
{
  auto myLevel = graph->getLevelOf(node);
  const auto& edges
      = dir == Direction::S ? graph->getIngoingEdgesOf(node) : graph->getOutgoingEdgesOf(node);
  auto& costs = dir == Direction::S ? costS : costT;
  for (const auto& edge : edges) {
    if (graph->getLevelOf(edge.end) < myLevel) {
      return false;
    }
    if (costs[edge.end] + edge.cost * config < cost) {
      return true;
    }
  }

  return false;
}
