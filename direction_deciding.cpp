/******************************************************************************
 * File Name    : direction_deciding.cpp
 * Description  : This file contains the logic for Service 3, responsible for
 *                determining the robot's movement direction and speed based 
 *                on the position of the detected red laser dot.
 * 
 *                The service computes movement commands such as FORWARD, LEFT,
 *                RIGHT, or STOP, and assigns speed levels depending on the
 *                vertical distance of the laser from the robot.
 *
 * Authors      : Bhavya Saravanan, Lokesh Senthil Kumar, Nalin Saxena
 ******************************************************************************/

 #include "red_laser_service.hpp"
 #include <syslog.h>
 #include "direction_deciding.hpp"
 #include <optional>  
 #include "watchdog.hpp"
 
 // Shared variable to store the most recently computed movement command
 std::optional<MovementCommand> latest_cmd;
 std::mutex cmd_mutex;
 std::atomic<bool> cmd_available{false};
 
 /**
  * @brief Determines the robot's movement direction and speed based on the 
  *        given position of the red laser dot.
  * 
  * @param pos - The 2D coordinates of the red laser dot.
  * @return MovementCommand - The decided movement direction and speed level.
  */
 MovementCommand service3_decide_direction(Point2D pos) {
     MovementCommand cmd;
 
     bool isLeft   = (pos.x < 200);
     bool isCenter = (pos.x >= 200) && (pos.x <= 450) && (pos.y <= 480);
     bool isRight  = (pos.x > 450);
     bool isTop    = (pos.y < 400);
     bool isBottom = (pos.y > 400);
 
     if (isLeft && (isTop || isBottom)) {
         cmd.dir = LEFT;
     }
     else if (isRight && (isTop || isBottom)) {
         cmd.dir = RIGHT;
     }
     else if (isCenter) {
         cmd.dir = FORWARD;
     }
     else {
         cmd.dir = STOP;
     }
 
     // Assign speed level based on how far the red dot is from the robot (y-axis)
     if (pos.y < 160) {
         cmd.speed_level = 3; // Fast
     }
     else if (pos.y <= 320) {
         cmd.speed_level = 2; // Medium
     }
     else {
         cmd.speed_level = 1; // Slow
     }
 
     return cmd;
 }
 
 /**
  * @brief Main thread function for Service 3.
  *        Reads the latest red laser position, calculates direction & speed,
  *        updates the global movement command, and logs the decision.
  */
 void service3_thread() {
     if (!point_available.load(std::memory_order_acquire)) {
        service3_ok = true;
         return;
     }
 
     std::optional<Point2D> pointer_location;
     {
         std::lock_guard<std::mutex> lock(point_mutex);
         pointer_location = latest_laser_point;
         point_available.store(false, std::memory_order_release);
     }
 
     if (!pointer_location) return;
 
     auto cmd = service3_decide_direction(*pointer_location);
     {
         std::lock_guard lock(cmd_mutex);
         latest_cmd = cmd;
         cmd_available.store(true, std::memory_order_release);
     }
 
     const char* dirStr = "STOP";
     switch (cmd.dir) {
         case FORWARD:                 dirStr = "FORWARD";        break;
         case LEFT:                    dirStr = "LEFT";           break;
         case RIGHT:                   dirStr = "RIGHT";          break;
         case STOP: default:           dirStr = "STOP";            break;
     }
 
     syslog(LOG_INFO,
            "Service 3 → Direction: %s | Speed Level: %d | Position: (%d, %d)",
            dirStr, cmd.speed_level, pointer_location->x, pointer_location->y);
 
     service3_ok = true; // Signal to watchdog that this service is alive
 }
 
