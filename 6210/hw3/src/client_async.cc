#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include <grpc++/grpc++.h>

#include "vendor.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;

class VendorClient
{
 public:
  explicit VendorClient(std::shared_ptr<Channel> channel): stub_(Vendor::NewStub(channel))
  {}

  BidReply get_details(const std::string& product_name)
  {
    BidQuery request;
    request.set_product_name(product_name);

    ClientContext context;
    CompletionQueue cq;
    std::unique_ptr<ClientAsyncResponseReader<BidReply> > rpc(stub_->AsyncgetProductBid(&context, request, &cq));

    BidReply reply;
    Status status;
    rpc->Finish(&reply, &status, (void*)1);

    void *got_tag;
    bool ok = false;   
    GPR_ASSERT(cq.Next(&got_tag, &ok));
    GPR_ASSERT(got_tag == (void*)1);
    GPR_ASSERT(ok);
    if (status.ok()) 
    {
      return reply;
    }
    else
    {
      return NULL;
    }
  }

 private:
  std::unique_ptr<Vendor::Stub> stub_;
};

BidReply run_client(std::string product_name, std::string ip_address)
{  
  VendorClient vendor_(grpc::CreateChannel(ip_address, grpc::InsecureChannelCredentials()));

  return vendor_.get_details(product_name);
}
