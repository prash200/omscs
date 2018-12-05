#include <memory>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <fstream>

#include <grpc++/grpc++.h>

#include "vendor.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using vendor::BidQuery;
using vendor::BidReply;
using vendor::Vendor;
using vendor::TransactionQuery;
using vendor::TransactionReply;
using vendor::GetProductInfoQuery;
using vendor::GetProductInfoReply;

typedef struct {
  uint totalCount;  // total number of product at initialization
  uint available;   // number of items available with this vendor
  uint reserved;    // number of items in reserved state
  uint sold;        // number of items sold
} countData;

typedef std::unordered_map<std::string, countData> ProductMap;

void DebugPrintProductMap (const ProductMap& debug_map) {
  std::cout << "DebugPrintProductMap++" << std::endl;
  for (auto &i : debug_map) {
    std::cout << i.first << " = " << "totalCount=" << i.second.totalCount << "; available=" << i.second.available ;
    std::cout << std::endl;
  }
  std::cout << "DebugPrintProductMap--" << std::endl;
}

void DebugPrintProduct (const ProductMap& debug_map, const std::string name) {
  auto itr = debug_map.find(name);
  if (itr == debug_map.end()) {
    std::cout << "DebugPrintProduct:: " << name << "not found" << std::endl;
    return;
  }
  // found
  std::cout << "DebugPrintProduct:: " << "name:" << name << "; totalCount:" << itr->second.totalCount << "; available:" << itr->second.available \
  << "; reserved:" << itr->second.reserved << "; sold:" << itr->second.sold << std::endl;
}

class VendorService final : public Vendor::Service {

  public:
    VendorService(const std::string& server_address, const std::string& config_file) : id_("Vendor_" + server_address),
    configFile_(config_file) {
      std::ifstream myfile (configFile_);
      if (myfile.is_open()) {
        std::string temp;
        // Each line should be of format - "ProductName;count"
        while (getline(myfile, temp)) {
          size_t pos = temp.find(';');
          const std::string productName = temp.substr(0, pos);
          pos += 1;
          uint count = stoi(temp.substr(pos));
          map_[productName].totalCount = count;
          map_[productName].available = count;
          map_[productName].reserved = 0;
          map_[productName].sold = 0;
        }
        myfile.close();
      }
      else {
        std::cerr << " VendorService:: Failed to open config file " << configFile_ << std::endl;
      }
    }

  private:
    Status getProductBid(ServerContext* context, const BidQuery* request,
                    BidReply* reply) override {
      reply->set_price(hasher_(id_ + request->product_name()) % 100);
      reply->set_vendor_id(id_);
      return Status::OK;
    }

    Status reserveProduct(ServerContext* context, const TransactionQuery* request,
                    TransactionReply* reply) override {
      uint count = request->count(); // number to reserve
      const std::string product = request->product_name();
      if (map_.find(product) == map_.end()) {
        // Not found with this vendor
        reply->set_success(false);
      } else {
        if (map_[product].available >= count) {
          // move to reserved state
          map_[product].available -= count;
          map_[product].reserved += count;
          reply->set_success(true);
        } else {
          // asked more than available currently.
          reply->set_success(false);
        }
      }
      return Status::OK;
    }

    Status releaseProduct(ServerContext* context, const TransactionQuery* request,
                    TransactionReply* reply) override {
      uint count = request->count(); // number to reserve
      const std::string product = request->product_name();
      if (map_.find(product) == map_.end()) {
        // Not found with this vendor
        reply->set_success(false);
      } else {
        if (map_[product].reserved >= count) {
          // move from reserved to available
          map_[product].reserved -= count;
          map_[product].available += count;
          reply->set_success(true);
        } else {
          // asked more than reserved currently.
          reply->set_success(false);
        }
      }
      return Status::OK;
    }

    Status sellProduct(ServerContext* context, const TransactionQuery* request,
                    TransactionReply* reply) override {
      uint count = request->count(); // number to reserve
      const std::string product = request->product_name();
      if (map_.find(product) == map_.end()) {
        // Not found with this vendor
        reply->set_success(false);
      } else {
        if (map_[product].reserved >= count) {
          // move to sold state
          map_[product].reserved -= count;
          map_[product].sold += count;
          reply->set_success(true);
        } else {
          // asked more than reserved currently.
          reply->set_success(false);
        }
      }
      return Status::OK;
    }

    Status getProductInfo(ServerContext* context, const GetProductInfoQuery* request,
                    GetProductInfoReply* reply) override {
      const std::string name = request->product_name();
      auto itr = map_.find(name);
      if (itr == map_.end()) {
        // not found
        reply->set_product_name(name);
        reply->set_totalcount(-1);
        reply->set_available(-1);
        reply->set_reserved(-1);
        reply->set_sold(-1);
      } else {
        // found
        reply->set_product_name(name);
        reply->set_totalcount(itr->second.totalCount);
        reply->set_available(itr->second.available);
        reply->set_reserved(itr->second.reserved);
        reply->set_sold(itr->second.sold);
      }
      return Status::OK;
    }


    std::hash<std::string> hasher_;
    const std::string id_;
    const std::string configFile_;
    ProductMap map_;
};

void run_server(const std::string server_address, const std::string config_file)   {
  std::string server_addressess(server_address);
  VendorService service(server_address, config_file);

  ServerBuilder builder;
  builder.AddListeningPort(server_addressess, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_addressess << std::endl;

  server->Wait();
}