/**
 * @file    watchdog.hpp
 * @brief   Header file for hardware watchdog monitoring service.
 *
 * This file declares the watchdog service function and global atomic flags
 * used to track the status of each core service in the system.
 * The watchdog checks if all services are alive and responsive within
 * the defined time window and resets the system otherwise.
 *
 * @authors Bhavya Saravanan,
 *          Lokesh Senthil Kumar,
 *          Nalin Saxena
 */

 #ifndef WATCHDOG_HPP
 #define WATCHDOG_HPP
 
 #include <atomic>
 
 // Flags set to true by each core service if it executed successfully
 extern std::atomic<bool> service1_ok;
 extern std::atomic<bool> service2_ok;
 extern std::atomic<bool> service3_ok;
 extern std::atomic<bool> service4_ok;
 
 /**
  * @brief Periodic function executed by the Sequencer on Core 2.
  * 
  * This function monitors the health of four critical services by checking
  * atomic boolean flags set by those services. If all are OK, the function
  * "kicks" the hardware watchdog to prevent system reset. If any service fails
  * to report, the watchdog will eventually trigger a system reboot.
  */
 void watchdog_service();
 
 #endif // WATCHDOG_HPP
 
