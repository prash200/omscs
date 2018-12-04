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

struct FileShard {
  std::unordered_map<std::string, std::pair<std::streampos, std::streampos>>
      shardsMap;
};

/// return total input files size in bytes
inline uint64_t get_totoal_size(const MapReduceSpec& mr_spec) {
  uint64_t totalSize = 0;
  for (auto& input : mr_spec.input_files) {
    std::ifstream myfile(input, std::ios::binary);
    myfile.seekg(0, std::ios::beg);
    std::streampos begin = myfile.tellg();
    myfile.seekg(0, std::ios::end);
    std::streampos end = myfile.tellg();
    totalSize += (end - begin + 1);
    myfile.close();
  }
  return totalSize;
}

/// return input file size in bytes
inline size_t get_input_size(std::ifstream& myfile) {
  myfile.seekg(0, std::ios::beg);
  std::streampos begin = myfile.tellg();

  myfile.seekg(0, std::ios::end);
  std::streampos end = myfile.tellg();
  return (end - begin + 1);
}

/* CS6210_TASK: Create fileshards from the list of input files, map_kilobytes
 * etc. using mr_spec you populated  */
inline bool shard_files(const MapReduceSpec& mr_spec,
                        std::vector<FileShard>& fileShards) {
  uint64_t totalSize = get_totoal_size(mr_spec);
  size_t shardNums = std::ceil(totalSize / (mr_spec.map_kilobytes * 1024.0)) + 1;
  fileShards.reserve(shardNums);

  for (auto& input : mr_spec.input_files) {
    std::ifstream myfile(input, std::ios::binary);
    uint64_t fileSize = get_input_size(myfile);

    std::cout << "\nSplit file : " << input << " " << fileSize
              << " Bytes into shards ...\n";
    std::streampos offset = 0;
    uint64_t restSize = fileSize;
    while (restSize > 0) {
      // find offset begin for a shard
      myfile.seekg(offset, std::ios::beg);
      std::streampos begin = myfile.tellg();

      // find offset end for a shard
      myfile.seekg(mr_spec.map_kilobytes * 1024, std::ios::cur);
      std::streampos end = myfile.tellg();

      // if offset exceed size, set its end position
      if (end >= fileSize) {
        myfile.seekg(0, std::ios::end);
      } else {
        // find closest '\n' delimit
        myfile.ignore(LONG_MAX, '\n');
      }
      end = myfile.tellg();

      size_t chunkSize = (end - begin + 1);
      std::cout << "Process offset (" << begin << "," << end << ") "
                << chunkSize << " bytes into shard ...\n";

      // store chunk into shards
      FileShard temp;
      temp.shardsMap[input] = make_pair(begin, end);
      fileShards.push_back(std::move(temp));

      restSize -= chunkSize;
      offset = static_cast<int>(end) + 1;
    }
    myfile.close();
  }

  return true;
}