#include "cameraService.hpp"
#include "watchdog.hpp"
cv::Mat latest_frame;
std::mutex frame_mutex;

//a global context for camera
CameraContext cam;


int init_camera()
{
//open the device in a non blocking mode
  const char* dev_name = CAM_DEVICE;
     cam.fd = open(dev_name, O_RDWR | O_NONBLOCK);
    if (cam.fd == -1) {
        syslog(LOG_ERR,"ERROR Opening video device");
        return EXIT_FAILURE;
    }
    
    //set format
    cam.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam.fmt.fmt.pix.width = FRAME_WIDTH;
    cam.fmt.fmt.pix.height = FRAME_HEIGHT;
    cam.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    cam.fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(cam.fd, VIDIOC_S_FMT, &cam.fmt) == -1) {
        syslog(LOG_ERR,"ERROR Setting Pixel Format");
         return EXIT_FAILURE;
    }
    
    //request for buffer to store our frames
    v4l2_requestbuffers req = {};
    req.count = NBUF;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam.fd, VIDIOC_REQBUFS, &req) == -1) {
        syslog(LOG_ERR,"ERROR Requesting Buffer");
        return EXIT_FAILURE;
    }
    
    //query and queue buffers
    for (int i = 0; i < NBUF; ++i) {
        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(cam.fd, VIDIOC_QUERYBUF, &buf) == -1) {
            syslog(LOG_ERR,"Querying Buffer failed");
            return EXIT_FAILURE;
        }

        cam.buffers[i].length = buf.length;
        cam.buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam.fd, buf.m.offset);
        
        if (cam.buffers[i].start == MAP_FAILED) {
            syslog(LOG_ERR,"mmap failed");
          return EXIT_FAILURE;
        }

        if (ioctl(cam.fd, VIDIOC_QBUF, &buf) == -1) {
            syslog(LOG_ERR,"Queue Buffer failed");
            return EXIT_FAILURE;
        }
    }
    // Start streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(cam.fd, VIDIOC_STREAMON, &type) == -1) {
        syslog(LOG_ERR,"error starting streaming");
         return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}	


void camera_capture_service() {
    fd_set fds;
    //int r;
 
        FD_ZERO(&fds);
        FD_SET(cam.fd, &fds);


        v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(cam.fd, VIDIOC_DQBUF, &buf) == -1) {
            if (errno == EAGAIN) return;
            syslog(LOG_ERR,"No frame data available service returning early");
            return;
        }

        cv::Mat yuyv(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC2, cam.buffers[buf.index].start);
        cv::Mat bgr;
        cv::cvtColor(yuyv, bgr, cv::COLOR_YUV2BGR_YUYV);

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = bgr.clone();
        }
        if (ioctl(cam.fd, VIDIOC_QBUF, &buf) == -1) {
			syslog(LOG_ERR,"error requeing buffer");
            return;

        }
        service1_ok = true;
        //syslog(LOG_INFO, "Service 1 OK set");
    
}
