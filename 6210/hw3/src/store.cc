#include "server_async.cc"
#include <chrono>
#include <iostream>

int main(int argc, char** argv)
{
  unsigned num_max_threads;
  std::string server_address;
  if (argc == 3)
  {
    server_address = std::string(argv[1]);
    num_max_threads = std::min(20, std::max(0,atoi(argv[2])));
  }
  else if (argc == 2)
  {
    server_address = std::string(argv[1]);
    num_max_threads = 4;
  }
  else
  {
    server_address = "0.0.0.0:50056";
    num_max_threads = 4;
  }

  run_server(server_address, num_max_threads);

	return EXIT_SUCCESS;
}
