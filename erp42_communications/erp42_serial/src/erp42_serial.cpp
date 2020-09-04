#include <erp42_serial.h>


using namespace unmansol::erp42;

ERP42Serial::ERP42Serial(
    const char *device_name,
    int serial_baudrate):
  serial_port_(device_name, serial_baudrate),
  m_nh("~")
{
  Init_data();
  Init_node();
}


void ERP42Serial::Init_data()
{
  m_pc2erp.speed._speed = 0x00;
  m_pc2erp.steer._steer = 0x00;
  m_pc2erp.brake = 0x00;
}

void ERP42Serial::Init_node()
{
  // DEBUG MODE
  if (ros :: console :: set_logger_level (ROSCONSOLE_DEFAULT_NAME, ros :: console :: levels :: Debug)) {
    ros :: console :: notifyLoggerLevelsChanged ();
  }

  std::string ns = ros::this_node::getNamespace();

  m_pub_feedback = m_nh.advertise<erp42_msgs::SerialFeedBack>(ns + "/feedback",1);
  m_pub_cmdcontrol = m_nh.advertise<erp42_msgs::CmdControl>(ns + "/command", 1);
  m_sub_mode = m_nh.subscribe(ns + "/mode", 1, &ERP42Serial::ModeCallback, this);
  m_sub_drive = m_nh.subscribe(ns + "/drive", 1, &ERP42Serial::DriveCallback, this);
}

void ERP42Serial::Write()
{
  //  std::cout << "test" << std::endl;
  unsigned char writeBuffer[14];
  Update(writeBuffer);
  serial_port_.Write(writeBuffer,14);
  m_pub_cmdcontrol.publish(m_cmdcontrol_msg);
}

void ERP42Serial::Update(unsigned char (&buffer)[14])
{
  m_pc2erp.speed.speed[0] = m_pc2erp.speed._speed & 0xff;
  m_pc2erp.speed.speed[1] = (m_pc2erp.speed._speed & 0xff00) >> 8;
  m_pc2erp.steer.steer[0] = (m_pc2erp.steer._steer & 0xff);
  m_pc2erp.steer.steer[1] = (m_pc2erp.steer._steer & 0xff00) >> 8;

  buffer[0] = m_pc2erp.S;
  buffer[1] = m_pc2erp.T;
  buffer[2] = m_pc2erp.X;
  buffer[3] = m_pc2erp.MorA;
  buffer[4] = m_pc2erp.E_stop;
  buffer[5] = m_pc2erp.gear;
  buffer[6] = m_pc2erp.speed.speed[1];
  buffer[7] = m_pc2erp.speed.speed[0];
  buffer[8] = m_pc2erp.steer.steer[1];
  buffer[9] = m_pc2erp.steer.steer[0];
  buffer[10] = m_pc2erp.brake;
  buffer[11] = m_AlvCnt++;
  buffer[12] = m_pc2erp.ETX0;
  buffer[13] = m_pc2erp.ETX1;

  // if It is Auto Mode, it can write serial data
  if (m_feedback_msg.MorA == 0x01)
  {
    std::cout << "\033[1;33mSending Packet..\033[0m\n";
    for(int i=0; i<14; i++)
    {
      std::cout << std::hex << (int)buffer[i] << " ";
    }
  }
  else
  {
    for(int i=0; i<14; i++)
    {
      std::cout << std::hex << (int)buffer[i] << " ";
    }
  }
  std::cout << std::endl;
}

void ERP42Serial::Read()
{
  unsigned char num_read = 18;
  //  m_read_data = new unsigned char[num_read];
  serial_port_.Read(m_read_data, num_read);
  Update();
}

void ERP42Serial::Update()
{
  int idx = 0;

  if (char(m_read_data[idx]) == 'S' && char(m_read_data[idx + 1]) == 'T' && char(m_read_data[idx + 2]) == 'X')
  {
    if (char(m_read_data[idx+16]) == 0x0D && char(m_read_data[idx + 17]) == 0x0A)
    {
      m_feedback_msg.MorA  = m_read_data[idx + 3];
      m_feedback_msg.EStop = m_read_data[idx + 4];
      m_feedback_msg.Gear  = m_read_data[idx + 5];

      int speed_now = 0;
      speed_now |= (int)((m_read_data[idx + 6]) & 0xff);
      speed_now |= (int)((m_read_data[idx + 7] << 8) & 0xff00);
      m_feedback_msg.speed = speed_now/SPEED_FACTOR;



      int steer_now = 0;
      steer_now |= (int)((m_read_data[idx + 8]) & 0xff);
      steer_now |= (int)((m_read_data[idx + 9] << 8) & 0xff00);
      if (steer_now > 30000)
        steer_now = steer_now - 65536;
      m_feedback_msg.steer = steer_now/STEER_FACTOR;

      m_feedback_msg.brake   = m_read_data[idx + 10];

      int32_t encoder = 0;
      encoder |= (int32_t)((m_read_data[idx + 11]) & 0xff);
      encoder |= (int32_t)((m_read_data[idx + 12] << 8) & 0xff00);
      encoder |= (int32_t)((m_read_data[idx + 13] << 16) & 0xff0000);
      encoder |= (int32_t)((m_read_data[idx + 14] << 24) & 0xff000000);

      m_feedback_msg.encoder = encoder;

      //      for(int i=11; i<15; i++)
      //      {
      //        std::cout << std::hex << (int)m_read_data[i] << " ";
      //      }
      //      std::cout<< "encoder : " << (int)encoder << std::endl;

      m_feedback_msg.alive = m_read_data[idx+15];

      m_pub_feedback.publish(m_feedback_msg);
    }
  }
  else
  {
    ROS_WARN("Incorrect data has been entered");
  }

}

void ERP42Serial::ModeCallback(const erp42_msgs::ModeCmd::Ptr &msg)
{
  m_pc2erp.MorA = msg->MorA;
  m_pc2erp.E_stop = msg->EStop;
  m_pc2erp.gear = msg->Gear;

  m_cmdcontrol_msg.MorA = msg->MorA;
  m_cmdcontrol_msg.EStop = msg->EStop;
  m_cmdcontrol_msg.Gear = msg->Gear;

}

void ERP42Serial::DriveCallback(const erp42_msgs::DriveCmd::Ptr &msg)
{
  m_pc2erp.speed._speed = msg->KPH * SPEED_FACTOR;
  m_pc2erp.steer._steer = msg->Deg * (-1)*STEER_FACTOR;
  m_pc2erp.brake = msg->brake;

  m_cmdcontrol_msg.KPH = msg->KPH;
  m_cmdcontrol_msg.Deg = msg->Deg;
  m_cmdcontrol_msg.brake = msg->brake;
}



