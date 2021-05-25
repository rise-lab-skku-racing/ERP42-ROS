#pragma once
/*
 * Copyright 2020 UNMANNED SOLUTION CO., LTD.
 * @file serial_interface.h
 *
 * @brief ERP42 Serial Interface
 *
 * Created on: 2020. 8. 31
 * Contact: <jdj2261@unmansol.com>
 * Author: Daejong Jin (djjin)
 */

#include <ros/ros.h>
#include <iostream>
#include <sstream>
#include <fstream> // For file I/O (reading/writing to COM port)
#include <vector>
#include <string.h>  	// String function definitions
#include <unistd.h>  	// UNIX standard function definitions
#include <fcntl.h>   	// File control definitions
#include <errno.h>   	// Error number definitions
#include <termios.h> 	// POSIX terminal control definitions (struct termios)
#include <system_error>	// For throwing std::system_error
#include <math.h>

#include <erp42_msgs/CmdControl.h>
#include <erp42_msgs/SerialFeedBack.h>
#include <erp42_msgs/DriveCmd.h>
#include <erp42_msgs/ModeCmd.h>


namespace unmansol
{
namespace erp42
{
class SerialPort
{
public:
    SerialPort(const char *device_name, const int &baudrate);
    virtual ~SerialPort()
    {
        std::cout << "Serial Closed..." << std::endl;
    }

    ros::NodeHandle m_nh;
    std::string ns_;

    bool Open();
    void Close();
    void Configure();

    bool Read(unsigned char* rpacket, int packetsize);
    bool Read(std::string& data);
    bool Write(unsigned char* wpacket, int packetsize);
    bool Write(const std::string& data);

private:
    const char *m_device_name;
    const int m_baudrate;
    int fd; // File Descriptor
    std::vector<char> readBuffer;
    const static unsigned char defaultReadBufferSize = 255;

}; // class SerialInterface
} // namespace erp42
} // namespace unmansol