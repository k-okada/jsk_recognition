// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2015, JSK Lab
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/o2r other materials provided
 *     with the distribution.
 *   * Neither the name of the JSK Lab nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#define BOOST_PARAMETER_MAX_ARITY 7
#include "jsk_pcl_ros/interactive_cuboid_likelihood.h"
#include "jsk_pcl_ros/pcl_conversion_util.h"

namespace jsk_pcl_ros
{
  void InteractiveCuboidLikelihood::onInit()
  {
    DiagnosticNodelet::onInit();
    pub_ = pnh_->advertise<std_msgs::Float32>("output", 1);
    srv_ = boost::make_shared <dynamic_reconfigure::Server<Config> > (*pnh_);
    pnh_->param("frame_id", frame_id_, std::string("odom"));
    typename dynamic_reconfigure::Server<Config>::CallbackType f =
      boost::bind (&InteractiveCuboidLikelihood::configCallback, this, _1, _2);
    srv_->setCallback (f);
    sub_ = pnh_->subscribe("input", 1, &InteractiveCuboidLikelihood::likelihood, this);
    server_.reset(new interactive_markers::InteractiveMarkerServer(getName()));
    visualization_msgs::InteractiveMarker int_marker = particleToInteractiveMarker(particle_);
    server_->insert(int_marker, boost::bind(&InteractiveCuboidLikelihood::processFeedback, this, _1));
    server_->applyChanges();
  }

  void InteractiveCuboidLikelihood::subscribe()
  {
    
  }

  void InteractiveCuboidLikelihood::unsubscribe()
  {

  }

  void InteractiveCuboidLikelihood::processFeedback(
    const visualization_msgs::InteractiveMarkerFeedback::ConstPtr& feedback)
  {
    boost::mutex::scoped_lock lock(mutex_);
    Eigen::Affine3f pose;
    tf::poseMsgToEigen(feedback->pose, pose);
    particle_.fromEigen(pose);
  }

  void InteractiveCuboidLikelihood::likelihood(
    const sensor_msgs::PointCloud2::ConstPtr& msg)
  {
    boost::mutex::scoped_lock lock(mutex_);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromROSMsg(*msg, *cloud);
    double l = computeLikelihood(particle_, cloud, config_);
    JSK_ROS_INFO("likelihood: %f", l);
    std_msgs::Float32 float_msg;
    float_msg.data = l;
    pub_.publish(float_msg);
  }

  void InteractiveCuboidLikelihood::configCallback(
    Config& config, uint32_t level)
  {
    
    boost::mutex::scoped_lock lock(mutex_);
    config_ = config;
    particle_.dx = config_.dx;
    particle_.dy = config_.dy;
    particle_.dz = config_.dz;
    if (server_) {
      visualization_msgs::InteractiveMarker int_marker = particleToInteractiveMarker(particle_);
      server_->insert(int_marker, boost::bind(&InteractiveCuboidLikelihood::processFeedback, this, _1));
      server_->applyChanges();
    }
  }

  visualization_msgs::Marker
  InteractiveCuboidLikelihood::particleToMarker(
    const Particle& p)
  {
    visualization_msgs::Marker marker;
    marker.type = visualization_msgs::Marker::CUBE;
    marker.scale.x = p.dx;
    marker.scale.y = p.dy;
    marker.scale.z = p.dz;
    marker.pose.orientation.w = 1.0;
    marker.color.r = 0.5;
    marker.color.g = 0.5;
    marker.color.b = 0.5;
    marker.color.a = 1.0;
    return marker;
  }
  
  visualization_msgs::Marker
  InteractiveCuboidLikelihood::particleToSupportPlaneMarker(
    const Particle& p)
  {
    visualization_msgs::Marker marker;
    marker.type = visualization_msgs::Marker::CUBE;
    marker.scale.x = 0.3;
    marker.scale.y = 0.3;
    marker.scale.z = 0.01;
    marker.pose.position.z = - p.dz / 2.0;
    marker.color.r = 0.8;
    marker.color.g = 1.0;
    marker.color.b = 0.2;
    marker.color.a = 0.5;
    return marker;
  }

  visualization_msgs::InteractiveMarker InteractiveCuboidLikelihood::particleToInteractiveMarker(const Particle& p)
  {
    visualization_msgs::InteractiveMarker int_marker;
    int_marker.header.frame_id = frame_id_;
    int_marker.header.stamp = ros::Time::now();
    int_marker.name = getName();
    Eigen::Affine3f pose = particle_.toEigenMatrix();
    tf::poseEigenToMsg(pose, int_marker.pose);

    visualization_msgs::InteractiveMarkerControl control;
    control.orientation.w = 1;
    control.orientation.x = 1;
    control.orientation.y = 0;
    control.orientation.z = 0;
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::ROTATE_AXIS;
    int_marker.controls.push_back(control);
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::MOVE_AXIS;
    int_marker.controls.push_back(control);

    control.orientation.w = 1;
    control.orientation.x = 0;
    control.orientation.y = 1;
    control.orientation.z = 0;
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::ROTATE_AXIS;
    int_marker.controls.push_back(control);
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::MOVE_AXIS;
    int_marker.controls.push_back(control);

    control.orientation.w = 1;
    control.orientation.x = 0;
    control.orientation.y = 0;
    control.orientation.z = 1;
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::ROTATE_AXIS;
    int_marker.controls.push_back(control);
    control.interaction_mode = visualization_msgs::InteractiveMarkerControl::MOVE_AXIS;
    int_marker.controls.push_back(control);
    visualization_msgs::InteractiveMarkerControl box_control;
    box_control.always_visible = true;
    box_control.markers.push_back(particleToMarker(particle_));
    box_control.markers.push_back(particleToSupportPlaneMarker(particle_));
    int_marker.controls.push_back(box_control);
    
    return int_marker;
  }
  
}

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS (jsk_pcl_ros::InteractiveCuboidLikelihood, nodelet::Nodelet);