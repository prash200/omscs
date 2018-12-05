#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>

#include <grpc++/grpc++.h>
#include <grpc/support/log.h>

#include "vendor.grpc.pb.h"

using grpc::Status;
//client things
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using vendor::Vendor;
using vendor::TransactionQuery;
using vendor::TransactionReply;
using vendor::GetProductInfoQuery;
using vendor::GetProductInfoReply;

class VendorInterface final {
	private:
		std::unique_ptr<Vendor::Stub> stub_;
	public:
		VendorInterface(const std::string& vendorIp) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(vendorIp, grpc::InsecureChannelCredentials());
			stub_ = Vendor::NewStub(channel);
		}

		bool testVendorReserve (const std::string name, const uint count) {
            TransactionQuery request;
            request.set_product_name(name);
            request.set_count(count);

			TransactionReply reply;
			ClientContext context;
			CompletionQueue cq;
			Status status;

			std::unique_ptr<ClientAsyncResponseReader<TransactionReply> > rpc;

            // reserve
            rpc = stub_->AsyncreserveProduct(&context,request,&cq);
            rpc->Finish(&reply,&status,this);

			void* got_tag;
			bool ok = false;
            GPR_ASSERT(cq.Next(&got_tag, &ok));
            GPR_ASSERT(ok);
            if(got_tag == this) {
                if(status.ok()) {
                    return reply.success();
                } else {
                    printf("AsyncreserveProduct:: RPC failed\n");
                }
            }
            return false;
		}

        bool testVendorRelease (const std::string name, const uint count) {
            TransactionQuery request;
            request.set_product_name(name);
            request.set_count(count);

			TransactionReply reply;
			ClientContext context;
			CompletionQueue cq;
			Status status;

			std::unique_ptr<ClientAsyncResponseReader<TransactionReply> > rpc;

            // release
            rpc = stub_->AsyncreleaseProduct(&context,request,&cq);
            rpc->Finish(&reply,&status,this);

			void* got_tag;
			bool ok = false;
            GPR_ASSERT(cq.Next(&got_tag, &ok));
            GPR_ASSERT(ok);
            if(got_tag == this) {
                if(status.ok()) {
                    return reply.success();
                } else {
                    printf("AsyncreleaseProduct:: RPC failed\n");
                }
            }
            return false;
		}

        bool testVendorSell (const std::string name, const uint count) {
            TransactionQuery request;
            request.set_product_name(name);
            request.set_count(count);

			TransactionReply reply;
			ClientContext context;
			CompletionQueue cq;
			Status status;

			std::unique_ptr<ClientAsyncResponseReader<TransactionReply> > rpc;

            // sell
            rpc = stub_->AsyncsellProduct(&context,request,&cq);
            rpc->Finish(&reply,&status,this);

			void* got_tag;
			bool ok = false;
            GPR_ASSERT(cq.Next(&got_tag, &ok));
            GPR_ASSERT(ok);
            if(got_tag == this) {
                if(status.ok()) {
                    return reply.success();
                } else {
                    printf("AsyncsellProduct:: RPC failed\n");
                }
            }
            return false;
		}

        void printProductInfo (const std::string name) {
            GetProductInfoQuery request;
            request.set_product_name(name);

			GetProductInfoReply reply;
			ClientContext context;
			CompletionQueue cq;
			Status status;

			std::unique_ptr<ClientAsyncResponseReader<GetProductInfoReply> > rpc;

            // get info
            rpc = stub_->AsyncgetProductInfo(&context,request,&cq);
            rpc->Finish(&reply,&status,this);

			void* got_tag;
			bool ok = false;
            GPR_ASSERT(cq.Next(&got_tag, &ok));
            GPR_ASSERT(ok);
            if(got_tag == this) {
                if(status.ok()) {
                    std::cout << "printProductInfo:: " \
                    << " Product Name : " << reply.product_name() \
                    << "; TotalCount : " << reply.totalcount() \
                    << "; Available : " << reply.available() \
                    << "; Reserved : " << reply.reserved() \
                    << "; Sold : " << reply.sold() \
                    << std::endl;
                } else {
                    printf("AsyncgetProductInfo:: RPC failed\n");
                }
            }
        }
};

int main () {
    std::cout << "test_vendor" << std::endl;
    const std::string vendor_ip = "localhost:50051";
    VendorInterface vendorInterface(vendor_ip);

    std::cout << "assuming book1 initially has 10 items." << std::endl;

    vendorInterface.printProductInfo("book1");
    if (!vendorInterface.testVendorReserve("book1", 5)) {
        std::cout << "test_vendor:: ERROR! - 1" << std::endl;
    }
    vendorInterface.printProductInfo("book1");
    if(!vendorInterface.testVendorSell("book1", 5)) {
        std::cout << "test_vendor:: ERROR! - 2" << std::endl;
    }
    vendorInterface.printProductInfo("book1");

    if (!vendorInterface.testVendorReserve("book1", 3)) {
        std::cout << "test_vendor:: ERROR! - 3" << std::endl;
    }
    vendorInterface.printProductInfo("book1");
    if (!vendorInterface.testVendorRelease("book1", 3)) {
        std::cout << "test_vendor:: ERROR! - 4" << std::endl;
    }
    vendorInterface.printProductInfo("book1");

    vendorInterface.printProductInfo("book1");
    if (!vendorInterface.testVendorReserve("book1", 5)) {
        std::cout << "test_vendor:: ERROR! - 5" << std::endl;
    }
    vendorInterface.printProductInfo("book1");
    if(!vendorInterface.testVendorSell("book1", 5)) {
        std::cout << "test_vendor:: ERROR! - 6" << std::endl;
    }
    vendorInterface.printProductInfo("book1");

    if (!vendorInterface.testVendorReserve("book1", 3)) {
        std::cout << "test_vendor:: ERROR! - 7" << std::endl;
    }
    vendorInterface.printProductInfo("book1");
    if (!vendorInterface.testVendorRelease("book1", 3)) {
        std::cout << "test_vendor:: ERROR! - 8" << std::endl;
    }
    vendorInterface.printProductInfo("book1");

    std::cout << "You should see error 7 and error 8" << std::endl;

    return 0;
}

