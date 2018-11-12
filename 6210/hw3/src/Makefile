CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11 -g
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../external/protos/

vpath %.proto $(PROTOS_PATH)

all: store

store: vendor.pb.o vendor.grpc.pb.o store.pb.o store.grpc.pb.o store.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	chmod 544 *.grpc.pb.* || true
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<
	chmod 444 *.grpc.pb.*

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	chmod 544 *.pb.* || true
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<
	chmod 444 *.pb.*

clean:
	rm -f *.o *.pb.cc *.pb.h store
