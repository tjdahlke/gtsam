/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    testDCFactorGraph.cpp
 * @brief   Unit tests for DCFactorGraph
 * @author  Varun Agrawal
 * @author  Fan Jiang
 * @author  Frank Dellaert
 * @date    December 2021
 */

#include <gtsam/hybrid/DCFactorGraph.h>
#include <gtsam/hybrid/DCGaussianMixtureFactor.h>
#include <gtsam/hybrid/DCMixtureFactor.h>
#include <gtsam/nonlinear/PriorFactor.h>
#include <gtsam/slam/BetweenFactor.h>

// Include for test suite
#include <CppUnitLite/TestHarness.h>

using namespace std;
using namespace gtsam;

using noiseModel::Isotropic;
using symbol_shorthand::M;
using symbol_shorthand::X;

/* ****************************************************************************
 * Test construction with small switching-like hybrid factor graph.
 */
TEST(DiscreteBayesTree, Switching) {
  // Number of time steps.
  const size_t K = 5;

  // Create DiscreteKeys for binary K+1 modes.
  DiscreteKeys modes;
  for (size_t k = 0; k <= K; k++) {
    using symbol_shorthand::M;
    modes.emplace_back(M(k), 2);
  }

  // Create hybrid factor graph.
  DCFactorGraph fg;

  // Add a prior on X(1).
  using PriorMixture = DCMixtureFactor<PriorFactor<double>>;
  auto prior = boost::make_shared<PriorFactor<double>>(
      X(1), 0, Isotropic::Sigma(1, 0.1));
  PriorMixture priorMixture({X(1)}, DiscreteKeys{modes[0]}, {prior, prior});
  fg.add(priorMixture);

  // Add "motion models".
  for (size_t k = 1; k < K; k++) {
    auto still = boost::make_shared<BetweenFactor<double>>(
        X(k), X(k + 1), 0.0, Isotropic::Sigma(2, 1.0));
    auto moving = boost::make_shared<BetweenFactor<double>>(
        X(k), X(k + 1), 1.0, Isotropic::Sigma(2, 1.0));
    using MotionMixture = DCMixtureFactor<BetweenFactor<double>>;
    MotionMixture mixture({X(k), X(k + 1)}, DiscreteKeys{modes[k]},
                          {still, moving});
    fg.add(mixture);
  }

  EXPECT_LONGS_EQUAL(5, fg.size());
}

/* ************************************************************************* */
/// Test that discrete keys are not duplicated
TEST(DCFactorGraph, DiscreteKeys) {
  DCFactorGraph fg;

  for (size_t k = 0; k < 2; k++) {
    auto still = boost::make_shared<BetweenFactor<double>>(
        X(k), X(k + 1), 0.0, Isotropic::Sigma(2, 1.0));
    auto moving = boost::make_shared<BetweenFactor<double>>(
        X(k), X(k + 1), 1.0, Isotropic::Sigma(2, 1.0));
    using MotionMixture = DCMixtureFactor<BetweenFactor<double>>;

    // Add a mixture which has a mode on each continious variable.
    MotionMixture mixture(
        {X(k), X(k + 1)},
        DiscreteKeys{DiscreteKey(M(k), 2), DiscreteKey(M(k + 1), 2)},
        {still, moving, moving, moving});
    fg.add(mixture);
  }

  EXPECT_LONGS_EQUAL(3, fg.discreteKeys().size());
  EXPECT(fg.discreteKeys() ==
         DiscreteKeys({DiscreteKey(M(0), 2), DiscreteKey(M(1), 2),
                       DiscreteKey(M(2), 2)}));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
