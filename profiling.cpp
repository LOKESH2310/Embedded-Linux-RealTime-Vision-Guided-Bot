// main.cpp

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <syslog.h>
#include <opencv2/opencv.hpp>

#define CAM_DEVICE "/dev/video0"
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define NBUF 4
#define NSEC_PER_SEC 1000000000L

struct Buffer {
    void* start;
    size_t length;
};

struct CameraContext {
    int fd;
    v4l2_format fmt{};
    Buffer buffers[NBUF];
};

cv::Mat latest_frame;
std::mutex frame_mutex;
CameraContext cam;
std::atomic<bool> stop_requested(false);

int delta_t(struct timespec* stop, struct timespec* start, struct timespec* delta_t) {
    int dt_sec = stop->tv_sec - start->tv_sec;
    int dt_nsec = stop->tv_nsec - start->tv_nsec;

    if (dt_nsec < 0) {
        dt_sec--;
        dt_nsec += NSEC_PER_SEC;
    }

    delta_t->tv_sec = dt_sec;
    delta_t->tv_nsec = dt_nsec;
    return 1;
}

int init_camera() {
    cam.fd = open(CAM_DEVICE, O_RDWR | O_NONBLOCK);
    if (cam.fd == -1) {
        perror("Opening video device");
        return EXIT_FAILURE;
    }

    cam.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam.fmt.fmt.pix.width = FRAME_WIDTH;
    cam.fmt.fmt.pix.height = FRAME_HEIGHT;
    cam.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    cam.fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(cam.fd, VIDIOC_S_FMT, &cam.fmt) == -1) {
        perror("Setting Pixel Format");
        return EXIT_FAILURE;
    }

    v4l2_requestbuffers req = {};
    req.count = NBUF;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam.fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("Requesting Buffer");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NBUF; ++i) {
        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(cam.fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("Querying Buffer");
            return EXIT_FAILURE;
        }

        cam.buffers[i].length = buf.length;
        cam.buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam.fd, buf.m.offset);

        if (cam.buffers[i].start == MAP_FAILED) {
            perror("mmap failed");
            return EXIT_FAILURE;
        }

        if (ioctl(cam.fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Queue Buffer failed");
            return EXIT_FAILURE;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam.fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Starting streaming");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void detect_red_laser(const cv::Mat& frame) {
    struct timespec start = {0}, end = {0}, exec = {0};
    clock_gettime(CLOCK_REALTIME, &start);

    cv::Mat hsv, mask, lower_red, upper_red;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    cv::inRange(hsv, cv::Scalar(0, 70, 50), cv::Scalar(10, 255, 255), lower_red);
    cv::inRange(hsv, cv::Scalar(170, 70, 50), cv::Scalar(180, 255, 255), upper_red);
     mask = lower_red | upper_red;

 //   cv::erode(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
  //  cv::dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        if (cv::contourArea(contour) < 50) continue;
        cv::Moments m = cv::moments(contour);
        if (m.m00 != 0) {
            int cx = static_cast<int>(m.m10 / m.m00);
            int cy = static_cast<int>(m.m01 / m.m00);
            cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(0, 255, 0), -1);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    delta_t(&end, &start, &exec);
    double run_time = (exec.tv_sec * 1000.0) + (exec.tv_nsec / 1000000.0);
    std::cout << "Red laser detection runtime: " << run_time << " ms" << std::endl;
    // Uncomment for visual output
    // cv::imshow("Red Laser Detection", frame);
    // cv::waitKey(1);
}

void signal_handler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Stopping..." << std::endl;
    stop_requested = true;
}

int main() {
    signal(SIGINT, signal_handler);
    std::cout << "Initializing camera..." << std::endl;

    if (init_camera() != EXIT_SUCCESS) {
        std::cerr << "Camera setup failed. Exiting!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Camera initialized. Starting capture loop." << std::endl;

    while (!stop_requested) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(cam.fd, &fds);
        timeval tv = {0, 100000};  // 100ms timeout
        int r = select(cam.fd + 1, &fds, NULL, NULL, &tv);
        if (r == -1) continue;
        if (r == 0) continue;

        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(cam.fd, VIDIOC_DQBUF, &buf) == -1) {
            if (errno == EAGAIN) continue;
            perror("Retrieving frame");
            continue;
        }

        cv::Mat yuyv(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC2, cam.buffers[buf.index].start);
        cv::Mat bgr;
        cv::cvtColor(yuyv, bgr, cv::COLOR_YUV2BGR_YUYV);

        detect_red_laser(bgr);

        if (ioctl(cam.fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Requeueing buffer");
            break;
        }
    }

    std::cout << "Exiting..." << std::endl;
    return 0;
}
