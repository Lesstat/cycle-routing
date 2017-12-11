#ifndef CONTRACTOR_H
#define CONTRACTOR_H
#include "dijkstra.hpp"
#include "graph.hpp"

class Contractor {

  public:
  //! Default constructor
  Contractor() = default;

  //! Copy constructor
  Contractor(const Contractor& other) = default;

  //! Move constructor
  Contractor(Contractor&& other) noexcept = default;

  //! Destructor
  virtual ~Contractor() noexcept = default;

  //! Copy assignment operator
  Contractor& operator=(const Contractor& other) = default;

  //! Move assignment operator
  Contractor& operator=(Contractor&& other) noexcept = default;

  Edge createShortcut(const Edge& e1, const Edge& e2);

  bool isShortestPath(const Graph& g, const EdgeId& startEdgeId, const EdgeId& destEdgeId, const Config& conf);

  protected:
  private:
};

#endif /* CONTRACTOR_H */