#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() { 
  string line, key, value, memoryTotal, memoryFree;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "MemTotal:") {
          memoryTotal = value;
        }
        if (key == "MemFree:") {
          memoryFree = value;
        }
      }
    }
  }
  return (stof(memoryTotal) - stof(memoryFree)) / stof(memoryTotal);
}

long LinuxParser::UpTime() { 
   string line, uptimeString;
   std::ifstream stream(kProcDirectory + kUptimeFilename);
   if (stream.is_open()) {
     std::getline(stream, line);
     std::istringstream uptimeFromStream(line);
     uptimeFromStream >> uptimeString;
   }
  return std::stol(uptimeString); 
}

long LinuxParser::Jiffies() { 
  vector<string> values = LinuxParser::CpuUtilization();
  vector<long> valuesLong(10, 0);
  long total = 0;
  vector<CPUStates> states = {kUser_, kNice_, kSystem_, kIdle_, kIOwait_, kIRQ_, kSoftIRQ_, kSteal_};
  for (int state: states) {
    valuesLong[state] = stol(values[state]);
    total += valuesLong[state];
  };
  return total;
}

long LinuxParser::ActiveJiffies(int pid) {
  long totaltime;
  string line, value;
  vector<string> values;
  std::ifstream stream(kProcDirectory + std::to_string(pid) + kStatFilename);
  long utime = 0, stime = 0, cutime = 0, cstime = 0;
  
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) {
      values.push_back(value);
    }

    if (std::all_of(values[13].begin(), values[13].end(), isdigit))
      utime = stol(values[13]);
    if (std::all_of(values[14].begin(), values[14].end(), isdigit))
      stime = stol(values[14]);
    if (std::all_of(values[15].begin(), values[15].end(), isdigit))
      cutime = stol(values[15]);
    if (std::all_of(values[16].begin(), values[16].end(), isdigit))
      cstime = stol(values[16]);
  }
  
  totaltime = utime + stime + cutime + cstime;
  return totaltime ;
}

long LinuxParser::ActiveJiffies() { 
  vector<string> jiffies = CpuUtilization();
  return stol(jiffies[CPUStates::kUser_]) + stol(jiffies[CPUStates::kNice_]) + stol(jiffies[CPUStates::kSystem_]) + stol(jiffies[CPUStates::kIRQ_]) + stol(jiffies[CPUStates::kSoftIRQ_]) + stol(jiffies[CPUStates::kSteal_]);
}

long LinuxParser::IdleJiffies() { 
  auto jiffies = CpuUtilization();
  return stol(jiffies[CPUStates::kIdle_]) + stol(jiffies[CPUStates::kIOwait_]);
}

vector<string> LinuxParser::CpuUtilization() { 
  string line, cpu, value;
  vector<string> values;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream lineFromStream(line);
    lineFromStream >> cpu;
    while (lineFromStream >> value) {
      values.push_back(value);
    };
  }
  return values;
 }

int LinuxParser::TotalProcesses() { 
  string line, key, value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream lineFromStream(line);
      while (lineFromStream >> key >> value) {
        if (key == "processes") { return stoi(value); }
      }
    }
  }
  return 0;
 }

int LinuxParser::RunningProcesses() { 
  string line, key, value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream lineFromStream(line);
      while (lineFromStream >> key >> value) {
        if (key == "procs_running") { return stoi(value); }
      }
    }
  }
  return 0;
}

string LinuxParser::Command(int pid) { 
  string command;
  std::ifstream stream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (stream.is_open()) {
    std::getline(stream, command);
  }
  return command;
}

string LinuxParser::Ram(int pid) { 
  string line, key, value;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key) {
        if (key == "VmSize:") {
          linestream >> value;
          return to_string(stol(value) / 1024);
        }
      }
     }
  }
  return "0";
}

string LinuxParser::Uid(int pid) { 
  string line, key, uid;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key;
      if (key == "Uid:") {
        linestream >> uid;
        break;
      }
    }
  }

  return uid;
}

string LinuxParser::User(int pid) { 
  string line, name, x, uid;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> name >> x >> uid) {
        if (uid == LinuxParser::Uid(pid)) {
          return name;
        }
      }
    }
  }
  return "default ";
}

long LinuxParser::UpTime(int pid) { 
  string line, value;
  vector<string> values;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) {
      values.push_back(value);
    };
  }
  return LinuxParser::UpTime() - (stol(values[21]) / sysconf(_SC_CLK_TCK));
}
