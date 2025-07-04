/******************************************************************************
 * File Name    : direction_deciding.hpp
 * Description  : Header file for Service 3, defines data structures and
 *                function prototypes for laser-based movement decisions.
 *
 * Authors      : Bhavya Saravanan, Lokesh Senthil Kumar, Nalin Saxena
 ******************************************************************************/

#pragma once
#include <optional>
#include <mutex>
#include <syslog.h>
#include <atomic>
#include "red_laser_service.hpp"

 // Enum defining movement directions
 enum Direction {
     FORWARD,
     LEFT,
     RIGHT,
     STOP
 };
 
 // Structure defining a movement command with direction , speed and to control the config behaviour
 struct MovementCommand {
     Direction dir;
     int speed_level;
     int config_behave;
 };
 
 // Shared data declarations
 extern std::optional<MovementCommand> latest_cmd;
 extern std::mutex cmd_mutex;
 extern std::atomic<bool> cmd_available;
 
 /**
  * @brief Calculates movement direction and speed based on laser dot position.
  * 
  * @param pos Position of the laser dot in the captured frame.
  * @return MovementCommand Decided command with direction and speed.
  */
 MovementCommand service3_decide_direction(Point2D pos);
 
 /**
  * @brief Thread function that runs Service 3 logic.
  *        Fetches laser point, decides command, logs, and updates globals.
  */
 void service3_thread();
 
