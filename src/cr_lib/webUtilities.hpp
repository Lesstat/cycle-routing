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
#ifndef WEBUTILITIES_H
#define WEBUTILITIES_H

std::string routeToJson(const Route& route, const Graph& g)
{
  std::stringstream resultJson;
  resultJson << "{ \"length\": " << route.costs.length << ", \"height\": " << route.costs.height
             << ", \"unsuitability\": " << route.costs.unsuitability
             << R"(, "route": { "type": "Feature", "geometry": { "type": "LineString", )"
             << "\"coordinates\":[";

  std::unordered_set<NodeId> nodes;
  nodes.reserve(route.edges.size());
  for (const auto& edge : route.edges) {
    nodes.insert(edge.getSourceId());
    nodes.insert(edge.getDestId());
  }

  for (const auto& edge : route.edges) {
    const auto& node = g.getNode(edge.sourcePos());
    resultJson << '[' << node.lng() << ", " << node.lat() << "],";
  }
  if (!route.edges.empty()) {
    const auto& lastEdge = route.edges[route.edges.size() - 1];
    const auto& endNode = g.getNode(lastEdge.destPos());
    resultJson << '[' << endNode.lng() << ", " << endNode.lat() << "]] } } }";
  }
  return resultJson.str();
}
void extractQueryFields(const SimpleWeb::CaseInsensitiveMultimap& queryFields,
    std::optional<size_t>& s, std::optional<size_t>& t, std::optional<size_t>& length,
    std::optional<size_t>& height, std::optional<size_t>& unsuitability)
{
  for (const auto& field : queryFields) {
    if (field.first == "s") {
      s = static_cast<size_t>(stoull(field.second));
    } else if (field.first == "t") {
      t = static_cast<size_t>(stoull(field.second));
    } else if (field.first == "length") {
      length = static_cast<size_t>(stoull(field.second));
    } else if (field.first == "height") {
      height = static_cast<size_t>(stoull(field.second));
    } else if (field.first == "unsuitability") {
      unsuitability = static_cast<size_t>(stoull(field.second));
    }
  }
}

void appendAlternativesToJsonStream(
    std::stringstream& result, const AlternativeRoutes& routes, const Graph& g)
{

  result << R"({ "config1": ")" << std::round(routes.config1.length * 100) << "/"
         << std::round(routes.config1.height * 100) << "/"
         << std::round(routes.config1.unsuitability * 100) << R"(", )";
  result << R"( "route1":  )" << routeToJson(routes.route1, g) << ", ";

  result << R"( "config2": ")" << std::round(routes.config2.length * 100) << "/"
         << std::round(routes.config2.height * 100) << "/"
         << std::round(routes.config2.unsuitability * 100) << R"(", )";
  result << R"( "route2":  )" << routeToJson(routes.route2, g) << ", ";
  result << R"( "shared":  )" << std::round(routes.shared * 100) << ", "
         << R"( "frechet":  )" << routes.frechet << "}";
}

void appendCsvLine(std::stringstream& result, const std::string& method, NodePos from, NodePos to,
    const AlternativeRoutes& altRoutes, long duration, const Length& shortestLength)
{
  result << from << ", " << to << ", " << method << ", " << altRoutes.shared << ", "
         << altRoutes.frechet << ", " << duration << ", " << altRoutes.config1.length << "/"
         << altRoutes.config1.height << "/" << altRoutes.config1.unsuitability << ", "
         << altRoutes.route1.costs.length << ", " << altRoutes.config2.length << "/"
         << altRoutes.config2.height << "/" << altRoutes.config2.unsuitability << ", "
         << altRoutes.route2.costs.length << ", " << shortestLength << '\n';
}

#endif /* WEBUTILITIES_H */