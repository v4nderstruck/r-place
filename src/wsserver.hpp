#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include "tile.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/signals2/connection.hpp>
#include <deque>
#include <memory>
#include <plog/Log.h>
#include <string>

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING  

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

typedef struct WsMessage {
  enum TYPE { MSG_TEXT, MSG_BINARY };
  TYPE msg_type;
  boost::asio::const_buffer buf;
} WsMessage;

//TODO: proper session closing on error states
class Session : public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket &&socket, std::shared_ptr<Tile> &tile_map);
  void run();
  void on_check_path(beast::error_code ec, std::size_t bytes_transferred);
  void on_run();
  void handle_messages();
  void on_accept();
  void do_read();
  void on_read(beast::error_code ec, std::size_t bytes_transferred);
  void send_messages();
  void on_write(beast::error_code ec, std::size_t bytes_transferred);
  void nop(beast::error_code ec, std::size_t bytes_transferred);
  void do_close();
  void send_tile_updates();

private:
  std::deque<WsMessage> send_queue;
  signals::connection s_connection;
  websocket::stream<beast::tcp_stream> ws;
  beast::flat_buffer buffer;
  std::shared_ptr<Tile> tile_map;
  http::request<beast::http::string_body> req;
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
