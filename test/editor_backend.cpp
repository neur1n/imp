#include <cstdlib>
#include <iostream>
#include <string>

// Let the editor test decide when each asynchronous request completes.
int main(int argc, char** argv)
{
  int idx = 1;
  std::string pair = "wetype";
  std::string config;
  while (idx + 1 < argc && (std::string(argv[idx]) == "--pair" || std::string(argv[idx]) == "--config")) {
    if (std::string(argv[idx]) == "--pair") {
      pair = argv[idx + 1];
    } else {
      config = argv[idx + 1];
    }
    idx += 2;
  }
  if (pair != "wetype" && pair != "pinyin" && pair != "wubi") {
    return EXIT_FAILURE;
  }

  std::string signal;
  if (!std::getline(std::cin, signal) || signal != "continue") {
    return EXIT_FAILURE;
  }

  std::cout << "pair=" << pair << '\n';
  std::cout << "config=" << config << '\n';
  if (argc - idx == 1 && std::string(argv[idx]) == "status") {
    std::cout << "mode=alphanumeric\n";
  } else if (argc - idx == 2 && std::string(argv[idx]) == "set") {
    std::cout << "mode=" << argv[idx + 1] << '\n';
  } else {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
