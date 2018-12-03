### external directory (Do not make any changes in this directory)
When you provide a package(you can relate with grpc) to the outside world, you expose mainly two things: `libraries and header files`.

1. **external/include** - This directory consists of header files containing function declarations, that the user will make use of to run MapReduce.
  - `mapreduce.h` - This contains the interface that enables the user to use a MapReduce job.
  - `mr_task_factory.h` - This contains the interfaces through which the user can register their own mapper and reducer algorithm with MapReduce.

2. **external/lib** - This directory consists of libraries that has definitions of above functions, and so their implementations are hidden. **Note that these are autogenerated when you run `make` in your src directory**
  - `libmapreduce.a` - Library contating definitions for `mapreduce.h`.
  - `libmr_worker.a` - Library contating definitions for `mr_task_factory.h`.

### test directory (Do not make any changes in this directory)
This gives you an insight of how a user will use your MapReduce package.

1. **`config.ini`** - Through this file the user gives input to your MapReduce framework as to how the user would like it to run.

2. **`user_tasks.cc`** - This is where the user has implemented his/her own algorithm for mapper and reducer. Then he/she registers it with the mr_task_factory, so that it can be picked up by your framework.

3. **`main.cc`** - This is just the entry point in the user's binary to run the MapReduce job.

### src directory (Read the comments in each source file that indicates what you are supposed to do with it)
Now you own the development of MapReduce and this is where you implement the ideas.
We are describing here only the files which you are supposed to work on. For the other files, just have a quick glance through them. It should be straightforward to understand their purpose.

1. **`masterworker.proto`** - containing the grpc protocol between master and worker. **The choice of communication mode (synchronous/asynchronous) is up to you, as long as the parallel nature of MapReduce is maintained (the workers must be able perform their task in parallel). We will accept both methods.**

2. **`file_shard.h`** - This file contains the data structure for how you would maintain info about the file splits, that master passes to workers for mapper task. Also it contains functions, that framework will call to generate file shards given the split size.

3. **`mapreduce_spec.h`** - This structure will be populated by the framework. There is a helper function which takes the config file name and populates this structure. Framework will also validate the structure(valid input file paths, apt amount of ip_addr_ports etc.). This structure is finally what is passed to the master, so think what all you would need in this.

4. **`master.h`** - This is where you have to think a lot about, what all master does in map reduce. And then construct the required data structure and book keeping. How it will communicate using grpc with the workers etc.

5. **`mr_tasks.h`** - This is where you implement the crux of how you handle a map/reduce task. It also includes emit function's implementation, which user's map/reduce function will use to write intermediate key/value papers.

6. **`worker.h`** - This is where you code how the workers will run map and reduce tasks on receiving a request from master, meanwhile simply waiting for a task the other times.

### How to run the test setup
  - Goto src directory and run `make` command.
  - Two libraries would be created in external directory: `libmapreduce.a` and `libmr_worker.a`.
  - Now goto test directory and run `make` command.
  - Two binaries would be created: `mrdemo` and `mr_worker`.
  - **Now running the demo, once you have created all the binaries and libraries.**
   1. Clear the files if any in the output directory
   2. Start all the worker processes in this fashion(e.g. for 2 workers): `./mr_worker localhost:50051 & ./mr_worker localhost:50052;`
   3. Then start your main map reduce process: `./mrdemo config.ini`
   4. Once the ./mrdemo finishes, kill all the worker proccesses you started.
   5. Check output directory to see if you have the correct results(obviusly once you have done the proper implementation of your library)