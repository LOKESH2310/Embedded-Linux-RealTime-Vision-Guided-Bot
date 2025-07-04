# Makefile for building the sequencer application and other sources.

# Compiler and flags
CXX = g++
CXXFLAGS = --std=c++23 -Wall

# pigpio needs librt (for gpioDelay) and the pigpio library itself
PIGPIO_LDFLAGS = -lpigpio -lrt

# OpenCV and pthread flags
OPENCV_FLAGS = `pkg-config --cflags --libs opencv4` $(PIGPIO_LDFLAGS) # Uses pkg-config for OpenCV 4
PTHREAD_FLAGS = -pthread

# Target executable
TARGET = rtes_cat_bot

# Source files (add your .cpp files here)
SRCS = main_cat.cpp red_laser_service.cpp cameraService.cpp config_update_service.cpp direction_deciding.cpp motor_control.cpp watchdog.cpp

# Object files (replace .cpp with .o)
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link object files into the final executable with OpenCV and pthread libraries
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OPENCV_FLAGS) $(PTHREAD_FLAGS)

# Compile .cpp to .o (include OpenCV flags!)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPENCV_FLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Useful phony targets
.PHONY: all clean
