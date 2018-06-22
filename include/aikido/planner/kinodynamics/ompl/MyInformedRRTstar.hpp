#ifndef MY_INFORMED_RRT_STAR_H_
#define MY_INFORMED_RRT_STAR_H_

// OMPL
#include <fstream>
#include <ios>
#include <iostream>
#include <ompl/datastructures/NearestNeighborsGNAT.h>
#include <ompl/datastructures/NearestNeighborsLinear.h>
#include <ompl/geometric/planners/rrt/InformedRRTstar.h>

#include "aikido/planner/ompl/BackwardCompatibility.hpp"

namespace ompl {
namespace geometric {
class MyInformedRRTstar : public ompl::geometric::InformedRRTstar
{
public:
  typedef enum { LOAD_SAMPLES, SAVE_SAMPLES, RANDOM_SAMPLES } PlannerMode;
  MyInformedRRTstar(const ompl::base::SpaceInformationPtr& si);

  virtual ompl::base::PlannerStatus solve(
      const ompl::base::PlannerTerminationCondition& ptc) override;

  void initLogFile(std::string scenarioName, std::string samplerName, int id);

  ompl::base::PlannerStatus solve(double solveTime);

  ompl::base::PlannerStatus solveAfterLoadingSamples(
      std::string filename, double solveTime);

  ompl::base::PlannerStatus solveAndSaveSamples(
      std::string filename, double solveTime);

  std::string fromState(ompl::base::State* fromState);
  bool toState(std::string stateString, ompl::base::State* toState);

  uint64_t getSamplesGeneratedNum()
  {
    return samplesGeneratedNum_;
  }

  ::ompl::base::PathPtr completeApproximateSolution(::ompl::base::PathPtr approximation);

private:
  PlannerMode mode_;
  std::ofstream out_;
  std::ofstream sampleSaveStream_;
  std::ifstream sampleLoadStream_;

  uint64_t samplesGeneratedNum_;

  std::list<std::string> loadedSamplesStr_;
};

using MyInformedRRTstarPtr
    = aikido::planner::ompl::ompl_shared_ptr<MyInformedRRTstar>;
}
}

#endif // MY_INFORMED_RRT_STAR_H_