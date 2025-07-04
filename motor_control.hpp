/***************************************************************
 * File: motor_control.hpp
 * Description: Header file for Service 4 - Motor control service.
 *              Declares the motor control service function and
 *              initialization routine for GPIO and PWM.
 * Author: Lokesh Senthil Kumar, Bhavya Saravanan, Nalin Saxena
 ***************************************************************/

#pragma once

#include <optional>
#include "direction_deciding.hpp"  // For MovementCommand definition
#include <pigpio.h>                // For gpioPWM functions
#include <linux/gpio.h>            // For gpiohandle structures

// Initializes the MotorDriver by configuring GPIO chip and requesting output lines
bool motor_driver_init();

// Main service function for motor control
// This reads MovementCommand and drives motors accordingly
void motor_control_service();
