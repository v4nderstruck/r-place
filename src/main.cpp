#include "tile.hpp"
#include "wsserver.hpp"
#include <CLI11.hpp>
#include <boost/asio/ip/address.hpp>
#include <iostream>
#include <memory>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Log.h>
#include <thread>

#define APP_DESC "r/place clone v0.0.1"

int main(int argc, char **argv) {
  plog::init<plog::TxtFormatter>(plog::info, plog::streamStdOut);

  CLI::App app{APP_DESC};
  int width = 1000;
  int height = 1000;
  int num_threads = 2;

  app.add_option("-x,--width", width, "Width of the canvas")
      ->check(CLI::Range(1, 10000));
  app.add_option("-y,--height", height, "Height of the canvas")
      ->check(CLI::Range(1, 10000));
  app.add_option("-t,--threads", num_threads, "Number of threads to use")
      ->check(CLI::Range(1, 100));

  std::string addr = "0.0.0.0";
  app.add_option<std::string>("-b,--bind", addr,
                              "Address to bind to (eg. 0.0.0.0)");
  unsigned short port = 8081;
  app.add_option("-p,--port", port, "Port to bind to (eg. 8081)")
      ->check(CLI::Range(1, 65535));

  auto const bind_addr = net::ip::make_address(addr);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    std::cout << e.what() << std::endl;
    return app.exit(e);
  }

  PLOG_INFO << "Starting tile server with width " << width << " and height "
            << height;
  PLOG_INFO << "Binding to " << bind_addr << ":" << port;
  PLOG_INFO << "Using " << num_threads << " threads";

  net::io_context ioc{num_threads};
  auto tile = std::make_shared<Tile>(width, height);

  std::make_shared<Listener>(ioc, tcp::endpoint(bind_addr, port), tile)->run();

  std::vector<std::thread> t;
  t.reserve(num_threads - 1);

  for (auto i = num_threads - 1; i > 0; --i) {
    t.emplace_back([&ioc, &tile] { ioc.run(); });
  }

  ioc.run();
  return 0;
}
