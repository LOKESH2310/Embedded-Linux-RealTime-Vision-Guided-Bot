# 🚗 Real-Time Laser-Guided Robot Using C++ on Embedded Linux

This project implements a **real-time, laser-guided robotic system** built using C++ on Embedded Linux. The robot visually tracks a laser dot (red or green) using a camera and follows it by calculating movement commands and actuating motors. The system is designed with **modular real-time services**, scheduled using a custom **cyclic executive sequencer**, and uses low-level Linux interfaces (V4L2, GPIO, pigpio) for performance and precision.

---

## 📌 Features

- Real-time C++ service architecture with strict priority and CPU affinity control
- Laser dot detection using OpenCV in HSV color space
- Non-blocking V4L2 video pipeline using memory-mapped buffers and ioctl
- Direction decision-making based on laser dot position in the video frame
- Motor control via GPIO + PWM using the pigpio library and Linux character device interface
- Runtime configuration switching (e.g., red-to-green laser change) via JSON file
- Thread-safe shared buffers and atomic flags
- Watchdog monitoring to track service health and detect timeouts
- Profiling with perf & debugging with GDB

---

## 🧠 System Overview

### 🎥 Vision Input
- Captures video frames from `/dev/video0` using V4L2 (YUYV format, 640x480 @ 30Hz)
- Converts YUYV to BGR and detects red or green laser dot using HSV thresholding
- Finds the centroid of the largest detected contour as the laser point

### 🧭 Direction Decision
- Based on the laser dot's (x, y) position:
  - Left/right movement for x-coordinate
  - Speed level determined by y-coordinate
- Encoded into a `MovementCommand` with direction, speed, and behavior mode

### 🛞 Motor Control
- Uses GPIO character device interface to control IN1–IN4 (direction) and ENA/ENB (PWM)
- pigpio library used for PWM duty control based on speed level
- Switches control logic based on behavior flag (normal vs alternate wiring)

### ⚙️ Runtime Configuration
- A JSON file is monitored for live config changes (e.g., switching color tracking mode)
- Enables dynamic behavior without restarting the system

### 🔒 Real-Time Coordination
- Custom sequencer schedules services with priorities, periods, and real-time flags
- OS primitives: `std::thread`, `std::mutex`, `std::atomic`, `sched_setscheduler`, `mlockall`
- Watchdog checks per-service `ok` flags to ensure responsiveness

---

## 🧪 Services Breakdown

| Service     | Description                                      |
|-------------|--------------------------------------------------|
| Service 1   | Captures camera frames using V4L2                |
| Service 2   | Detects laser dot and updates shared point       |
| Service 3   | Decides direction and speed based on dot location|
| Service 4   | Drives motors via GPIO and PWM                   |
| Service 5   | Monitors JSON config file for runtime changes    |
| Watchdog    | Monitors services for heartbeat and health       |

---

## 📈 Performance & Optimization

- Used `perf stat` and `perf top` to profile cache misses, instructions per cycle (IPC), and context switches
- Focused on responsiveness over micro-optimization
- Optimized thread scheduling, non-blocking I/O, and buffer access patterns

---

## 🧰 Tools & Libraries

- `pigpio` – PWM and GPIO control
- `OpenCV` – Image processing
- `perf`, `GDB` – Profiling and debugging
- `nlohmann/json` – Config parsing
- `std::thread`, `mutex`, `atomic`, `chrono` – C++ STL concurrency primitives
- `sched.h`, `mlockall` – Real-time scheduling and memory locking

---
🙋 Author
Lokesh S.
M.S. in Embedded Systems
Real-Time Robotics | Linux | Vision-Guided Control

