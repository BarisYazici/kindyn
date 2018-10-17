#include <type_traits>

#include <controller_interface/controller.h>
#include <hardware_interface/joint_command_interface.h>
#include <pluginlib/class_list_macros.h>
#include "kindyn/robot.hpp"
#include "kindyn/controller/cardsflow_state_interface.hpp"
#include <roboy_communication_simulation/ControllerState.h>

using namespace std;

class CableLengthController : public controller_interface::Controller<hardware_interface::CardsflowCommandInterface> {
public:
    CableLengthController() {

    };

    bool init(hardware_interface::CardsflowCommandInterface *hw, ros::NodeHandle &n) {
        nh = n;
        // get joint name from the parameter server
        if (!n.getParam("joint", joint_name)) {
            ROS_ERROR("No joint given (namespace: %s)", n.getNamespace().c_str());
            return false;
        }
        spinner.reset(new ros::AsyncSpinner(0));
        spinner->start();
        controller_state = nh.advertise<roboy_communication_simulation::ControllerState>("/controller_type",1);
        joint = hw->getHandle(joint_name); // throws on failure
        joint_index = joint.getJointIndex();
        last_update = ros::Time::now();
        return true;
    }

    void update(const ros::Time &time, const ros::Duration &period) {
        double dt = (time-last_update).toSec();
        double q = joint.getPosition();
        double qd = joint.getVelocity();
        double q_target = joint.getJointPositionCommand();
        double qd_target = joint.getJointVelocityCommand();
        MatrixXd L = joint.getL();
        double p_error = q - q_target;
        VectorXd ld = L.col(joint_index) * (Kd * (p_error - p_error_last)/dt + Kp * p_error);
//        ROS_INFO_STREAM_THROTTLE(1, dt);
        joint.setMotorCommand(ld);
        p_error_last = p_error;
        last_update = time;
    }

    void starting(const ros::Time& time) {
        ROS_WARN("cable length controller started for %s", joint_name.c_str());
        roboy_communication_simulation::ControllerState msg;
        msg.joint_name = joint_name;
        msg.type = 2;
        controller_state.publish(msg);
    }
    void stopping(const ros::Time& time) { ROS_WARN("cable length controller stopped for %s", joint_name.c_str());}

private:
    double Kp = 1000, Kd = 10;
    double p_error_last = 0;
    ros::NodeHandle nh;
    ros::Publisher controller_state;
    boost::shared_ptr<ros::AsyncSpinner> spinner;
    hardware_interface::CardsflowHandle joint;
    string joint_name;
    int joint_index;
    ros::Time last_update;
};
PLUGINLIB_EXPORT_CLASS(CableLengthController, controller_interface::ControllerBase);