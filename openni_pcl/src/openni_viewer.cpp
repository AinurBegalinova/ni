/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
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
 *
 * $Id: pointcloud_online_viewer.cpp 33238 2010-03-11 00:46:58Z rusu $
 *
 */

// ROS core
#include <signal.h>
#include <ros/ros.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
// PCL includes
#include <pcl/point_types.h>
#include <pcl_visualization/pcl_visualizer.h>

// Global data
sensor_msgs::PointCloud2ConstPtr cloud_, cloud_old_;
boost::mutex m;

void
cloud_cb (const sensor_msgs::PointCloud2ConstPtr& cloud)
{
  m.lock ();
  printf ("\rPointCloud with %d data points (%s), stamp %f, and frame %s.",
      cloud->width * cloud->height, pcl::getFieldsList (*cloud).c_str (), cloud->header.stamp.toSec (), cloud->header.frame_id.c_str ());
  cloud_ = cloud;
  m.unlock ();
}

void
updateVisualization ()
{
  pcl::PointCloud<pcl::PointXYZ> cloud_xyz;
  pcl::PointCloud<pcl::PointXYZRGB> cloud_xyz_rgb;

  ros::Duration d (0.01);
  bool rgb = false;
  std::vector<sensor_msgs::PointField> fields;
  
  // Create the visualizer
  pcl_visualization::PCLVisualizer p ("OpenNI Kinect Viewer");

  // Add a coordinate system to screen
  p.addCoordinateSystem (0.1);

  while (true)
  {
    d.sleep ();
    // If no cloud received yet, continue
    if (!cloud_)
      continue;

    p.spinOnce (1);

    if (cloud_old_ == cloud_)
      continue;
    
    m.lock ();
    
    // Convert to PointCloud<T>
    if (pcl::getFieldIndex (*cloud_, "rgb") != -1)
    {
      rgb = true;
      pcl::fromROSMsg (*cloud_, cloud_xyz_rgb);
    }
    else
    {
      rgb = false;
      pcl::fromROSMsg (*cloud_, cloud_xyz);
      pcl::getFields (cloud_xyz, fields);
    }
    cloud_old_ = cloud_;
    m.unlock ();

    // Save the last point size used
    //p.getPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_POINT_SIZE, psize, "cloud");

    p.removePointCloud ("cloud");
    
    // If no RGB data present, use a simpler white handler
    if (rgb && pcl::getFieldIndex (cloud_xyz_rgb, "rgb", fields) != -1)
    {
      pcl_visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> color_handler (cloud_xyz_rgb);
      p.addPointCloud (cloud_xyz_rgb, color_handler, "cloud");
    }
    else
    {
      pcl_visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> color_handler (cloud_xyz, 255, 0, 255);
      p.addPointCloud (cloud_xyz, color_handler, "cloud");
    }

    // Set the point size
    //p.setPointCloudRenderingProperties (pcl_visualization::PCL_VISUALIZER_POINT_SIZE, psize, "cloud");
  }
}

void
  sigIntHandler (int sig)
{
  exit (0);
}

/* ---[ */
int
main (int argc, char** argv)
{
  ros::init (argc, argv, "openni_viewer", ros::init_options::NoSigintHandler);
  ros::NodeHandle nh ("~");

  // Create a ROS subscriber
  ros::Subscriber sub = nh.subscribe ("input", 30, cloud_cb);

  ROS_INFO ("Subscribing to %s for PointCloud2 messages...", nh.resolveName ("input").c_str ());

  signal (SIGINT, sigIntHandler);

  boost::thread visualization_thread (&updateVisualization);

  // Spin
  ros::spin ();

  // Join, delete, exit
  visualization_thread.join ();
  return (0);
}
/* ]--- */
