#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include "tile.hpp"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <memory>
#include <plog/Log.h>
#include <string>

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket &&socket, std::shared_ptr<Tile> &tile_map);
  void run();
  void do_read();
  void on_read(beast::error_code ec, std::size_t bytes_transferred);
  void send_response(http::message_generator &&boost);
  void on_write(bool keep_alive, beast::error_code ec,
                std::size_t bytes_transferred);
  void do_close();

private:
  beast::tcp_stream stream;
  beast::flat_buffer buffer;
  std::shared_ptr<Tile> tile_map;
  http::request<http::string_body> req;
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
  Listener(net::io_context &ioc, tcp::endpoint endpoint,
           std::shared_ptr<Tile> &tile_map);
  void run();

private:
  void do_accept();
  void on_accept(beast::error_code ec, tcp::socket socket);

  net::io_context &ioc;
  tcp::acceptor acceptor;
  std::shared_ptr<Tile> tile_map;
};

#endif // HTTPSERVER_HPP
