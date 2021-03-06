#include <text-client.h>
#include <text-server.h>

#include <cstddef>
#include <iostream>

namespace logger {

Consumer::Consumer(const char* shm_name, const char* shm_log_signal_name)
    : shm_name_(shm_name),
      shm_log_signal_(shm_log_signal_name) {
  // open shared memory and capture file descriptor
  int shm_fd = ::shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0660);
  if (shm_fd < 0) {
    std::cerr << ::strerror(errno) << std::endl;

    ::exit(errno);
  }

  // set size of shared memory with file descriptor
  const size_t kBuffer_size = 4096 - sizeof(size_t);  // page - buffer_size size
  const size_t kSHM_size = sizeof(SharedMemoryStore) + kBuffer_size;
  if (::ftruncate(shm_fd, kSHM_size) < 0) {
    std::cerr << ::strerror(errno) << std::endl;

    ::exit(errno);
  }

  // get copy of mapped mem
  const int kProt = PROT_READ | PROT_WRITE;
  store_ = static_cast<SharedMemoryStore*>(
    ::mmap(nullptr, sizeof(SharedMemoryStore), kProt, MAP_SHARED, shm_fd, 0));

  if (store_ == MAP_FAILED) {
    std::cerr << ::strerror(errno) << std::endl;

    ::exit(errno);
  }

  // init memory map
  *store_ = {};
  store_->buffer_size = kBuffer_size;  // set store's buffer size

  // create signal mux (unlocked by producer(s))
  shm_log_signal_.Create(0);
  shm_log_signal_.Open();
}


Consumer::~Consumer() {
  // return copy of mapped mem, capture any error/exit code
  int exit_code = ::munmap(store_, sizeof(SharedMemoryStore));

  // alert for error in ::munmap
  if (exit_code < 0)
    std::cerr << ::strerror(errno) << std::endl;

  // delete shared memory map
  if (::shm_unlink(shm_name_) < 0)
    std::cerr << ::strerror(errno) << std::endl;

  // delete named semaphore
  shm_log_signal_.Destroy();

  ::exit(errno);
}


void Consumer::Consume(const char log_name[]) {
  // write any logs to file
  std::string msg;
  while (true) {
    shm_log_signal_.Down();  // block until occupied signal
    std::ofstream fout(log_name, std::ofstream::app);

    fout << store_->buffer << '\n';  // write to file

    fout.close();
  }
}


}  // namespace logger
// Deletes kLogger memory (calls destructor)
void LoggerSigTermHandler(int sig);


logger::Consumer* log_writer;


int main(int argc, char* argv[]) {
  assert(argc == 4
    && "usage: consumer <shared_memory_name> <log_signal_mux_name> <log_file_name>");

  // set SIGTERM signal handler to unlink shm
  ::signal(SIGTERM, LoggerSigTermHandler);
  ::signal(SIGINT, LoggerSigTermHandler);

  // build Consumer and start writing to file
  log_writer = new logger::Consumer(argv[1], argv[2]);
  log_writer->Consume(argv[3]);

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
