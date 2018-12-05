#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <grpc++/grpc++.h>
#include "store.grpc.pb.h"
#include "threadpool.h"
#include "vendor_client_async.cc"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using store::ProductQuery;
using store::ProductReply;
using store::ProductQueryItem;
using store::Store;
using vendor::BidReply;
using vendor::TransactionReply;
using vendor::GetProductInfoReply;

BidReply query_vendor(std::string product_name, std::string vendor_address);

class ServerImpl final
{
public:
  ServerImpl(std::string server_address, unsigned num_max_threads)
  {
    server_address_ = server_address;
    pool_ = new ThreadPool(num_max_threads);
  }

  ~ServerImpl()
  {
    delete pool_;
    server_->Shutdown();
    cq_->Shutdown();
  }

  void Run()
  {
    ServerBuilder builder;
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();

    std::cout<<"Server listening on "<<server_address_<<std::endl;

    HandleRpcs();
  }

  bool check_product_availibility_from_vendor(std::string vendor_id, std::string product_name, size_t count)
  {
    return pool_->enqueue([](std::string vendor_id, std::string product_name, size_t count)
    {
      VendorClient vendor_client(grpc::CreateChannel(vendor_id, grpc::InsecureChannelCredentials()));
      return true;
      //return vendor_client.get_product_info(product_name).available() >= count;
    },
    vendor_id,
    product_name,
    count).get();
  }

  bool reserve_product_from_vendor(std::string vendor_id, std::string product_name, size_t count)
  {
    return pool_->enqueue([](std::string vendor_id, std::string product_name, size_t count)
    {
      VendorClient vendor_client(grpc::CreateChannel(vendor_id, grpc::InsecureChannelCredentials()));
      return vendor_client.reserve_product(product_name, count).success();
    },
    vendor_id,
    product_name,
    count).get();
  }

  void release_product_from_vendor(std::string vendor_id, std::string product_name, size_t count)
  {
    pool_->enqueue([](std::string vendor_id, std::string product_name, size_t count)
    {
      VendorClient vendor_client(grpc::CreateChannel(vendor_id, grpc::InsecureChannelCredentials()));
      return vendor_client.release_product(product_name, count);
    },
    vendor_id,
    product_name,
    count).get();
  }

  void buy_product_from_vendor(std::string vendor_id, std::string product_name, size_t count)
  {
    pool_->enqueue([](std::string vendor_id, std::string product_name, size_t count)
    {
      VendorClient vendor_client(grpc::CreateChannel(vendor_id, grpc::InsecureChannelCredentials()));
      return vendor_client.buy_product(product_name, count);
    },
    vendor_id,
    product_name,
    count).get();
  }

private:
  class CallData
  {
  public:
    CallData(Store::AsyncService* service, ServerCompletionQueue* cq, ServerImpl* caller) : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE), caller_(caller)
    {
      Proceed();
    }

    void Proceed()
    {
      if (status_ == CREATE)
      {
        status_ = PROCESS;
        service_->RequestbuyProducts(&ctx_, &request_, &responder_, cq_, cq_, this);
      }
      else if (status_ == PROCESS)
      {
        new CallData(service_, cq_, caller_);

        bool success;
        for (auto& query_item : request_.query_items())
        {
          std::cout << this << " " << query_item.vendor_id() << " " << query_item.product_name() << " " << query_item.count() << std::endl;
          // This is just a primary check to see if the items are available with a vendor.
          // Even if this check succeeds, it doesn't mean the transaction would be successful. We would still need the following state machine.
          // It adoids obvious reservation followed by release when items are not available with a vendor latter in the list.
          // Which in turn could avoid failure for other client, increasing overall throughput of the store.
          success = caller_->check_product_availibility_from_vendor(query_item.vendor_id(), query_item.product_name(), query_item.count());
          if (!success)
          {
            break;
          }
        }

        if (success)
        {
          std::vector<ProductQueryItem> reserved_items;
          for (auto& query_item : request_.query_items())
          {
            success = caller_->reserve_product_from_vendor(query_item.vendor_id(), query_item.product_name(), query_item.count());
            if (!success)
            {
              for (auto& reserved_item : reserved_items)
              {
                caller_->release_product_from_vendor(reserved_item.vendor_id(), reserved_item.product_name(), reserved_item.count());
              }

              break;
            }

            reserved_items.push_back(query_item);
          }

          if (success)
          {
            for (auto& query_item : request_.query_items())
            {
              caller_->buy_product_from_vendor(query_item.vendor_id(), query_item.product_name(), query_item.count());
            }
          }
        }

        reply_.set_success(success);
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      }
      else
      {
        GPR_ASSERT(status_ == FINISH);
        delete this;
      }
    }

  private:
    ServerImpl *caller_;

    Store::AsyncService *service_;
    ServerCompletionQueue *cq_;
    ServerContext ctx_;
    ProductQuery request_;
    ProductReply reply_;
    ServerAsyncResponseWriter<ProductReply> responder_;

    enum CallStatus {CREATE, PROCESS, FINISH};
    CallStatus status_;
  };

  void HandleRpcs()
  {
    new CallData(&service_, cq_.get(), this);

    void *tag;
    bool ok;
    while (true)
    {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  ThreadPool *pool_;
  std::string server_address_;
  std::unique_ptr<ServerCompletionQueue> cq_;
  std::unique_ptr<Server> server_;
  Store::AsyncService service_;
};

void run_server(std::string server_address, unsigned num_max_threads)
{
  ServerImpl server(server_address, num_max_threads);
  server.Run();
}
