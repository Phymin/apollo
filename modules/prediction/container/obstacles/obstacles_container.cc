/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/prediction/container/obstacles/obstacles_container.h"
#include "modules/common/math/math_utils.h"

namespace apollo {
namespace prediction {

using apollo::perception::PerceptionObstacle;
using apollo::perception::PerceptionObstacles;

std::mutex ObstaclesContainer::g_mutex_;

ObstaclesContainer::ObstaclesContainer() {
  timestamp_ = 0.0;
}

void ObstaclesContainer::Insert(const ::google::protobuf::Message& message) {
  const PerceptionObstacles& perception_obstacles =
      dynamic_cast<const PerceptionObstacles&>(message);
  double timestamp = 0.0;
  if (perception_obstacles.has_header() &&
      perception_obstacles.header().has_timestamp_sec()) {
    timestamp = perception_obstacles.header().timestamp_sec();
  }
  if (apollo::common::math::DoubleCompare(timestamp, timestamp_) < 0) {
    AERROR << "Invalid timestamp curr [" << timestamp << "] v.s. prev ["
           << timestamp_ << "].";
  } else {
  	timestamp_ = timestamp;
  }
  for (const PerceptionObstacle& perception_obstacle :
      perception_obstacles.perception_obstacle()) {
    InsertPerceptionObstacle(perception_obstacle, timestamp_);
  }

  SetObstacleLaneGraphFeatures(perception_obstacles);
}

void ObstaclesContainer::InsertPose(PoseContainer* pose_container) {
  if (pose_container == nullptr) {
  	AERROR << "Unable to find pose.";
  	return;
  }
  InsertPerceptionObstacle(
      *(pose_container->ToPerceptionObstacle()), timestamp_);
}

Obstacle* ObstaclesContainer::GetObstacle(int id) {
  return nullptr;
}

void ObstaclesContainer::InsertPerceptionObstacle(
    const PerceptionObstacle& perception_obstacle, const double timestamp) {
  std::lock_guard<std::mutex> lock(g_mutex_);
  int id = perception_obstacle.id();
  if (id < -1) {
    AERROR << "Invalid ID [" << id << "]";
    return;
  }
  if (!IsPredictable(perception_obstacle)) {
    ADEBUG << "Obstacle [" << id << "] is not predictable.";
    return;
  }
  Obstacle* obstacle_ptr = obstacles_.GetSilently(id);
  if (obstacle_ptr != nullptr) {
    obstacle_ptr->Insert(perception_obstacle, timestamp);
  } else {
    Obstacle obstacle;
    obstacle.Insert(perception_obstacle, timestamp);
    obstacles_.Put(id, std::move(obstacle));
  }
  ADEBUG << "Obstacle [" << id << "] was inserted.";
}

bool ObstaclesContainer::IsPredictable(
    const PerceptionObstacle& perception_obstacle) {
  if (!perception_obstacle.has_type() ||
      perception_obstacle.type() == PerceptionObstacle::UNKNOWN_MOVABLE) {
    return false;
  }
  return true;
}

void ObstaclesContainer::SetObstacleLaneGraphFeatures(
    const PerceptionObstacles& perception_obstacles) {
  // TODO(kechxu) Implement
}

}  // namespace prediction
}  // namespace apollo
