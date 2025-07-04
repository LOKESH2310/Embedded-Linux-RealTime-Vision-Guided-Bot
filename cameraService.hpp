#pragma once
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <mutex>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define CAM_DEVICE "/dev/video0"
#define NBUF 4


struct Buffer {
    void* start;
    size_t length;
};
//a structure to hold buffers and file descriptor of camera
typedef struct CameraContext{
	int fd=-1;
	Buffer buffers[NBUF];
    v4l2_format fmt{};

}CameraContext;


extern cv::Mat latest_frame;
extern std::mutex frame_mutex;
extern CameraContext cam;

/*
 * initialze camera*
 * refrence https://www.marcusfolkesson.se/blog/capture-a-picture-with-v4l2/
 */ 
int init_camera();

//service implementation for camera capture
void camera_capture_service();
