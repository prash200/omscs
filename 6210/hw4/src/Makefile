HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)

CXX = g++
CPPFLAGS += -I/usr/local/include -pthread
CXXFLAGS += -std=c++11 -g

ifeq ($(SYSTEM), Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl -luuid
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -lgrpc++_reflection \
           -lprotobuf -lpthread -ldl -luuid
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ./

vpath %.proto $(PROTOS_PATH)

MAPREDUCE_LIB = ../external/lib/libmapreduce.a
MR_WORKER_LIB = ../external/lib/libmr_worker.a

all: $(MAPREDUCE_LIB) $(MR_WORKER_LIB)

$(MAPREDUCE_LIB): masterworker.pb.o masterworker.grpc.pb.o mapreduce_impl.o mapreduce.o
	ar -rcs $@ masterworker.pb.o masterworker.grpc.pb.o mapreduce_impl.o mapreduce.o
	ranlib $@

$(MR_WORKER_LIB): masterworker.pb.o masterworker.grpc.pb.o mr_task_factory.o run_worker.o
	ar -rcs $@ masterworker.pb.o masterworker.grpc.pb.o mr_task_factory.o run_worker.o
	ranlib $@

%.o: %.cc
	$(CXX) -c $< -I../external/include $(CXXFLAGS)

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
	rm -f *.o *.pb.cc *.pb.h $(MAPREDUCE_LIB) $(MR_WORKER_LIB)
