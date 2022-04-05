#include <text-server.h>
#include <text-client.h>
#include <named_semaphore.h>
#include <shm_store.h>

#include <cstddef>
#include <iostream>

namespace logger {

Producer::Producer(const char shm_name[], const char mutex_name[])
    : shm_name_(shm_name),
      shm_log_signal_(mutex_name) {
  // get shared memory log signal named semaphore
  //   created by and locked by consumer
  shm_log_signal_.Open();
  std::cout << "SERVER STARTED" <<std::endl;
}


int Producer::Produce(const std::string& msg) {
  //OPENS THE SHARED MEMORY (3)
  std::clog << "\tMEMORY OPEN" <<std::endl;
  // open shared memory and get file descriptor
  int shm_fd = ::shm_open(shm_name_.c_str(), O_RDWR, 0);
  if (shm_fd < 0) {
    std::cerr << ::strerror(errno) << std::endl;

    return errno;
  }


  // get copy of mapped mem
  SharedMemoryStore* store = static_cast<SharedMemoryStore*>(
    ::mmap(nullptr,
           sizeof(SharedMemoryStore),
           PROT_READ | PROT_WRITE,
           MAP_SHARED,
           shm_fd,
           0));
  if (store == MAP_FAILED) {
    std::cerr << ::strerror(errno) << std::endl;

    return errno;
  }

  std::clog << "\tOPENING" << store << std::endl;
  std::ifstream in(msg);
  if(in.is_open()) {
    while (!in.eof())
    {
      std::string message = "";
      //strncpy(store->buffer, getline(in, message).c_str(), store->buffer_size);
    }
    
  in.close();
  }


  // copy string msg into shared memory via strncpy ('n' for bounded copy)
  strncpy(store->buffer, msg.c_str(), store->buffer_size);


  // signal consumer
  shm_log_signal_.Up();

  // release copy of mapped mem
  int result = ::munmap(store, sizeof(SharedMemoryStore));
  if (result < 0) {
    std::cerr << ::strerror(errno) << std::endl;

    return errno;
  }

  return 0;
}
}

int main(int argc, char* argv[]) {
  // wrappers::NamedSemaphore Create(int client);
  // wrappers::NamedSemaphore Create(int server);

  assert(argc == 3 && "producer <shared_mem_name> <log_mutex_name>");

  logger::Producer log_writer(argv[1], argv[2]);

  std::cout << "Sending: > ";

  const size_t kBuffer_size = 64;
  char buffer[kBuffer_size];
  std::cin.getline(buffer, kBuffer_size);
  buffer[std::cin.gcount() + 1] = '\n';
  std::string msg(buffer);
  std::cout << "Bytes read: " << std::cin.gcount() << std::endl;
  while (std::cin.gcount() == kBuffer_size - 1) {
    std::cin.clear();
    std::cin.getline(buffer, kBuffer_size);
    std::cout << "Bytes read: " << std::cin.gcount() << std::endl;
    buffer[std::cin.gcount() + 1] = '\0';
    msg += buffer;
  }
  std::cout << "Sent: " << msg << std::endl;
  log_writer.Produce(std::string(msg));

  return 0;
}
