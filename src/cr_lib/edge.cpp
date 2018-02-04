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
#include <cassert>

std::atomic<size_t> Edge::lastId = 0;
std::vector<Edge> Edge::edges{};

Edge::Edge(NodeId source, NodeId dest)
    : Edge(source, dest, {}, {})
{
}

Edge::Edge(NodeId source, NodeId dest, ReplacedEdge edgeA, ReplacedEdge edgeB)
    : internalId(lastId++)
    , source(source)
    , destination(dest)
    , edgeA(std::move(edgeA))
    , edgeB(std::move(edgeB))
{
  assert(source != dest);
}

NodeId Edge::getSourceId() const { return source; }
NodeId Edge::getDestId() const { return destination; }
NodePos Edge::getSourcePos() const { return sourcePos; }
NodePos Edge::getDestPos() const { return destinationPos; }

void Edge::setDestPos(NodePos p) { destinationPos = p; }

void Edge::setSourcePos(NodePos p) { sourcePos = p; }

Edge Edge::createFromText(const std::string& text)
{

  size_t source, dest;
  double length;
  short height, unsuitability;
  long edgeA, edgeB;

  std::sscanf(text.c_str(), "%lu%lu%lf%hd%hd%li%li", &source, &dest, &length, &height, // NOLINT
      &unsuitability, &edgeA, &edgeB); // NOLINT

  Edge e{ NodeId(source), NodeId(dest) };
  if (edgeA > 0) {
    e.edgeA = EdgeId{ static_cast<size_t>(edgeA) };
    e.edgeB = EdgeId{ static_cast<size_t>(edgeB) };
  }
  e.cost.length = Length(length);
  e.cost.height = Height(height);
  e.cost.unsuitability = Unsuitability(unsuitability);
  assert(e.cost.length >= 0);
  assert(e.cost.height >= 0);
  assert(e.cost.unsuitability >= 0);
  return e;
}

double Edge::costByConfiguration(const Config& conf) const { return cost * conf; }

EdgeId Edge::getId() const { return internalId; }

const Cost& Edge::getCost() const { return cost; }

void Edge::setCost(Cost c) { this->cost = c; }

const ReplacedEdge& Edge::getEdgeA() const { return edgeA; }
const ReplacedEdge& Edge::getEdgeB() const { return edgeB; }

HalfEdge Edge::makeOutEdge() const
{
  HalfEdge e;
  e.id = internalId;
  e.end = destinationPos;
  e.cost = cost;
  return e;
}

HalfEdge Edge::makeInEdge() const
{
  HalfEdge e;
  e.id = internalId;
  e.end = sourcePos;
  e.cost = cost;
  return e;
}

float HalfEdge::costByConfiguration(const Config& conf) const { return cost * conf; }

void Edge::administerEdges(const std::vector<Edge>& edges)
{
  Edge::edges.reserve(lastId);
  for (const auto& edge : edges) {
    while (Edge::edges.size() <= edge.getId()) {
      Edge::edges.emplace_back(Edge{});
    }
    Edge::edges[edge.getId()] = edge;
  }
}
const Edge& Edge::getEdge(EdgeId id) { return edges.at(id); }

void Edge::setPosition(EdgeId id, NodePos source, NodePos dest)
{
  Edge& edge = Edge::edges[id];
  edge.setSourcePos(source);
  edge.setDestPos(dest);
}

float Cost::operator*(const Config& conf) const
{
  float combinedCost = this->length * conf.length + this->height * conf.height
      + this->unsuitability * conf.unsuitability;

  assert(combinedCost >= 0);
  return combinedCost + std::numeric_limits<float>::epsilon();
}
