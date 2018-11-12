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
using store::ProductInfo;
using store::Store;
using vendor::BidReply;

BidReply query_vendor(std::string product_name, std::string vendor_address);

class ServerImpl final
{
public:
  ServerImpl(std::string server_address, unsigned num_max_threads)
  {
    server_address_ = server_address;
    pool_ = Threadpool(num_max_threads);
  }

  ~ServerImpl()
  {
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

  BidReply query_vendor(std::string product_name, std::string vendor_address)
  {
    auto result = pool_.enqueue([](std::string product_name, std::string vendor_address)
    {
      VendorClient vendor_client(grpc::CreateChannel(vendor_address, grpc::InsecureChannelCredentials()));
      return vendor_client.get_details(product_name);
    },
    request_.product_name(),
    vendor_address);

    return result.get();
  }

/*
  BidReply query_vendor(std::string product_name, std::string vendor_address)
  {
    VendorClient vendor_client(grpc::CreateChannel(vendor_address, grpc::InsecureChannelCredentials()));
    return vendor_client.get_details(product_name);
  }
*/

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
        service_->RequestgetProducts(&ctx_, &request_, &responder_, cq_, cq_, this);
      }
      else if (status_ == PROCESS)
      {
        new CallData(service_, cq_, caller_);

        std::string vendor_address;
        std::ifstream myfile("vendor_addresses.txt");
        while(getline(myfile, vendor_address))
        {
          BidReply reply = caller->query_vendor(request_.product_name(), vendor_address);

          if (reply.price() == -1)
          {
            continue;
          }

          ProductInfo prod_info;
          prod_info.set_vendor_id(reply.vendor_id());
          prod_info.set_price(reply.price());
          reply_.add_products()->CopyFrom(prod_info);
        }

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

  Threadpool pool_;
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
