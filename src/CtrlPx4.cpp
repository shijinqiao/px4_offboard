#include "px4_offboard/include.h"
#include "px4_offboard/CtrlPx4.h"

CtrlPx4::CtrlPx4()
{
  mavros_acc_pub         =   nh.advertise<geometry_msgs::Vector3Stamped>("/mavros/setpoint_accel/accel",50);
  mavros_pos_pub         =   nh.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_position/local",50);
  mavros_vel_pub         =   nh.advertise<geometry_msgs::TwistStamped>("/mavros/setpoint_velocity/cmd_vel",50);
  mavros_set_mode_client =   nh.serviceClient<mavros_msgs::SetMode>("/mavros/set_mode");
  mavros_armed_client    =   nh.serviceClient<mavros_msgs::CommandBool>("/mavros/cmd/arming");
  state_sub              =   nh.subscribe("/mavros/state", 100, &CtrlPx4::stateCallback,this);
  radio_vel_sub          =   nh.subscribe("/mavros/rc/in",100,&CtrlPx4::radioCallback,this);
  joy_vel_sub            =   nh.subscribe("/joy/cmd_vel",100,&CtrlPx4::joyVelCallback,this);
  joy_state_sub          =   nh.subscribe("/joy/state",100,&CtrlPx4::joyStateCallback,this);
};

bool CtrlPx4::commandUpdate()
{
//  if (OffSw) {
    if (!stateCmp())
    {
        set_armed.request.value = state_set.armed;
        mavros_armed_client.call(set_armed);

        if (state_set.offboard)
          set_mode.request.custom_mode = "OFFBOARD";
        else
          set_mode.request.custom_mode = "MANUAL";

        ros::spinOnce();
        mavros_set_mode_client.call(set_mode);
    }
    // if state is armed publish fmu_controller commnad
    if (state_set.armed)
    {

      #ifdef VELOCITY
      mavros_vel_pub.publish(fmu_controller_setpoint);
      ROS_INFO("Command: [x: %f y:%f z: %f, yaw:%f]", fmu_controller_setpoint.twist.linear.x,fmu_controller_setpoint.twist.linear.y,fmu_controller_setpoint.twist.linear.z,fmu_controller_setpoint.twist.angular.z);
      #endif

      #ifdef ACCELERATION
      mavros_acc_pub.publish(fmu_controller_setpoint);
      ROS_INFO("Command: [x: %f y:%f z: %f]", fmu_controller_setpoint.vector.x,fmu_controller_setpoint.vector.y,fmu_controller_setpoint.vector.z);
      #endif

    }
  //}
  //else
  //{
  //  ROS_INFO("In Manual Mode");
  //}


}

bool CtrlPx4::stateCmp()
{
  myState *read = &state_read;
  myState *set  = &state_set;
  return ((read->armed == set->armed)&&(read->offboard == set->offboard)&&(read->control == set->control));
}


void CtrlPx4::joyVelCallback(const geometry_msgs::Twist twist)
{
    #ifdef VELOCITY
    fmu_controller_setpoint.twist.linear.x = twist.linear.x;
    fmu_controller_setpoint.twist.linear.y = twist.linear.y;
    fmu_controller_setpoint.twist.linear.z = twist.linear.z;
    fmu_controller_setpoint.twist.angular.x = twist.angular.x;
    fmu_controller_setpoint.twist.angular.y = twist.angular.y;
    fmu_controller_setpoint.twist.angular.z = twist.angular.z;
    #endif

    #ifdef ACCELERATION
    fmu_controller_setpoint.vector.x = twist.linear.x;
    fmu_controller_setpoint.vector.y = twist.linear.y;
    fmu_controller_setpoint.vector.z = twist.linear.z;
    // fmu_controller_setpoint.vector.twist.angular.x = twist.angular.x;
    // fmu_controller_setpoint.vector.twist.angular.y = twist.angular.y;
    // fmu_controller_setpoint.vector.twist.angular.z = twist.angular.z;
    #endif
}

void CtrlPx4::stateCallback(const mavros_msgs::State state)
{
	 state_read.armed = (state.armed==128);
	 state_read.offboard = (strcmp(state.mode.c_str(),"OFFBOARD")==0);
}

void CtrlPx4::radioCallback(const mavros_msgs::RCIn rc_in)
{
    OffSw = (rc_in.channels[5]<1200) && (rc_in.channels[7]>1200); //
}

void CtrlPx4::joyStateCallback(const std_msgs::Byte state)
{
    switch (state.data)
    {
      //DISARM
      case 0:
        state_set.armed    =   0;
        state_set.offboard =   0;
        break;
      //ARM
      case 1:
        state_set.armed    =   1;
        state_set.offboard =   1;
        break;

     //ARMED BUT NO OFFBOARD COMMAND (DANGEROUS)
      case 2:
        state_set.armed    =   1;
        state_set.offboard =   0;
        break;
    }
}
