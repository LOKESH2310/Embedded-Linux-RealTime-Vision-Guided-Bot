/*
 * This is a C++ version of the canonical pthread service example. It intends
 * to abstract the service management functionality and sequencing for ease
 * of use. Much of the code is left to be implemented by the student.
 *
 * Build with g++ --std=c++23 -Wall -Werror -pedantic
 * Steve Rizor 3/16/2025
 * 
 * Code Modified by- Nalin Saxena
 * 
 * This file contains code written for ECEN 5623 ex-5 question 4
 * 
 * Various methods for accessing gpios are explored and compared.
 */

 #pragma once

 #include <cstdint>
 #include <functional>
 #include <thread>
 #include <iostream>
 #include <csignal>
 #include <ctime>
 #include <vector>
 #include <chrono>  
 #include <fstream>  
 #include <syslog.h>
 #define NSEC_PER_SEC (1000000000)
 
 // The service class contains the service function and service parameters
 // (priority, affinity, etc). It spawns a thread to run the service, configures
 // the thread as required, and executes the service whenever it gets released.
 class Service
 {
 public:
     template <typename T>
     Service(T &&doService, uint8_t affinity, uint8_t priority, int period, int service_indetifier) : _doService(doService),
                                                                                                           _affinity(affinity),
                                                                                                           _priority(priority),
                                                                                                           _period(period),
                                                                                                           _service_indetifier(service_indetifier),
                                                                                                           _semaphore(0)
     {
         // todo: store service configuration values
         // todo: initialize release semaphore
 
         // Start the service thread, which will begin running the given function immediately
         syslog(LOG_INFO, "constructor called for service %d \n", service_indetifier);
         syslog(LOG_INFO, "affinity %d \n", static_cast<int>(_affinity));
         syslog(LOG_INFO, "priority  %d \n", static_cast<int>(_priority));
         syslog(LOG_INFO, "period %d   \n", static_cast<int>(_period));
           std::ofstream clear_file("service_runs" + std::to_string(_service_indetifier) + "_.csv"); //clear previous logs
         _service = std::jthread(&Service::_provideService, this);
     }
 
     void stop()
     {
         // todo: change state to "not running" using an atomic variable
         // (heads up: what if the service is waiting on the semaphore when this happens?)
         _running = false;
         _semaphore.release();
     }
 
     void release()
     {
         // todo: release the service using the semaphore
         _semaphore.release();
     }
 
     int get_period()
     {
         return _period;
     }
 
     void logStats()
     {
 auto avg_time = _accum_exec_time / _exec_count;
 syslog(LOG_INFO, "Service Execution Stats: For %d", _service_indetifier);
 syslog(LOG_INFO, "  Number of Executions  : %ld", _exec_count);
 syslog(LOG_INFO, "  Min Execution Time    : %.3f ms (%.0f ns)", _min_execution_time, _min_execution_time * 1e6);
 syslog(LOG_INFO, "  Max Execution Time    : %.3f ms (%.0f ns)", _max_execution_time, _max_execution_time * 1e6);
 syslog(LOG_INFO, "  Avg Execution Time    : %.3f ms (%.0f ns)", avg_time, avg_time * 1e6);
 syslog(LOG_INFO, "  Jitter  : %.3f ms (%.0f ns)", _max_execution_time-_min_execution_time, (_max_execution_time-_min_execution_time) * 1e6);
 
     }
 
 private:
     std::function<void(void)> _doService;
     std::jthread _service;
     int _affinity;
     int _priority;
     int _period;
     int _service_indetifier;
     std::binary_semaphore _semaphore;
     std::atomic<bool> _running{true}; // Atomic boolean initialized to true
                                       
                                       // logging data
     double _min_execution_time = std::numeric_limits<double>::max();
     double _max_execution_time = std::numeric_limits<double>::min();
     double _accum_exec_time = 0;
     uint64_t _exec_count = 0;
 
 
     void _initializeService()
     {
         // todo: set affinity, priority, sched policy
         // (heads up: the thread is already running and we're in its context right now)
         syslog(LOG_INFO, "Initializing service...");
         pthread_t threadID = pthread_self(); // Get current thread ID
         cpu_set_t cpuset;
         struct sched_param param;
         CPU_ZERO(&cpuset);
         CPU_SET(_affinity, &cpuset);
         if (pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset) != 0)
         {
             syslog(LOG_ERR, "Failed to set CPU affinity for thread.");
         }
         int policy = SCHED_FIFO; // FIFO policy (use SCHED_OTHER for normal priority)
         param.sched_priority = _priority;
 
         if (pthread_setschedparam(threadID, policy, &param) != 0)
         {
             syslog(LOG_ERR, "Failed to set scheduling parameters.");
         }
 
         syslog(LOG_INFO, "Service initialized with affinity %d and priority %d", _affinity, _priority);
     }
     
     void _runAndLog()
 {
     struct timespec service_start = {0, 0};
     struct timespec service_end = {0, 0};
     struct timespec service_exec = {0, 0};
 
     clock_gettime(CLOCK_REALTIME, &service_start);
     _doService();
     clock_gettime(CLOCK_REALTIME, &service_end);
 
     delta_t(&service_end, &service_start, &service_exec);
     double run_time = (service_exec.tv_sec * 1000.0) + (service_exec.tv_nsec / 1000000.0);
 
     std::ofstream run_time_logs;
     run_time_logs.open("service_runs" + std::to_string(_service_indetifier) + "_.csv", std::ios::app);
     if (run_time_logs.is_open())
     {
         run_time_logs << run_time << "\n";
         run_time_logs.close();
     }
 
     _exec_count++;
     _min_execution_time = std::min(_min_execution_time, run_time);
     _max_execution_time = std::max(_max_execution_time, run_time);
     _accum_exec_time += run_time;
 }
 
 
 void _provideService()
 {
     _initializeService();
 
     while (_running)
     {
         if (_period < 0)
         {
             _runAndLog();
         }
         else
         {
             _semaphore.acquire();
             _runAndLog();
         }
     }
 }
 
     int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
     {
         int dt_sec = stop->tv_sec - start->tv_sec;
         int dt_nsec = stop->tv_nsec - start->tv_nsec;
 
         if (dt_sec >= 0)
         {
             if (dt_nsec >= 0)
             {
                 delta_t->tv_sec = dt_sec;
                 delta_t->tv_nsec = dt_nsec;
             }
             else
             {
                 delta_t->tv_sec = dt_sec - 1;
                 delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
             }
         }
         else
         {
             if (dt_nsec >= 0)
             {
                 delta_t->tv_sec = dt_sec;
                 delta_t->tv_nsec = dt_nsec;
             }
             else
             {
                 delta_t->tv_sec = dt_sec - 1;
                 delta_t->tv_nsec = NSEC_PER_SEC + dt_nsec;
             }
         }
 
         return (1);
     }
 };
 
 
 
 
 class Sequencer
 {
 public:
     template <typename... Args>
     void addService(Args &&...args)
     {
         // Add the new service to the services list,
         // constructing it in-place with the given args
         _services.emplace_back(std::make_unique<Service>(std::forward<Args>(args)...));
     }
 
     void startServices()
     {
         // todo: start timer(s), release services
         _seq_running = true;
         _chrono_timer_thread = std::jthread(&Sequencer::chrono_timer_loop, this);
         
          pthread_t threadID = _chrono_timer_thread.native_handle();
          cpu_set_t cpuset;
          CPU_ZERO(&cpuset);
          CPU_SET(1, &cpuset); // Core 1
          if (pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset) != 0)
          {
          syslog(LOG_ERR, "Failed to set CPU affinity for sequencer thread.");
          }
     struct sched_param param;
     param.sched_priority = 99;
    if (pthread_setschedparam(threadID, SCHED_FIFO, &param) != 0)
    {
        syslog(LOG_ERR, "Failed to set scheduling parameters for sequencer thread use root priviliges.");
        exit(1);
    }
       
}
 
     void stopServices()
     {
         // todo: stop timer(s), stop services
         _seq_running = false;
         for (auto &service : _services)
         {
             service->stop();
             service->logStats();
         }
     }
 
 private:
     std::vector<std::unique_ptr<Service>> _services; // Store services as unique_ptrs  (semaphore and atmoic variables are non movable)
     std::atomic<bool> _seq_running{false};
     std::atomic<uint64_t> _tick_counter{0};
     std::jthread _chrono_timer_thread; //create a chrono for timer thread
     
     void chrono_timer_loop()
{
    std::chrono::steady_clock::time_point next_wakeup = std::chrono::steady_clock::now();

    while (_seq_running)
    {
        next_wakeup += std::chrono::milliseconds(1);
        std::this_thread::sleep_until(next_wakeup);

        uint64_t tick_time = ++_tick_counter;
        for (auto &service : _services)
        {
            if (tick_time % service->get_period() == 0)
            {
                service->release();
            }
        }
    }
}


 };
 
