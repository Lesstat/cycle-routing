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
#include "graph.hpp"
#include <algorithm>
#include <cassert>

std::vector<Edge> Edge::edges{};

Edge::Edge(NodeId source, NodeId dest)
    : Edge(source, dest, {}, {})
{
}

Edge::Edge(NodeId source, NodeId dest, ReplacedEdge edgeA, ReplacedEdge edgeB)
    : source(source)
    , destination(dest)
    , edgeA(std::move(edgeA))
    , edgeB(std::move(edgeB))
{
  assert(source != dest);
  if (edges.size() > 0 && (edgeA || edgeB)) {
    auto& e1 = Edge::getEdge(*edgeA);
    auto& e2 = Edge::getEdge(*edgeB);
    if (e1.getSourceId() == e2.getSourceId()) {
      std::cerr << "Same starting point" << '\n';
      std::terminate();
    }
  }
}

NodeId Edge::getSourceId() const { return source; }
NodeId Edge::getDestId() const { return destination; }

Edge Edge::createFromText(const std::string& text)
{

  size_t source, dest;
  double length, height, unsuitability;
  long edgeA, edgeB;

  std::sscanf(text.c_str(), "%lu%lu%lf%lf%lf%li%li", &source, &dest, &length, &height, // NOLINT
      &unsuitability, &edgeA, &edgeB); // NOLINT

  Edge e{ NodeId(source), NodeId(dest) };
  if (edgeA > 0) {
    e.edgeA = EdgeId{ static_cast<size_t>(edgeA) };
    e.edgeB = EdgeId{ static_cast<size_t>(edgeB) };
  }
  e.cost = Cost({ length, height, unsuitability });
  for (double c : e.cost.values) {
    if (0 > c) {
      throw std::invalid_argument("Cost below zero: " + std::to_string(c));
    }
  }
  return e;
}

double Edge::costByConfiguration(const Config& conf) const { return cost * conf; }

EdgeId Edge::getId() const { return internalId; }

const Cost& Edge::getCost() const { return cost; }

void Edge::setCost(Cost c) { this->cost = c; }

const ReplacedEdge& Edge::getEdgeA() const { return edgeA; }
const ReplacedEdge& Edge::getEdgeB() const { return edgeB; }

HalfEdge Edge::makeHalfEdge(NodePos begin, NodePos end) const
{
  HalfEdge e;
  e.id = internalId;
  e.begin = begin;
  e.end = end;
  e.cost = cost;
  return e;
}

void Edge::setId(EdgeId id) { this->internalId = id; }

double HalfEdge::costByConfiguration(const Config& conf) const { return cost * conf; }

void Edge::administerEdges(std::vector<Edge>& edges)
{
  for (auto& edge : edges) {
    if (edge.getId() == 0) {
      edge.setId(EdgeId{ Edge::edges.size() });
    } else if (edge.getId() != Edge::edges.size()) {
      std::cerr << "Edge ids dont align" << '\n';
    }
    Edge::edges.emplace_back(edge);
  }
}
const Edge& Edge::getEdge(EdgeId id) { return edges.at(id); }
Edge& Edge::getMutEdge(EdgeId id) { return edges.at(id); }

double Cost::operator*(const Config& conf) const
{
  double combinedCost = 0;

  for (size_t i = 0; i < dim; ++i) {
    combinedCost += values[i] * conf.values[i];
  }

  if (!(combinedCost >= 0)) {
    std::cout << "Cost < 0 detected" << '\n';
    for (size_t i = 0; i < dim; ++i) {
      std::cout << "metric " << i << ": " << values[i] << " * " << conf.values[i] << '\n';
    }
    throw std::invalid_argument("cost < 0");
  }
  return combinedCost;
}

NodePos Edge::sourcePos() const { return sourcePos_; }
NodePos Edge::destPos() const { return destPos_; }
void Edge::sourcePos(NodePos source) { sourcePos_ = source; }
void Edge::destPos(NodePos dest) { destPos_ = dest; }
