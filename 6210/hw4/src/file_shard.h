#pragma once

#include <climits>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <cmath>
#include <vector>
#include "mapreduce_spec.h"

#define FILE_NAME_MAX_LEN 100

struct FileShard 
{
  std::vector<std::string> file_names;
  std::vector<long> start_offsets;
  std::vector<long> end_offsets;
};

inline uint64_t get_input_size(const std::string& file_name)
{
  std::ifstream file(file_name, std::ios::binary);
  file.seekg(0, std::ios::beg);
  std::streampos begin = file.tellg();
  file.seekg(0, std::ios::end);
  std::streampos end = file.tellg();

  return (end - begin + 1);
}

inline uint64_t get_total_size(const MapReduceSpec& mr_spec)
{
  uint64_t total_size = 0;
  for (auto& input_file : mr_spec.input_files)
  {
    total_size += get_input_size(input_file);
  }

  return total_size;
}

inline bool shard_files(const MapReduceSpec& mr_spec, std::vector<FileShard>& file_shards)
{
  uint64_t total_size = get_total_size(mr_spec);
  size_t n_shards = std::ceil(total_size / (mr_spec.map_kilobytes * 1024.0));
  std::cout << n_shards << std::endl;

  for (size_t curr_shard_num = 0, input_file_index = 0, input_file_offset = 0 ; curr_shard_num < n_shards ; ++curr_shard_num)
  {
    size_t curr_shard_size = 0;
    std::cout << curr_shard_size << std::endl;
    while (curr_shard_size < mr_spec.map_kilobytes * 1024)
    {
      std::cout << curr_shard_size << std::endl;
      std::string input_file = mr_spec.input_files[input_file_index];
      uint64_t input_file_size = get_input_size(input_file);

      std::cout << input_file_size << std::endl;
      if (input_file_size - input_file_offset >= mr_spec.map_kilobytes * 1024 - curr_shard_size)
      {
        std::ifstream myfile(input_file, std::ios::binary);

        myfile.seekg(input_file_offset, std::ios::beg);
        std::streampos begin = myfile.tellg();

        myfile.seekg(mr_spec.map_kilobytes * 1024 - curr_shard_size, std::ios::cur);
        myfile.ignore(LONG_MAX, '\n');
        std::streampos end = myfile.tellg();

        if (end >= input_file_size)
        {
          ++input_file_index;
          input_file_offset = 0;
        }
        else
        {
          input_file_offset = end - end + end + 1;
        }

        curr_shard_size += end - begin + 1;

        FileShard file_shard;
        if (file_shards.size() <= curr_shard_num)
        {
          file_shards.push_back(file_shard);
        }
        else
        {
          file_shard = file_shards[curr_shard_num];
        }

        file_shard.file_names.push_back(input_file);
        file_shard.start_offsets.push_back(begin);
        file_shard.end_offsets.push_back(end);
      }
      else
      {
        std::ifstream myfile(input_file, std::ios::binary);

        myfile.seekg(input_file_offset, std::ios::beg);
        std::streampos begin = myfile.tellg();

        myfile.seekg(0, std::ios::end);
        std::streampos end = myfile.tellg();

        ++input_file_index;
        input_file_offset = 0;

        curr_shard_size += end - begin + 1;

        FileShard file_shard;
        if (file_shards.size() <= curr_shard_num)
        {
          file_shards.push_back(file_shard);
        }
        else
        {
          file_shard = file_shards[curr_shard_num];
        }

        file_shard.file_names.push_back(input_file);
        file_shard.start_offsets.push_back(begin);
        file_shard.end_offsets.push_back(end);
      }
    }
  }
}
