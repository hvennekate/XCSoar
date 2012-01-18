/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef NAV_DIJKSTRA_HPP
#define NAV_DIJKSTRA_HPP

#include "Util/NonCopyable.hpp"
#include "Dijkstra.hpp"
#include "ScanTaskPoint.hpp"
#include "Compiler.h"

#include <unordered_map>
#include <assert.h>

/**
 * Abstract class for A* /Dijkstra searches of nav points, managing
 * edges in multiple stages (corresponding to turn points).
 *
 * Expected running time, see http://www.avglab.com/andrew/pub/neci-tr-96-062.ps
 *
 * NavDijkstra<SearchPoint>
 */
class NavDijkstra: 
  private NonCopyable 
{
protected:
  enum {
    MAX_STAGES = 16,
  };

  struct DijkstraMap {
    struct Hash {
      std::size_t operator()(ScanTaskPoint p) const {
        return p.Key();
      }
    };

    struct Equal {
      std::size_t operator()(ScanTaskPoint a, ScanTaskPoint b) const {
        return a.Key() == b.Key();
      }
    };

    template<typename Value>
    struct Bind : public std::unordered_map<ScanTaskPoint, Value,
                                            Hash, Equal> {
    };
  };

  Dijkstra<ScanTaskPoint, DijkstraMap> dijkstra;

  /** Number of stages in search */
  unsigned num_stages;

  /**
   * An array containing the point index for each of the solution's stages.
   */
  unsigned solution[MAX_STAGES];

  bool solution_valid;

protected:
  /** 
   * Constructor
   * 
   * @param _num_stages Number of stages in search
   * 
   * @return Initialised object
   */
  NavDijkstra(const bool is_min, const unsigned _num_stages)
    :dijkstra(is_min), solution_valid(false)
  {
    set_stages(_num_stages);
  }

protected:
  /** 
   * Set the number of stages to search for, and clear the solution
   * array
   */
  void set_stages(const unsigned _num_stages) {
    assert(_num_stages <= MAX_STAGES);
    num_stages =_num_stages;
  }

  /** 
   * Determine whether a finished path is valid
   * 
   * @param sp Point to check
   * 
   * @return True if this terminal point completes a valid solution
   */
  virtual bool finish_satisfied(const ScanTaskPoint sp) const {
    return true;
  }

  /** 
   * Add edges from an origin node
   * 
   * @param curNode Origin node to add edges from
   */
  virtual void add_edges(const ScanTaskPoint curNode) = 0;

  /** 
   * Determine whether a point is terminal (no further edges)
   * 
   * @param sp Point to test
   * 
   * @return True if point is terminal
   */
  gcc_pure
  bool is_final(const ScanTaskPoint sp) const {
    assert(num_stages <= MAX_STAGES);
    return (unsigned)(sp.GetStageNumber() + 1) == num_stages;
  }

  /** 
   * Iterate search algorithm
   * 
   * @param dijkstra Dijkstra structure to iterate
   * @param max_steps Maximum number of steps to update
   * 
   * @return True if algorithm returns a terminal path or no path found
   */
  bool distance_general(unsigned max_steps = 0 - 1) {
    while (!dijkstra.empty()) {
      const ScanTaskPoint destination = dijkstra.pop();

      if (is_final(destination)) {
        find_solution(destination);
        if (finish_satisfied(destination)) {
          solution_valid = true;
          return true;
        }
      } else {
        add_edges(destination);
        if (dijkstra.empty())
          return true; // error, no way to reach final
      }

      if (max_steps)
        --max_steps;
      else
        return false; // Reached limit
    }

    return false; // No path found
  }

  /**
   * Search the chain for the ScanTaskPoint at the specified stage.
   */
  gcc_pure
  ScanTaskPoint FindStage(ScanTaskPoint p, unsigned stage_number) const {
    assert(stage_number <= p.GetStageNumber());

    while (p.GetStageNumber() > stage_number) {
#ifndef NDEBUG
      const ScanTaskPoint o(p);
#endif
      p = dijkstra.get_predecessor(p);
      assert(p.GetStageNumber() + 1 == o.GetStageNumber());
    }

    return p;
  }

  /**
   * Find the first ScanTaskPoint in the chain.
   */
  gcc_pure
  ScanTaskPoint FindStart(ScanTaskPoint p) const {
    return FindStage(p, 0);
  }

  /** 
   * Determine optimal solution by backtracing the Dijkstra tree
   * 
   * @param destination Terminal point to query
   */
  void find_solution(const ScanTaskPoint destination) {
    ScanTaskPoint p(destination); 
    unsigned last_stage_number;

    do {
      last_stage_number = p.GetStageNumber();
      solution[p.GetStageNumber()] = p.GetPointIndex();
      p = dijkstra.get_predecessor(p);
    } while (p.GetStageNumber() != last_stage_number);
  }
};

#endif
