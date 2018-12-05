#include <memory>
#include <stdlib.h>

#include <grpc++/grpc++.h>

#include "store.grpc.pb.h"

#include "product_queries_util.h"

using store::Store;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using store::ProductQuery;
using store::ProductReply;
using store::ProductQueryItem;

class StoreClient {

 public:
  StoreClient(std::shared_ptr<Channel>);
  bool getProducts(const std::vector<ProductSpec>&);

 private:
  std::unique_ptr<Store::Stub> stub_;
};


bool run_client(const std::string& server_addr, const std::vector<ProductSpec> query) {

  StoreClient store_client(grpc::CreateChannel(server_addr, grpc::InsecureChannelCredentials()));
  return store_client.getProducts(query);
}


StoreClient::StoreClient(std::shared_ptr<Channel> channel)
  : stub_(Store::NewStub(channel))
  {}

bool StoreClient::getProducts(const std::vector<ProductSpec>& query) {
  std::cout << "Attempting to purchase: \n";
  for (int i = 0; i < query.size(); ++i) {
    std::cout << "Vendor Id: " << query[i].vendor_id_ << "; " << "Product Name: " << query[i].name_ << "; " << "Count: " << query[i].count_ << std::endl;
  }
  std::cout<<std::endl;
  ProductQuery prot_query;
  for (int i = 0; i < query.size(); ++i) {
    ProductQueryItem* query_item = prot_query.add_query_items();
    query_item->set_vendor_id(query[i].vendor_id_);
    query_item->set_product_name(query[i].name_);
    query_item->set_count(query[i].count_);
  }

  ProductReply reply;
  ClientContext context;

  Status status = stub_->buyProducts(&context, prot_query, &reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return false;
  }

  std::cout << "Purchasing result:" << reply.success() << std::endl;

  return true;
}
