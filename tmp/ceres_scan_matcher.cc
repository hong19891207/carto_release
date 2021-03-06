/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ceres_scan_matcher.h"

#include <utility>
#include <vector>

#include "eigen3/Eigen/Core"
#include "port.h"
#include "laser.h"
#include "rigid_transform.h"
#include "ceres/ceres.h"
//#include "glog/logging.h"

namespace carto_release {
namespace mapping_2d {

//common::CreateCeresSolverOptions(options.ceres_solver_options())
CeresScanMatcher::CeresScanMatcher()
    :  ceres_solver_options_() {
  ceres_solver_options_.linear_solver_type = ceres::DENSE_QR;
}

CeresScanMatcher::~CeresScanMatcher() {}

void CeresScanMatcher::Match(const transform::Rigid2f& previous_pose,
                             const transform::Rigid2f& initial_pose_estimate,
                             const PointCloud2D& point_cloud,
                             const ProbabilityGrid& probability_grid,
                             transform::Rigid2f* const pose_estimate,
                             ceres::Solver::Summary* const summary) const {
  double ceres_pose_estimate[3] = {initial_pose_estimate.translation().x(),
                                   initial_pose_estimate.translation().y(),
                                   initial_pose_estimate.rotation().angle()};
  ceres::Problem problem;
//  CHECK_GT(options_.occupied_space_cost_functor_weight(), 0.);
  problem.AddResidualBlock(
      new ceres::AutoDiffCostFunction<OccupiedSpaceCostFunctor, ceres::DYNAMIC,
                                      3>(
          new OccupiedSpaceCostFunctor(
              occupied_space_cost_functor_weight_ /
                  std::sqrt(static_cast<double>(point_cloud.size())),
              point_cloud, probability_grid),
          point_cloud.size()),
      nullptr, ceres_pose_estimate);
//  CHECK_GT(options_.previous_pose_translation_delta_cost_functor_weight(), 0.);
  problem.AddResidualBlock(
      new ceres::AutoDiffCostFunction<TranslationDeltaCostFunctor, 2, 3>(
          new TranslationDeltaCostFunctor(
              previous_pose_translation_delta_cost_functor_weight_ ,
              previous_pose)),
      nullptr, ceres_pose_estimate);
//  CHECK_GT(options_.initial_pose_estimate_rotation_delta_cost_functor_weight(), 0.);
  problem.AddResidualBlock(
      new ceres::AutoDiffCostFunction<RotationDeltaCostFunctor, 1,
                                      3>(new RotationDeltaCostFunctor(
          initial_pose_estimate_rotation_delta_cost_functor_weight_,
          ceres_pose_estimate[2])),
      nullptr, ceres_pose_estimate);

  ceres::Solve(ceres_solver_options_, &problem, summary);

  *pose_estimate = transform::Rigid2f(
      {ceres_pose_estimate[0], ceres_pose_estimate[1]}, ceres_pose_estimate[2]);
/*
//  ceres::Covariance::Options options;
//  ceres::Covariance covariance_computer(options);
//  std::vector<std::pair<const double*, const double*>> covariance_blocks;
//  covariance_blocks.emplace_back(ceres_pose_estimate, ceres_pose_estimate);
//  CHECK(covariance_computer.Compute(covariance_blocks, &problem));
//  double ceres_covariance[3 * 3];
//  covariance_computer.GetCovarianceBlock(ceres_pose_estimate,
//                                         ceres_pose_estimate, ceres_covariance);
//  *covariance = Eigen::Map<kalman_filter::Pose2DCovariance>(ceres_covariance);
//  *covariance *= options_.covariance_scale();
*/
}

}  // namespace mapping_2d
}  // namespace carto_release
