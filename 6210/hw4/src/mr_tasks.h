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
  void set_mapper_id(std::string mapper_id);

  std::unordered_set<std::string> temp_file_names_;
  std::string mapper_id_;
};

inline BaseMapperInternal::BaseMapperInternal() {}

inline void BaseMapperInternal::emit(const std::string& key, const std::string& val)
{
  std::string file_name = "output/mapper/" + mapper_id_ + "/" + key + ".txt";
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

  temp_file_names_.insert(file_name);
}

inline void BaseMapperInternal::set_mapper_id(std::string mapper_id)
{
  mapper_id_ = mapper_id;
}

struct BaseReducerInternal
{
  BaseReducerInternal();

  void emit(const std::string& key, const std::string& val);
  void set_reducer_id(std::string reducer_id);

  std::unordered_set<std::string> output_file_names_;
  std::string reducer_id_;
};

inline BaseReducerInternal::BaseReducerInternal() {}

inline void BaseReducerInternal::emit(const std::string& key, const std::string& val)
{
  std::string file_name = "output/reducer/" + reducer_id_ + "/" + key + ".txt";

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

  output_file_names_.insert(file_name);
}

inline void BaseReducerInternal::set_reducer_id(std::string reducer_id)
{
  reducer_id_ = reducer_id;
}
