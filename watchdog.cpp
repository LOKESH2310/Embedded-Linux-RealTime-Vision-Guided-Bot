/**
 * @file    watchdog.cpp
 * @brief   Implementation of the hardware watchdog service using /dev/watchdog.
 *
 * This service runs on Core 2 and monitors the health of the four essential
 * services running on Core 1. It uses Linux's watchdog device to reset the
 * Raspberry Pi if any of the services become unresponsive.
 *
 * The watchdog is kicked every 100ms by the sequencer only if all services
 * report successful execution by setting their flags to true.
 *
 * @authors Lokesh Senthil Kumar
 */

 #include "watchdog.hpp"
 #include <chrono>
 #include <ctime>
 #include <syslog.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <cstring>
 #include <linux/watchdog.h>
 #include <sys/ioctl.h>
 
 // Atomic flags updated by each service upon successful execution
 std::atomic<bool> service1_ok{false};
 std::atomic<bool> service2_ok{false};
 std::atomic<bool> service3_ok{false};
 std::atomic<bool> service4_ok{false};
 
 /**
  * @brief Monitors services and kicks the hardware watchdog if all are OK.
  *
  * This function:
  * - Opens /dev/watchdog once and sets a 5-second timeout.
  * - On every execution, checks if all service flags are set to true.
  * - If yes, sends a keep-alive signal to the watchdog to prevent reset.
  * - If not, the system is allowed to reset after the timeout expires.
  */
 void watchdog_service() {
     static int fd = -1;
 
     // Open /dev/watchdog only once
     if (fd == -1) {
         fd = open("/dev/watchdog", O_WRONLY);
         if (fd < 0) {
             syslog(LOG_ERR, "Watchdog: failed to open /dev/watchdog: %s", strerror(errno));
             return;
         }
 
         int timeout = 5; // Set timeout in seconds
         if (ioctl(fd, WDIOC_SETTIMEOUT, &timeout) < 0) {
             syslog(LOG_ERR, "Watchdog: failed to set timeout: %s", strerror(errno));
         } else {
             syslog(LOG_INFO, "Watchdog armed with timeout = %d seconds", timeout);
         }
     }
    // Check flags and reset them
     if (service1_ok && service2_ok && service3_ok && service4_ok) {
         write(fd, "\0", 1);  // Kick the watchdog
         service1_ok = service2_ok = service3_ok = service4_ok = false;
     } else {
         syslog(LOG_ERR, "Watchdog: One or more services FAILED and System is resetting");
         
     }
 }
 
