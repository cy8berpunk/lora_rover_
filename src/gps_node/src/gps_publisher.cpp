#include "ros/ros.h"

#include "std_msgs/String.h"
#include "gps_node/gps_raw.h"


#include "MicroNMEA.cpp"

#include <wiringSerial.h>
#include <wiringPi.h>

#include <sstream>
#include <iostream>
#include <limits>

int main(int argc, char **argv)
{
  int fd;

  ros::init(argc, argv, "gps_publisher");

  ros::NodeHandle n;

  gps_node::gps_raw gps_data;

  ros::Publisher chatter_pub = n.advertise<gps_node::gps_raw>("gps_raw", 1);

  ros::Rate loop_rate(1000);

	if (wiringPiSetup () == -1)
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
  }
  REINIT:if ((fd = serialOpen ("/dev/ttyS0", 9600)) < 0)
  {
   fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
  }

  //MicroNMEA library structures
  char nmeaBuffer[200];
  MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

  std::string data;
  std::string NavSystem;
  int NumSatellites;
  long lat;
  long lon;

  double latf;
  double lonf;

  int input = 0;
  while (ros::ok())
  {
    while (serialDataAvail (fd))
    {
      input = serialGetchar (fd);
      try
      {
        nmea.process(input);

        NumSatellites = nmea.getNumSatellites();

        lat = nmea.getLatitude();
        lon = nmea.getLongitude();

        // std::cout << lat << ", " << lon << std::endl;

        latf = (double) lat/1000000;
        lonf = (double) lon/1000000;

        // std::cout << "double: " << lat << ", " << lon << std::endl;

        // ROS_INFO("NumSatellites: %ld", NumSatellites);
        // ROS_INFO("Lat: %d ", latf);
        // ROS_INFO("Lon: %d", lonf);

        float debug_lat = 49.466602;
        float debug_lon = 10.967921;

        gps_data.num_sats = NumSatellites;
        gps_data.lat = latf;
        gps_data.lon = lonf;

      }
      catch (const std::invalid_argument& ia) {
          //std::cerr << "Invalid argument: " << ia.what() << std::endl;
          return -1;
      }

      catch (const std::out_of_range& oor) {
          //std::cerr << "Out of Range error: " << oor.what() << std::endl;
          return -2;
      }

      catch (const std::exception& e)
      {
          //std::cerr << "Undefined error: " << e.what() << std::endl;
          return -3;
      }


      chatter_pub.publish(gps_data);

      usleep(10000);

      ros::spinOnce();
      loop_rate.sleep();
      if(input==-1){
        goto REINIT;
      }
    }
  }

  ros::spin();

  return 0;
}
