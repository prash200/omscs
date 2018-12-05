#include "product_queries_util.h"

#include <algorithm>

extern bool run_client(const std::string& server_addr, const std::vector<ProductSpec> query);

bool run_test(std::vector<std::vector<ProductSpec>>& product_specs, const std::string& server_addr);

int main(int argc, char** argv) {

  std::string server_addr;
  if (argc == 2) {
    server_addr = std::string(argv[1]);
  }
  else {
    server_addr = "0.0.0.0:50001";
  }

  std::vector<std::vector<ProductSpec>> product_specs;
  const std::string filename = "product_query_list.txt";
  if (!read_product_queries(product_specs, filename)) {
  	std::cerr << "Failed to extract product queries from: " << filename << std::endl;
  	return EXIT_FAILURE;
  }

  return run_test(product_specs, server_addr)
      ? EXIT_SUCCESS : EXIT_FAILURE;
}


bool warm_up(bool & is_done){
  is_done = true;
}

bool run_test(std::vector<std::vector<ProductSpec>>& product_specs, const std::string& server_addr) {
  for (int i = 0; i < product_specs.size(); ++i) {
    if (!run_client(server_addr, product_specs[i])) {
      std::cout << "\nStore failed to receive reply for: query id: " << i <<  std::endl;
    }
  }

  return true;
}
