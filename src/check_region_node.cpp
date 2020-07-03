#include <ros/ros.h>
#include <algorithm>
// #include <gazebo/gazebo_client.hh>
// #include <gazebo/transport/transport.hh>
#include <gazebo_msgs/ModelStates.h>
#include <social_worlds/Regions.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>

#include <typeinfo>

class CheckRegion
{
  private:
  // rate
  // ros::Rate loop_rate;

  // parameters
  std::string robot_name;
  std::string region_name;

  // variables
  geometry_msgs::Pose robot_pose;
  std::vector<geometry_msgs::Point> region_points;

  // publisher
  ros::Publisher pub_check;

  // Subscriber
  // gazebo::transport::SubscriberPtr sub_models;
  ros::Subscriber sub_models;

  // service client
  ros::ServiceClient srv_region;


public:
  CheckRegion(){

    // gazebo transport node initialization
    // gazebo::transport::NodePtr node(new gazebo::transport::Node());
    // node->Init();

    // ros NodeHandle and loop rate
    ros::NodeHandle n("~");
    // this->loop_rate = ros::Rate(10);


    // parameters
    n.getParam("robot_name", this->robot_name);
    n.getParam("region_name", this->region_name);
    ROS_INFO_STREAM("robot_name: " << this->robot_name);
    ROS_INFO_STREAM("region_name: " << this->region_name);

    this->pub_check = n.advertise<geometry_msgs::Point>
      ("", 1000);

    // this->sub_models = node->Subscribe("/gazebo/model_states",
    //   &CheckRegion::modelsCallback, this);
    this->sub_models = n.subscribe("/gazebo/model_states", 1000,
      &CheckRegion::modelsCallback, this);

    std::string s = "/regions/" + this->region_name;
    this->srv_region = n.serviceClient<social_worlds::Regions>(s);
    this->srv_region.waitForExistence();

  }

  ~CheckRegion()
  {
    // gazebo::client::shutdown();
  }

  // void modelsCallback(ConstModelStatesPtr &_msg)
  void modelsCallback(const gazebo_msgs::ModelStates::ConstPtr& msg)
  {
    std::vector<std::string> models = msg->name;
    std::vector<std::string>::iterator it =
      std::find(models.begin(), models.end(), this->robot_name);
      if (it != models.end()){
        int index = std::distance(models.begin(), it);
        this->robot_pose = msg->pose[index];
      }
  }

  void start()
  {
    social_worlds::Regions srv;
    bool ok = false;
    do{
      ROS_INFO_STREAM("Trying to get region points.");
      ok = this->srv_region.call(srv);
      ros::Rate(10).sleep();
    }while(!ok);
    this->region_points = srv.response.points;

    while (ros::ok())
    {

      std::vector<geometry_msgs::Point>::iterator it;
      for (it = this->region_points.begin(); it < this->region_points.end(); it++)
      {
        double dist = sqrt(
          pow(this->robot_pose.position.x - (*it).x,2)+
          pow(this->robot_pose.position.y - (*it).y,2));
        if(dist <= 0.05)
        {
          ROS_DEBUG_STREAM("dist to region: " << dist);
          geometry_msgs::Point check_msg;
          check_msg.x = (*it).x;
          check_msg.y = (*it).y;
          ROS_DEBUG_STREAM("point: (" << check_msg.x << "," << check_msg.y << "). Dist: " << dist << ".");
          this->pub_check.publish(check_msg);

          break;
        }
      }


      ros::Rate(10).sleep();
      ros::spinOnce();
    }

  }

};


int main(int argc, char **argv){

  // init gazebo and ros
  ros::init(argc, argv, "check_region_node");
  // gazebo::client::setup(argc, argv);

  // start node
  CheckRegion* cr = new CheckRegion();
  cr->start();

  return 0;
}