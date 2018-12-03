#pragma once

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <unistd.h>
#include <unordered_map>
#include <thread>
#include "file_shard.h"
#include "mapreduce_spec.h"
#include "masterworker.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using masterworker::MapReduceMasterWorker;
using masterworker::ShardInfo;
using masterworker::MapperReply;
using masterworker::TempFileInfo;
using masterworker::ReducerReply;

enum WORKER_STATUS
{
  AVAILABLE,
  BUSY
};

class Master
{
public:
  Master(const MapReduceSpec&, const std::vector<FileShard>&);
  bool run();
  void update_worker_status(const std::string& ip_addr_port, WORKER_STATUS status);
  void update_temp_file_name_map(const std::vector<std::string>& temp_file_names);
  void update_output_files(const std::vector<std::string>& output_file_names);
  void increment_completion_count();

private:
  void run_all_map_tasks();
  void run_all_reduce_tasks();
  std::string get_idle_worker();

  MapReduceSpec mr_spec_;
  std::vector<FileShard> file_shards_;
  std::unordered_map<std::string, WORKER_STATUS> worker_status_;
  std::unordered_map<std::string, std::vector<std::string> > temp_file_name_map_;
  std::vector<std::string> output_file_names_;
  size_t completion_count_;
  bool is_worker_idle_;

  std::mutex m_;
  std::condition_variable cv_;
};

class MasterImpl
{
public:
  explicit MasterImpl(std::string ip_addr_port, Master* master)
  {
    ip_addr_port_ = ip_addr_port;
    master_ = master;
    stub_ = MapReduceMasterWorker::NewStub(grpc::CreateChannel(ip_addr_port, grpc::InsecureChannelCredentials()));
  }

  void map(const std::string& user_id, const FileShard& file_shard)
  {
    std::vector<std::string> file_names;
    std::vector<std::string> start_offsets;
    std::vector<std::string> end_offsets;
    for(auto kv : file_shard.shards)
    {
      file_names.push_back(kv.first);
      start_offsets.push_back(kv.second.first);
      end_offsets.push_back(kv.second.second);
    }

    ShardInfo shard_info;
    shard_info.set_file_names(file_names);
    shard_info.set_start_offsets(start_offsets);
    shard_info.set_end_offsets(end_offsets);
    shard_info.set_user_id(user_id);

    AsyncClientCall* call = new AsyncClientCall;

    call->mapper_response_reader = stub_->Asyncmap(&call->context, shard_info, &cq_);

    call->mapper_response_reader->StartCall();

    call->mapper_response_reader->Finish(&call->mapper_reply, &call->mapper_status, (void*)call);
  }

  void reduce(const std::string& user_id, const std::vector<std::string>& temp_file_names)
  {
    TempFileInfo temp_file_info;
    temp_file_info.set_file_names(temp_file_names);
    temp_file_info.set_user_id(user_id);

    AsyncClientCall* call = new AsyncClientCall;

    call->reducer_response_reader = stub_->Asyncreduce(&call->context, temp_file_info, &cq_);

    call->reducer_response_reader->StartCall();

    call->reducer_response_reader->Finish(&call->reducer_reply, &call->reducer_status, (void*)call);
  }

  void async_client_call_complete()
  {
    void* got_tag;
    bool ok = false;
    while (cq_.Next(&got_tag, &ok))
    {
      AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

      GPR_ASSERT(ok);
      if (call->mapper_status.ok())
      {
        master_->update_temp_file_name_map(call->mapper_reply.file_names());
        master_->update_worker_status(ip_addr_port_, AVAILABLE);
        master_->increment_completion_count();
      }
      else if (call->reducer_status.ok())
      {
        master_->update_output_files(call->reducer_reply.file_names());
        master_->update_worker_status(ip_addr_port_, AVAILABLE);
        master_->increment_completion_count();
      }
      else
      {
        std::cout << "RPC failed" << std::endl;
      }

      delete call;
      delete this;
    }
  }

private:
  struct AsyncClientCall
  {
    MapperReply mapper_reply;
    ReducerReply reducer_reply;

    ClientContext context;

    Status mapper_status;
    Status reducer_status;

    std::unique_ptr<ClientAsyncResponseReader<MapperReply> > mapper_response_reader;
    std::unique_ptr<ClientAsyncResponseReader<ReducerReply> > reducer_response_reader;
  };

  std::string ip_addr_port_;
  Master *master_;
  std::unique_ptr<MapReduceMasterWorker::Stub> stub_;
  CompletionQueue cq_;
};

Master::Master(const MapReduceSpec& mr_spec, const std::vector<FileShard>& file_shards)
{
  mr_spec_ = mr_spec;
  file_shards_ = file_shards;

  for (auto& worker_ipaddr_port : mr_spec.worker_ipaddr_ports)
  {
    worker_status_[worker_ipaddr_port] = AVAILABLE;
  }
}

bool Master::run()
{
  run_all_map_tasks();
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [this]{ return completion_count_ == file_shards_.size(); });
  run_all_reduce_tasks();
  cv_.wait(lock, [this]{ return completion_count_ == mr_spec_.n_output_files; });
  
  return true;
}

inline void Master::update_temp_file_name_map(const std::vector<std::string>& temp_file_names)
{
  for (auto& temp_file_name : temp_file_names)
  {
    std::string key = temp_file_name.substr(temp_file_name.find_last_of('/'));
    if (temp_file_name_map_.count(key) == 0)
    {
      temp_file_name_map_[key] = std::vector<std::string>();
    }

    temp_file_name_map_[key].push_back(temp_file_name);
  }
}

inline void Master::update_worker_status(const std::string& ip_addr_port, WORKER_STATUS status)
{
  if (status == AVAILABLE)
  {
    is_worker_idle_ = true;
  }

  worker_status_[ip_addr_port] = status;
}

inline void Master::update_output_files(const std::vector<std::string>& output_file_names)
{
  output_file_names_.insert(output_file_names_.end(), output_file_names.begin(), output_file_names.end());
}

inline void Master::increment_completion_count()
{
  completion_count_ += 1;
}

inline void Master::run_all_map_tasks()
{
  completion_count_ = 0;

  for (auto& file_shard : file_shards_)
  {
    MasterImpl* master_impl = new MasterImpl(get_idle_worker(), this);
    std::thread(&MasterImpl::async_client_call_complete, &master_impl);
    master_impl->map(mr_spec_.user_id(), file_shard);
  }
}

inline void Master::run_all_reduce_tasks()
{
  completion_count_ = 0;

  size_t n_keys_per_reducer = std::ceil(double(temp_file_name_map_.size()) / mr_spec_.n_output_files);
  size_t curr_index_in_temp_file_name_map = 0;
  size_t n_curr_output_files = 0;
  while (curr_index_in_temp_file_name_map < temp_file_name_map_.size())
  {
    std::vector<std::string> temp_file_names;
    for (size_t n_keys_curr_reducer = 0 ; n_keys_curr_reducer < n_keys_per_reducer ; ++n_keys_curr_reducer)
    {
      temp_file_names.insert(temp_file_names.end(), 
        std::next(temp_file_name_map_.begin(), curr_index_in_temp_file_name_map)->second.begin(),
        std::next(temp_file_name_map_.begin(), curr_index_in_temp_file_name_map)->second.end());
      ++curr_index_in_temp_file_name_map;
    }

    MasterImpl* master_impl = MasterImpl(get_idle_worker(), this);
    std::thread(&MasterImpl::async_client_call_complete, &master_impl);
    master_impl->reduce(mr_spec_.user_id(), temp_file_names);

    ++n_curr_output_files;
    if (n_keys_per_reducer != 1 &&
      (temp_file_name_map_.size() - curr_index_in_temp_file_name_map == mr_spec_.n_output_files - n_curr_output_files))
    {
      n_keys_per_reducer = 1;      
    }
  }
}

inline std::string Master::get_idle_worker()
{
  for (auto& worker_ipaddr_port : mr_spec_.worker_ipaddr_ports)
  {
    if (worker_status_[worker_ipaddr_port] == AVAILABLE)
    {
      worker_status_[worker_ipaddr_port] = BUSY;

      return worker_ipaddr_port;
    }
  }

  is_worker_idle_ = false;
  std::unique_lock<std::mutex> lock(m_);
  cv_.wait(lock, [this]{ return is_worker_idle_; });

  return get_idle_worker();
}
