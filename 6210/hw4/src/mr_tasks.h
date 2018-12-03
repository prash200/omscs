#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>

struct BaseMapperInternal
{
  BaseMapperInternal();

  void emit(const std::string& key, const std::string& val);

  std::unordered_set<std::string> temp_file_names;
  std::string mapper_id;
};

inline BaseMapperInternal::BaseMapperInternal() {}

inline void BaseMapperInternal::emit(const std::string& key, const std::string& val)
{
  std::string file_name = "output/mapper/" + mapper_id + "/" + key + ".txt";
  std::ofstream myfile(file_name, std::ios::app);
  if (myfile.is_open())
  {
    myfile << key << " " << val << std::endl;
    myfile.close();
  }
  else
  {
    std::cerr << "Failed to open file " << file_name << std::endl;
    exit(-1);
  }

  temp_file_names.insert(file_name);
}

struct BaseReducerInternal
{
  BaseReducerInternal();

  void emit(const std::string& key, const std::string& val);

  std::unordered_set<std::string> output_file_names;
  std::string reducer_id;
};

inline BaseReducerInternal::BaseReducerInternal() {}

inline void BaseReducerInternal::emit(const std::string& key, const std::string& val)
{
  std::string file_name = "output/reducer/" + reducer_id + "/" + key + ".txt";

  std::ofstream myfile(file_name, std::ios::app);
  if (myfile.is_open())
  {
    myfile << key << " " << val << std::endl;
    myfile.close();
  }
  else
  {
    std::cerr << "Failed to open file " << file_name << std::endl;
    exit(-1);
  }

  output_file_names.insert(file_name);
}
