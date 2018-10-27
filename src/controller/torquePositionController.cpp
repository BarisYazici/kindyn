#include <type_traits>

#include <controller_interface/controller.h>
#include <hardware_interface/joint_command_interface.h>
#include <pluginlib/class_list_macros.h>
#include "kindyn/robot.hpp"
#include <roboy_communication_simulation/ControllerState.h>
#include <std_msgs/Float32.h>

using namespace std;

class TorquePositionController : public controller_interface::Controller<hardware_interface::EffortJointInterface> {
public:
    TorquePositionController() {

    };

    bool init(hardware_interface::EffortJointInterface *hw, ros::NodeHandle &n) {
        nh = n;
        // get joint name from the parameter server
        if (!n.getParam("joint", joint_name)) {
            ROS_ERROR("No joint given (namespace: %s)", n.getNamespace().c_str());
            return false;
        }
        spinner.reset(new ros::AsyncSpinner(0));
        spinner->start();
        controller_state = nh.advertise<roboy_communication_simulation::ControllerState>("/controller_type",1);
        ros::Rate r(10);
        while(controller_state.getNumSubscribers()==0)
            r.sleep();
        joint = hw->getHandle(joint_name); // throws on failure
        last_update = ros::Time::now();
        joint_command = nh.subscribe((joint_name+"/target").c_str(),1,&TorquePositionController::JointCommand, this);
        return true;
    }

    void update(const ros::Time &time, const ros::Duration &period) {
        double q = joint.getPosition();
        double qd = joint.getVelocity();
        double p_error = q - q_target;
        double cmd = Kp * p_error + Kd *qd;
        joint.setCommand(cmd);
        p_error_last = p_error;
        last_update = time;
//        ROS_INFO_THROTTLE(1, "%s target %lf current %lf", joint_name.c_str(), q_target, q);
    }

    void starting(const ros::Time& time) {
        ROS_WARN("torque position controller started for %s", joint_name.c_str());
        roboy_communication_simulation::ControllerState msg;
        msg.joint_name = joint_name;
        msg.type = 0;
        controller_state.publish(msg);
    }
    void stopping(const ros::Time& time) { ROS_WARN("cable length controller stopped for %s", joint_name.c_str());}

    void JointCommand(const std_msgs::Float32ConstPtr &msg){
        q_target = msg->data;
    }
private:
    double q_target = 0;
    double Kp = 1000, Kd = 5;
    double p_error_last = 0;
    ros::NodeHandle nh;
    ros::Publisher controller_state;
    ros::Subscriber joint_command;
    boost::shared_ptr<ros::AsyncSpinner> spinner;
    hardware_interface::JointHandle joint;
    string joint_name;
    int joint_index;
    ros::Time last_update;
};
PLUGINLIB_EXPORT_CLASS(TorquePositionController, controller_interface::ControllerBase);
