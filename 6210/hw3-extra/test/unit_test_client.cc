#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>

#include "store.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using store::Store;
using store::ProductReply;
using store::ProductQuery;
using store::ProductQueryItem;

std::string serverAddress = "0.0.0.0:50001";

class ServerImpl final : public Store::Service { 
	Status buyProducts(ServerContext* context, const ProductQuery* query, ProductReply* reply) {
		std::cout << "Query : \n";
		for (const ProductQueryItem& item : query->query_items()) {
			std::cout << item.vendor_id()<<std::endl;
			std::cout << item.product_name()<<std::endl;
			std::cout << item.count()<<std::endl;
		}
		reply->set_success(true);
		return Status::OK;
	}
};

void RunServer() {
  ServerImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << serverAddress << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}
int main(int argc, char** argv) {
	RunServer();
  
	return 0;
}

