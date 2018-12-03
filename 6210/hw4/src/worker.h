#pragma once

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <mr_task_factory.h>
#include <uuid/uuid.h>
#include <fstream>
#include <iostream>

#include "masterworker.grpc.pb.h"
#include "mr_tasks.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using masterworker::MapReduceMasterWorker;
using masterworker::ShardInfo;
using masterworker::MapperReply;
using masterworker::TempFileInfo;
using masterworker::ReducerReply;

extern std::shared_ptr<BaseReducer> get_reducer_from_task_factory(const std::string& user_id);
extern std::shared_ptr<BaseMapper> get_mapper_from_task_factory(const std::string& user_id);

class Worker
{
public:
  Worker(std::string ip_addr_port);
  bool run();
  void set_mapper_id(BaseMapper* mapper);
  void set_reducer_id(BaseReducer* reducer);
  std::unordered_set<std::string>& get_temp_file_names(BaseMapper* mapper);
  std::unordered_set<std::string>& get_output_file_names(BaseReducer* reducer);

private:
  std::string ip_addr_port_;

  std::string new_guid()
  {
    uuid_t guid;
    uuid_generate(guid);

    char *guid_str = new char[100];
    uuid_unparse(guid, guid_str);

    return std::string(guid_str);
  }
};

class WorkerImpl final : public MapReduceMasterWorker::Service
{
private:
  Worker* worker_;

public:
  WorkerImpl(Worker* worker)
  {
    worker_ = worker;
  }

  Status map(ServerContext* context, const ShardInfo* shard_info, MapperReply* mapper_reply) override
  {
    auto mapper = get_mapper_from_task_factory(shard_info->user_id());
    worker_->set_mapper_id(mapper);

    auto& file_names = shard_info->file_names();
    auto& start_offsets = shard_info->start_offsets();
    auto& end_offsets = shard_info->end_offsets();

    for (size_t i = 0 ; i < file_names.size() ; ++i)
    {
      std::ifstream myfile(file_names[i], std::ios::binary);
      if (myfile.is_open())
      {
        myfile.seekg(start_offsets[i]);
        std::string line;
        while (getline(myfile, line))
        {
          mapper->map(line);
          if (end_offsets[i] == myfile.tellg())
          {
            break;
          }
        }

        myfile.close();
      }
      else
      {
        std::cerr << "Failed to open file " << file_names[i] << std::endl;
        exit(-1);
      }
    }

    for (auto& temp_file_name : worker_->get_temp_file_names(mapper))
    {
      mapper_reply->set_file_names(0, temp_file_name);
    }

    return Status::OK;
  }

  Status reduce(ServerContext* context, const TempFileInfo* temp_file_info, ReducerReply* reducer_reply) override
  {
    auto reducer = get_reducer_from_task_factory(temp_file_info->user_id());
    worker_->set_reducer_id(reducer);

    std::map<std::string, std::vector<std::string> > kv_store;
    for (auto& file_name : temp_file_info->file_names())
    {
      std::ifstream myfile(file_name, std::ios::binary);

      if (myfile.is_open())
      {
        std::string key;
        while (getline(myfile, key, ' '))
        {
          std::string value;
          getline(myfile, value);

          kv_store[key].push_back(value);
        }

        myfile.close();
      }
      else
      {
        std::cerr << "Failed to open file " << file_name << std::endl;
        exit(-1);
      }
    }

    for (auto& kv : kv_store)
    {
      reducer->reduce(kv.first, kv.second);
    }

    for (auto& output_file_name : worker_->get_output_file_names(reducer))
    {
      reducer_reply->set_file_names(0, output_file_name);
    }

    return Status::OK;
  }
};

Worker::Worker(std::string ip_addr_port)
{
  ip_addr_port_ = ip_addr_port;
}

bool Worker::run()
{
  WorkerImpl service(this);
  ServerBuilder builder;
  builder.AddListeningPort(ip_addr_port_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << ip_addr_port_ << std::endl;
  server->Wait();

  return true;
}

void Worker::set_mapper_id(BaseMapper* mapper)
{
  mapper->impl_->mapper_id = new_guid();
}

void Worker::set_reducer_id(BaseReducer* reducer)
{
  reducer->impl_->reducer_id = new_guid();
}

std::unordered_set<std::string>& Worker::get_temp_file_names(BaseMapper* mapper)
{
  return mapper->impl_->temp_file_names;
}

std::unordered_set<std::string>& Worker::get_output_file_names(BaseReducer* reducer)
{
  return reducer->impl_->output_file_names;
}
