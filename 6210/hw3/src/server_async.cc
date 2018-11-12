#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <grpc++/grpc++.h>
#include "client_async.cc"

#include "store.grpc.pb.h"

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

class ServerImpl final
{
 public:
  ServerImpl(std::string server_address)
  {
    server_address_ = server_address;
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

    HandleRpcs();
  }

 private:
  class CallData
  {
   public:
    CallData(Store::AsyncService* service, ServerCompletionQueue* cq) : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
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
        new CallData(service_, cq_);

        std::string ip_addr;
        std::ifstream myfile("vendor_addresses.txt");
        while(getline(myfile, ip_addr))
        {
          BidReply reply = run_client(request_.product_name(), ip_addr);

          ProductInfo prod_info;
          prod_info.set_vendor_id(reply.vendor_id());
          prod_info.set_price(reply.vendor_id());
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
    new CallData(&service_, cq_.get());

    void *tag;
    bool ok;
    while (true)
    {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  std::string server_address_;
  std::unique_ptr<ServerCompletionQueue> cq_;
  std::unique_ptr<Server> server_;
  Store::AsyncService service_;
};

int run_server(std::string server_address, unsigned num_max_threads)
{
  ServerImpl server(server_address);
  server.Run();

  return 0;
}
