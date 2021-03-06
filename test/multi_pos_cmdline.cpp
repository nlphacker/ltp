/*
 * Multi-threaded postagger test program. The user input a line
 * of Chinese sentence an the program will output its segment
 * result.
 *
 *  @dependency package: tinythread - a portable c++ wrapper for
 *                       multi-thread library.
 *  @author:             LIU, Yijia
 *  @data:               2013-09-24
 *
 * This program is special designed for UNIX user, for get time
 * is not compilable under MSVC
 */
#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <vector>
#include <list>
#include "time.hpp"
#include "tinythread.h"
#include "postag_dll.h"

//using namespace std;
//using namespace tthread;

const int MAX_LEN = 1024;

class Dispatcher {
public:
  Dispatcher( void * model ) {
    _model = model;
  }

  int next(std::vector<std::string> &words) {
    std::string line;
    std::string word;
    tthread::lock_guard<tthread::mutex> guard(_mutex);
    if (getline(std::cin, line, '\n')) {
      std::stringstream S(line);
      words.clear();
      while (S >> word) { words.push_back(word); }
    } else {
      return -1;
    }
    return 0;
  }

  void output(const std::vector<std::string> & words,
      const std::vector<std::string> &postags) {
      tthread::lock_guard<tthread::mutex> guard(_mutex);
    if (words.size() != postags.size()) {
      return;
    }

    for (int i = 0; i < words.size(); ++ i) {
      std::cout << words[i] << "_" << postags[i];
      std::cout << (i == words.size() - 1 ? '\n' : '|');
    }
    return;
  }

  void * model() {
    return _model;
  }

private:
  tthread::mutex  _mutex;
  void * _model;
  std::string _sentence;
};

void multithreaded_postag( void * args) {
  std::vector<std::string> words;
  std::vector<std::string> postags;

  Dispatcher * dispatcher = (Dispatcher *)args;
  void * model = dispatcher->model();

  while (true) {
    int ret = dispatcher->next(words);

    if (ret < 0)
      break;

    postags.clear();
    postagger_postag(model, words, postags);
    dispatcher->output(words, postags);
  }

  return;
}

int main(int argc, char ** argv) {
  if (argc < 2 || (0 == strcmp(argv[1], "-h"))) {
    std::cerr << "Usage: ./multi_pos_cmdline [model path] threadnum" << std::endl;
    std::cerr << std::endl;
    std::cerr << "This program recieve input word sequence from stdin." << std::endl;
    std::cerr << "One sentence per line. Words are separated by space." << std::endl;
    return -1;
  }

  void * engine = postagger_create_postagger(argv[1]);

  if (!engine) {
    return -1;
  }

  int num_threads = atoi(argv[2]);

  if(num_threads < 0 || num_threads > tthread::thread::hardware_concurrency()) {
    num_threads = tthread::thread::hardware_concurrency();
  }

  std::cerr << "TRACE: Model is loaded" << std::endl;
  std::cerr << "TRACE: Running " << num_threads << " thread(s)" << std::endl;

  Dispatcher * dispatcher = new Dispatcher( engine );

  double tm = ltp::utility::get_time();
  std::list<tthread::thread *> thread_list;
  for (int i = 0; i < num_threads; ++ i) {
      tthread::thread * t = new tthread::thread( multithreaded_postag, (void *)dispatcher );
    thread_list.push_back( t );
  }

  for (std::list<tthread::thread *>::iterator i = thread_list.begin();
      i != thread_list.end(); ++ i) {
      tthread::thread * t = *i;
    t->join();
    delete t;
  }

  tm = ltp::utility::get_time() - tm;
  std::cerr << "TRACE: multi-pos-tm-consume "
    << tm
    << " seconds."
    << std::endl;

  return 0;
}

