#include "config_update_service.hpp"

void load_config(const std::string& filename)
{
  std::ifstream file(filename);//input file stream
  nlohmann::json json_instance;
  
  file>>json_instance;
  
  auto colour=json_instance.at("colour");
  auto l1 = colour.at("lower");
  auto u1 = colour.at("upper");
  auto b=colour.at("behaviour");

  HSVConfig new_config;
    new_config.hue_min = l1[0];
    new_config.sat_min = l1[1];
    new_config.val_min = l1[2];
    new_config.hue_max = u1[0];
    new_config.sat_max = u1[1];
    new_config.val_max = u1[2];
    new_config.behaviour=b;
    syslog(LOG_INFO,"loading new config");     
   std::lock_guard<std::mutex> lock(config_mutex);
   config = new_config;
}

void config_update_service()
{ 	//keep  a track of the last modified time
	static std::time_t last_mod_time = 0;
    struct stat file_stat;
     if (stat(CONFIG_FILE, &file_stat) == 0) {
        if (file_stat.st_mtime != last_mod_time) {
            last_mod_time = file_stat.st_mtime;
            load_config(CONFIG_FILE);
        }
}
}
