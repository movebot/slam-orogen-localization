/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "PointcloudInMLS.hpp"

using namespace localization;



PointcloudInMLS::PointcloudInMLS(std::string const& name)
    : PointcloudInMLSBase(name)
{
}

PointcloudInMLS::PointcloudInMLS(std::string const& name, RTT::ExecutionEngine* engine)
    : PointcloudInMLSBase(name, engine)
{
}

PointcloudInMLS::~PointcloudInMLS()
{
}

void PointcloudInMLS::pointcloud_samplesTransformerCallback(const base::Time &ts, const ::base::samples::Pointcloud &pointcloud_samples_sample)
{
    Eigen::Affine3d pointcloud2body;
    if (!_pointcloud2body.get(ts, pointcloud2body))
    {
        RTT::log(RTT::Error) << "skip, have no pointcloud2body transformation sample!" << RTT::endlog();
        new_state = TaskBase::MISSING_TRANSFORMATION;
        return;
    }
    envire::TransformWithUncertainty body2odometry;
    if (!_body2odometry.get(ts, body2odometry))
    {
        RTT::log(RTT::Error) << "skip, have no body2odometry transformation sample!" << RTT::endlog();
        new_state = TaskBase::MISSING_TRANSFORMATION;
        return;
    }

    if(init_odometry)
    {
        init_odometry = false;
        last_odometry2body = body2odometry.inverse();
        return;
    }
    
    if(newICPRunPossible(body2odometry.getTransform()))
    {
        //save pointcloud for processing later on
        pc = PointcloudWithPose(ts, pointcloud_samples_sample, pointcloud2body, body2odometry);
        hasNewPointCloud = true;
    }
}


/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See PointcloudInMLS.hpp for more detailed
// documentation about them.

bool PointcloudInMLS::configureHook()
{
    if (! PointcloudInMLSBase::configureHook())
        return false;
    
    body_frame = _body_frame.get();
    
    return true;
}
bool PointcloudInMLS::startHook()
{
    if (! PointcloudInMLSBase::startHook())
        return false;
    return true;
}
void PointcloudInMLS::updateHook()
{
    //process all callbacks
    PointcloudInMLSBase::updateHook();

    //if we got a new pointcloud, perform alignment on the latest one
    if(hasNewPointCloud)
    {
        PCLPointCloudPtr pcl_pointcloud(new PCLPointCloud());
        if(!pc.pointcloud2body.matrix().isApprox(Eigen::Matrix4d::Identity()))
        {
            // apply transformation
            std::vector<base::Vector3d> transformed_pointcloud;
            transformed_pointcloud.reserve(pc.pointcloud_sample.points.size());
            for(std::vector<base::Vector3d>::const_iterator it = pc.pointcloud_sample.points.begin(); it != pc.pointcloud_sample.points.end(); it++)
            {
                transformed_pointcloud.push_back(pc.pointcloud2body * (*it));
            }
            convertBaseToPCLPointCloud(transformed_pointcloud, *pcl_pointcloud);
        }
        else
            convertBaseToPCLPointCloud(pc.pointcloud_sample.points, *pcl_pointcloud);

        alignPointcloud(pc.time, pcl_pointcloud, pc.body2odometry);
        hasNewPointCloud = false;
    }
}
void PointcloudInMLS::errorHook()
{
    PointcloudInMLSBase::errorHook();
}
void PointcloudInMLS::stopHook()
{
    PointcloudInMLSBase::stopHook();
}
void PointcloudInMLS::cleanupHook()
{
    PointcloudInMLSBase::cleanupHook();
}
