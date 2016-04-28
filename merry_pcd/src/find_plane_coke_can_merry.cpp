//find_plane_pcd_file.cpp
// prompts for a pcd file name, reads the file, and displays to rviz on topic "pcd"
// can select a patch; then computes a plane containing that patch, which is published on topic "planar_pts"
// illustrates use of PCL methods: computePointNormal(), transformPointCloud(), 
// pcl::PassThrough methods setInputCloud(), setFilterFieldName(), setFilterLimits, filter()
// pcl::io::loadPCDFile() 
// pcl::toROSMsg() for converting PCL pointcloud to ROS message
// voxel-grid filtering: pcl::VoxelGrid,  setInputCloud(), setLeafSize(), filter()
//wsn March 2016

#include <ros/ros.h> 
#include <stdlib.h>
#include <math.h>

#include <sensor_msgs/PointCloud2.h> 
#include <pcl_ros/point_cloud.h> //to convert between PCL and ROS
#include <pcl/ros/conversions.h>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
//#include <pcl/PCLPointCloud2.h> //PCL is migrating to PointCloud2 

#include <pcl/common/common_headers.h>
#include <pcl-1.7/pcl/point_cloud.h>
#include <pcl-1.7/pcl/PCLHeader.h>

//will use filter objects "passthrough" and "voxel_grid" in this example
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h> 

#include <pcl_utils/pcl_utils.h>  //a local library with some utility fncs


#include <iostream> 

using namespace std;
extern PclUtils *g_pcl_utils_ptr; 

//this fnc is defined in a separate module, find_indices_of_plane_from_patch.cpp
extern void find_indices_of_plane_from_patch(pcl::PointCloud<pcl::PointXYZRGB>::Ptr input_cloud_ptr,
        pcl::PointCloud<pcl::PointXYZ>::Ptr patch_cloud_ptr, vector<int> &indices);


int main(int argc, char** argv) {
    ros::init(argc, argv, "coke_can_finder"); //node name
    ros::NodeHandle nh;

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr pclKinect_clr_ptr(new pcl::PointCloud<pcl::PointXYZRGB>); //pointer for color version of pointcloud
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr plane_pts_ptr(new pcl::PointCloud<pcl::PointXYZRGB>); //pointer for pointcloud of planar points found
    pcl::PointCloud<pcl::PointXYZ>::Ptr selected_pts_cloud_ptr(new pcl::PointCloud<pcl::PointXYZ>); //ptr to selected pts from Rvis tool
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr downsampled_kinect_ptr(new pcl::PointCloud<pcl::PointXYZRGB>); //ptr to hold filtered Kinect image

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr rough_table_cloud_ptr(new pcl::PointCloud<pcl::PointXYZRGB>); //ptr to table pts from Rvis tool    
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr can_pts_cloud_ptr(new pcl::PointCloud<pcl::PointXYZRGB>);

    pcl::PointCloud<pcl::PointXYZ>::Ptr table_xyz_ptr(new pcl::PointCloud<pcl::PointXYZ>);

    vector<int> indices;

    PclUtils pclUtils(&nh); //instantiate a PclUtils object--a local library w/ some handy fncs
    g_pcl_utils_ptr = &pclUtils; // make this object shared globally, so above fnc can use it too

    ros::Publisher pubCloud = nh.advertise<sensor_msgs::PointCloud2> ("/pcd", 1);
    ros::Publisher pubPlane = nh.advertise<sensor_msgs::PointCloud2> ("planar_pts", 1);
    ros::Publisher pubDnSamp = nh.advertise<sensor_msgs::PointCloud2> ("downsampled_pcd", 1);

    sensor_msgs::PointCloud2 tablePts; //create a ROS message
    ros::Publisher table = nh.advertise<sensor_msgs::PointCloud2> ("/table_pts", 1);

    sensor_msgs::PointCloud2 canPts;
    ros::Publisher Can = nh.advertise<sensor_msgs::PointCloud2> ("/coke_can_pts", 1);    

    sensor_msgs::PointCloud2 ros_cloud, table_planar_cloud, ros_planar_cloud, downsampled_cloud; //here are ROS-compatible messages    
while (ros::ok()) {

    if (pclUtils.got_kinect_cloud_)
    {
        pclUtils.get_kinect_points(pclKinect_clr_ptr);
    //will publish  pointClouds as ROS-compatible messages; create publishers; note topics for rviz viewing        
        pcl::toROSMsg(*pclKinect_clr_ptr, ros_cloud); //convert from PCL cloud to ROS message this way

    //use voxel filtering to downsample the original cloud:
        cout << "starting voxel filtering" << endl;
        pcl::VoxelGrid<pcl::PointXYZRGB> vox;
        vox.setInputCloud(pclKinect_clr_ptr);

        vox.setLeafSize(0.02f, 0.02f, 0.02f);
        vox.filter(*downsampled_kinect_ptr);
        cout << "done voxel filtering" << endl;//camera/depth_registered/points;

        cout << "num bytes in original cloud data = " << pclKinect_clr_ptr->points.size() << endl;
        cout << "num bytes in filtered cloud data = " << downsampled_kinect_ptr->points.size() << endl; // ->data.size()<<endl;    
        pcl::toROSMsg(*downsampled_kinect_ptr, downsampled_cloud); //convert to ros message for publication and display

        //cout << " select a patch of points to find corresponding plane..." << endl; //prompt user action

    //******************************************************************//        
        pclUtils.seek_rough_table_merry(pclKinect_clr_ptr, 0, rough_table_cloud_ptr);

        pclUtils.from_RGB_to_XYZ(rough_table_cloud_ptr, table_xyz_ptr);
        find_indices_of_plane_from_patch(downsampled_kinect_ptr, table_xyz_ptr, indices);    
        pcl::copyPointCloud(*downsampled_kinect_ptr, indices, *plane_pts_ptr); //extract these pts into new cloud

        pcl::toROSMsg(*plane_pts_ptr, table_planar_cloud); //rough table cloud

        pcl::PointCloud<pcl::PointXYZRGB>::Ptr final_table_cloud_ptr(new pcl::PointCloud<pcl::PointXYZRGB>); //ptr to table pts from Rvis tool 
        pclUtils.find_final_table_merry(plane_pts_ptr, final_table_cloud_ptr);

        pcl::toROSMsg(*final_table_cloud_ptr, tablePts); //final table cloud
        //the new cloud is a set of points from original cloud, coplanar with selected patch; display the result


        pclUtils.seek_coke_can_cloud(pclKinect_clr_ptr, can_pts_cloud_ptr);

        bool can_exist = pclUtils.is_coke_can(can_pts_cloud_ptr);
        Eigen::Vector3f can_bottom;

        if (can_exist)
        {
            cout << "coke can detected !" << endl;
            can_bottom = pclUtils.find_can_bottom(final_table_cloud_ptr); //find bottom in table
            pcl::toROSMsg(*can_pts_cloud_ptr, canPts);
            ROS_INFO("The x y z of the can bottom is: (%f, %f, %f)", can_bottom[0], can_bottom[1], can_bottom[2]);
        }
        else{
            cout << "NO coke can!" << endl;
            
        }
        pclUtils.got_kinect_cloud_ = false;
    }
    else{
          ROS_INFO("NO Kinect callback !");
    }

        // if (pclUtils.got_selected_points()) { //here if user selected a new patch of points
        //     pclUtils.reset_got_selected_points(); // reset for a future trigger
        //     pclUtils.get_copy_selected_points(selected_pts_cloud_ptr); //get a copy of the selected points
        //     cout << "got new patch with number of selected pts = " << selected_pts_cloud_ptr->points.size() << endl;

        //     //find pts coplanar w/ selected patch, using PCL methods in above-defined function
        //     //"indices" will get filled with indices of points that are approx co-planar with the selected patch
        //     // can extract indices from original cloud, or from voxel-filtered (down-sampled) cloud
        //     //find_indices_of_plane_from_patch(pclKinect_clr_ptr, selected_pts_cloud_ptr, indices);
        //     find_indices_of_plane_from_patch(downsampled_kinect_ptr, selected_pts_cloud_ptr, indices);
        //     pcl::copyPointCloud(*downsampled_kinect_ptr, indices, *plane_pts_ptr); //extract these pts into new cloud
        //     //the new cloud is a set of points from original cloud, coplanar with selected patch; display the result
        //     pcl::toROSMsg(*plane_pts_ptr, ros_planar_cloud); //convert to ros message for publication and display
        // }   

        // pubPlane.publish(ros_planar_cloud); //select points

        pubPlane.publish(table_planar_cloud); // should be the rough table, to debug
        table.publish(tablePts);  //final table
        Can.publish(canPts);

        pubCloud.publish(ros_cloud); // will not need to keep republishing if display setting is persistent

        pubDnSamp.publish(downsampled_cloud); //can directly publish a pcl::PointCloud2!!
        
        ros::spinOnce(); //pclUtils needs some spin cycles to invoke callbacks for new selected points
        ros::Duration(4).sleep();
    }

    return 0;
}
