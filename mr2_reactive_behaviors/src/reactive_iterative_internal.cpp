// Copyright 2021 Intelligent Robotics Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memory>
#include <utility>

#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

#include "rclcpp/rclcpp.hpp"

using std::placeholders::_1;
using namespace std::chrono_literals;

class Avoidance : public rclcpp::Node
{
public:
  Avoidance()
  : Node("avoidance")
  {
    vel_pub_ = create_publisher<geometry_msgs::msg::Twist>("output_vel", 100);
    scan_sub_ = create_subscription<sensor_msgs::msg::LaserScan>(
      "input_scan", 100, std::bind(&Avoidance::scan_callback, this, _1));
    timer_ = create_wall_timer(
      50ms, std::bind(&Avoidance::control_cycle, this));
  }

  void scan_callback(sensor_msgs::msg::LaserScan::UniquePtr msg)
  {
    last_scan_ = std::move(msg);
  }

  void control_cycle()
  {
    if (last_scan_ == nullptr) {
      return;
    }

    const auto & ranges = last_scan_->ranges;
    const float & read_front = ranges[0];
    const float & read_left = ranges[(ranges.size() * 5) / 6];
    const float & read_right = ranges[ranges.size() / 6];

    const float OBSTACLE_DISTANCE = 0.5;
    bool obstacle_front = read_front < OBSTACLE_DISTANCE;
    bool obstacle_left = read_left < OBSTACLE_DISTANCE;
    bool obstacle_right = read_right < OBSTACLE_DISTANCE;

    RCLCPP_INFO_STREAM(
      get_logger(),
      read_left << " " << read_front << " " << read_right);

    const float LINEAR_SPEED = 0.2;
    const float ANGULAR_SPEED = 0.5;

    geometry_msgs::msg::Twist vel;

    if (!obstacle_front) {
      vel.linear.x = LINEAR_SPEED;
    }

    if (obstacle_left) {
      vel.angular.z = vel.angular.z + ANGULAR_SPEED;
    }
    if (obstacle_right) {
      vel.angular.z = vel.angular.z - ANGULAR_SPEED;
    }

    vel_pub_->publish(vel);
  }

private:
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr vel_pub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
  rclcpp::TimerBase::SharedPtr timer_;

  sensor_msgs::msg::LaserScan::UniquePtr last_scan_;
};


int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node_bump_stop = std::make_shared<Avoidance>();
  rclcpp::spin(node_bump_stop->get_node_base_interface());

  rclcpp::shutdown();

  return 0;
}
