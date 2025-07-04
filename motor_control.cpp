/***************************************************************
 * File: motor_control.cpp
 * Description: Implements Service 4 - Motor control logic for a
 *              real-time robot using GPIO and PWM. Interprets
 *              high-level movement commands and actuates motors
 *              accordingly via pigpio and Linux GPIO character device.
 * Author: Lokesh Senthil Kumar, Bhavya Saravanan, Nalin Saxena
 ***************************************************************/

#include "motor_control.hpp"

#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <cstring>
#include <atomic>
#include "watchdog.hpp"

// Externally shared movement command and sync primitives
extern std::optional<MovementCommand> latest_cmd;
extern std::mutex cmd_mutex;
extern std::atomic<bool> cmd_available;

// Class responsible for direct GPIO-based motor control
class MotorDriver {
public:
    // Initialize the GPIO driver and request output lines
    bool init() {
        chipFd = open("/dev/gpiochip0", O_RDONLY);
        if (chipFd < 0) {
            syslog(LOG_ERR, "MotorDriver open failed: %s", strerror(errno));
            return false;
        }

        gpiohandle_request req{};
        req.lines = 6;
        for (int i = 0; i < 6; ++i) req.lineoffsets[i] = offsets[i];
        req.flags = GPIOHANDLE_REQUEST_OUTPUT;

        if (ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
            syslog(LOG_ERR, "MotorDriver line request failed: %s", strerror(errno));
            close(chipFd);
            return false;
        }

        lineFd = req.fd;
        return true;
    }

    // Send drive signal to motors based on direction and speed
    void drive(bool motor1_forward, bool motor1_backward,
               bool motor2_forward, bool motor2_backward,
               int speed, const char* direction) {

        int pwmPin = 18, pwmPin1 = 19;

        // Calculate PWM duty cycle based on speed level
        int duty = (speed == 3) ? 100 :
                   (speed == 2) ? 80  :
                   (speed == 1) ? 70  : 0;

        gpioPWM(pwmPin, duty);     // Set PWM for motor A
        gpioPWM(pwmPin1, duty);    // Set PWM for motor B

        // Set GPIO direction bits
        data.values[0] = motor1_forward;
        data.values[1] = motor1_backward;
        data.values[2] = motor2_forward;
        data.values[3] = motor2_backward;

        // Apply changes to GPIO hardware
        ioctl(lineFd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

        if (speed > 0) {
            syslog(LOG_INFO, "MotorDrive → Direction: %s | PWM Duty: %d%%", direction, duty);
        }
    }

    // Destructor to cleanly release GPIO handles
    ~MotorDriver() {
        if (lineFd >= 0) close(lineFd);
        if (chipFd >= 0) close(chipFd);
    }

private:
    int chipFd{-1}, lineFd{-1};
    gpiohandle_data data{};
    // Pin mapping: IN1, IN2, IN3, IN4, ENA, ENB
    unsigned offsets[6]{17, 27, 22, 23, 18, 19};
};

// Global motor driver instance
static MotorDriver drv;

// External interface to initialize the driver
bool motor_driver_init() {
    return drv.init();
}

// Core of Service 4: Reads MovementCommand and drives motors
void motor_control_service() {
    static bool initialized = false;

    // Initialize motor GPIO driver once
    if (!initialized) {
        if (!motor_driver_init()) {
            syslog(LOG_ERR, "Motor driver initialization failed.");
            return;
        }
        initialized = true;
    }

    // Default state: Stop
    const char* direction = "STOP";
    bool motor1_forward = false, motor1_backward = false;
    bool motor2_forward = false, motor2_backward = false;
    int speed = 0;

    // If no new command, stop the robot
    if (!cmd_available.load(std::memory_order_acquire)) {
        drv.drive(motor1_forward, motor1_backward, motor2_forward, motor2_backward, speed, direction);
        service4_ok = true;
        return;
    }

    std::optional<MovementCommand> new_command;
    {
        // Safely retrieve the command and mark it consumed
        std::lock_guard<std::mutex> lock(cmd_mutex);
        new_command = latest_cmd;
        cmd_available.store(false, std::memory_order_release);
    }

    // Interpret command and set direction pins
    if (new_command) {
        speed = new_command->speed_level;

        if (new_command->behav) {
            // Behavior mode 1: normal forward logic
            switch (new_command->dir) {
                case FORWARD: motor1_forward = true;  motor2_forward = true; direction = "FORWARD"; break;
                case LEFT:    motor1_backward = false; motor2_forward = true; direction = "LEFT";    break;
                case RIGHT:   motor1_forward = true;  motor2_backward = false; direction = "RIGHT";  break;
                case STOP:
                default:      speed = 0; direction = "STOP"; break;
            }
        } else {
            // Behavior mode 2: alternative wiring logic
            switch (new_command->dir) {
                case FORWARD: motor1_backward = true;  motor2_backward = true; direction = "FORWARD"; break;
                case LEFT:    motor1_backward = true; motor2_forward = false; direction = "LEFT";     break;
                case RIGHT:   motor1_forward = false; motor2_backward = true; direction = "RIGHT";    break;
                case STOP:
                default:      speed = 0; direction = "STOP"; break;
            }
        }
    }

    // Drive motors based on interpreted direction
    drv.drive(motor1_forward, motor1_backward, motor2_forward, motor2_backward, speed, direction);
    service4_ok = true;
}
