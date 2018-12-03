#pragma once

#include <assert.h>
#include <string.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

struct MapReduceSpec
{
  size_t n_workers;
  size_t n_output_files;
  size_t map_kilobytes;
  std::string output_dir;
  std::string user_id;
  std::vector<std::string> worker_ipaddr_ports;
  std::vector<std::string> input_files;
};

inline std::string trim(const string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

inline std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);

   while (std::getline(tokenStream, token, delimiter))
   {
    token = trim(token);
    tokens.push_back(token);
   }

   return tokens;
}

inline bool read_mr_spec_from_config_file(const std::string& config_filename, MapReduceSpec& mr_spec)
{
  std::unordered_map<std::string, std::vector<std::string> > map;
  std::ifstream config_file(config_filename);
  if (config_file.is_open())
  {
    std::string spec;
    while (getline(config_file, spec, '='))
    {
      spec = trim(spec);
      std::string value;
      getline(config_file, value);
      value = trim(value);
      if (spec.compare("n_workers") == 0)
      {
        mr_spec.n_workers = atoi(value.c_str());
      }
      else if (spec.compare("n_output_files") == 0)
      {
        mr_spec.n_output_files = atoi(value.c_str());
      }
      else if (spec.compare("map_kilobytes") == 0)
      {
        mr_spec.map_kilobytes = atoi(value.c_str());
      }
      else if (spec.compare("output_dir") == 0)
      {
        mr_spec.output_dir = value;
      }
      else if (spec.compare("user_id") == 0)
      {
        mr_spec.user_id = value;
      }
      else if (spec.compare("worker_ipaddr_ports") == 0)
      {
        mr_spec.worker_ipaddr_ports = split(value, ',');
      }
      else if (spec.compare("input_files") == 0)
      {
        mr_spec.input_files = split(value, ',');
      }
    }

    config_file.close();
  }
  else
  {
    std::cerr << "Failed to open file " << config_filename << std::endl;
    return false;
  }

  return true;
}

inline bool validate_mr_spec(const MapReduceSpec& mr_spec)
{
  assert(mr_spec.n_workers > 0 && mr_spec.n_workers == mr_spec.worker_ipaddr_ports.size());
  assert(mr_spec.n_output_files > 0 && mr_spec.map_kilobytes > 0);
  assert(mr_spec.output_dir.c_str() != nullptr);
  assert(mr_spec.user_id.c_str() != nullptr);

  for (auto& input_file : mr_spec.input_files)
  {
    std::ifstream file(input_file);
    assert(file.is_open());
    file.close();
  }

  for (auto& worker_ipaddr_port : mr_spec.worker_ipaddr_ports)
  {
    std::vector<std::string> splits = split(worker_ipaddr_port, ':');
    assert(strncmp(splits[0].c_str(), "localhost", 9) == 0);
    assert(atoi(splits[1].c_str()) <= 65535);
  }

  return true;
}
