#pragma once
#include <mutex>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <syslog.h>
#include <optional>
#include <atomic>

#define NSEC_PER_SEC (1000000000)

struct HSVConfig {
    int hue_min, hue_max;
    int sat_min, sat_max;
    int val_min, val_max;
    int behaviour;
    
    HSVConfig() {
        hue_min = 20;
        hue_max = 160;
        sat_min = 100;
        sat_max = 255;
        val_min = 200;
        val_max = 255;
        behaviour=1;
    }
};



struct Point2D {
    int x;
    int y;
    int behav;
};

extern std::mutex config_mutex;
extern HSVConfig config; 

extern cv::Mat latest_frame;
extern std::mutex frame_mutex;

extern std::optional<Point2D> latest_laser_point;
extern std::mutex point_mutex;
extern std::atomic<bool> point_available; 

void red_laser_detect();
