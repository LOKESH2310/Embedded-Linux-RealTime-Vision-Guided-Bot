#define CONFIG_FILE "Config.json"
#include <nlohmann/json.hpp>
#include <sys/stat.h> 
#include <opencv2/opencv.hpp>
#include "red_laser_service.hpp"
#include <fstream>

extern HSVConfig config; 
extern 	std::mutex config_mutex;

void config_update_service();
