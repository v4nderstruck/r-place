#include "wsserver.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/exception/all.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/url/src.hpp>
#include <chrono>
#include <exception>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

#define ENDPOINT "tile"

template <class Body, class Allocator>
http::message_generator
not_found(http::request<Body, http::basic_fields<Allocator>> &&req) {
  // Returns a not found response
  auto const nf = [&req]() {
    http::response<http::string_body> res{http::status::not_found,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  };
  return nf();
}

Listener::Listener(net::io_context &ioc, tcp::endpoint endpoint,
                   std::shared_ptr<Tile> &tile_map)
    : ioc(ioc), acceptor(net::make_strand(ioc)), tile_map(tile_map) {
  beast::error_code ec;

  acceptor.open(endpoint.protocol(), ec);
  if (ec) {
    PLOG_ERROR << "Failed to open " << ec.message();
    return;
  }

  acceptor.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    PLOG_ERROR << "Failed to set socket reuse " << ec.message();
    return;
  }

  acceptor.bind(endpoint, ec);
  if (ec) {
    PLOG_ERROR << "Failed to bind " << ec.message();
    return;
  }

  acceptor.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    PLOG_ERROR << "Failed to listen " << ec.message();
    return;
  }
}

void Listener::run() { do_accept(); }

void Listener::do_accept() {
  acceptor.async_accept(
      net::make_strand(ioc),
      beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    PLOG_ERROR << "Failed to accept " << ec.message();
    return;
  }

  PLOG_DEBUG << "Incoming connection..";
  std::make_shared<Session>(std::move(socket), tile_map)->run();
  do_accept();
}

Session::Session(tcp::socket &&socket, std::shared_ptr<Tile> &tile_map)
    : ws(std::move(socket)), tile_map(tile_map) {}

void Session::run() {
  net::dispatch(ws.get_executor(), beast::bind_front_handler(
                                       &Session::on_run, shared_from_this()));
}

void Session::on_run() {
  PLOG_DEBUG << "setting ws options";
  ws.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  ws.set_option(
      websocket::stream_base::decorator([](websocket::response_type &res) {
        res.set(beast::http::field::server, std::string("r-place"));
      }));

  auto &stream = ws.next_layer();

  // verify URI path
  beast::http::async_read(
      stream, buffer, req,
      beast::bind_front_handler(&Session::on_check_path, shared_from_this()));
}

void Session::on_check_path(beast::error_code ec,
                            std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == beast::http::error::end_of_stream)
    return do_close();

  if (ec) {
    PLOG_ERROR << "Failed to read " << ec.message();
    return;
  }
  PLOG_DEBUG << "check path";

  if (websocket::is_upgrade(req)) {
    try {
      auto res = boost::urls::parse_origin_form(req.target());
      PLOG_DEBUG << req.method_string() << req.target() << " parsed url";
      if (!res.has_value()) {
        PLOG_ERROR << req.method_string() << req.target() << " invalid url";
        return do_close();
      }

      PLOG_DEBUG << req.method_string() << req.target() << " checking endpoint";
      auto url = res.value();
      auto segments = url.segments();
      auto endpoint = *segments.begin(); // first segment must be the endpoint

      if (endpoint.compare(ENDPOINT) != 0) {
        PLOG_ERROR << req.method_string() << req.target() << " wrong endpoint";
        auto &stream = ws.next_layer();
        auto msg = not_found(std::move(req));
        beast::async_write(
            stream, std::move(msg),
            beast::bind_front_handler(&Session::on_write, shared_from_this()));
        return;
      }

      PLOG_DEBUG << req.method_string() << req.target() << " accepting?";
      // else accept the websocket
      ws.accept(req);
      on_accept();
      return;

    } catch (std::length_error &e) {
      PLOG_ERROR << req.method_string() << req.target() << " invalid url";
      return do_close();
    } catch (std::exception &e) {
      PLOG_ERROR << req.method_string() << req.target() << " any exception";
      return do_close();
    }

  } else {
    auto &stream = ws.next_layer();
    PLOG_ERROR << req.method_string() << req.target() << " not ws endpoint";
    auto msg = not_found(std::move(req));
    beast::async_write(
        stream, std::move(msg),
        beast::bind_front_handler(&Session::on_write, shared_from_this()));
    return;
  }
}

void Session::on_accept() {
  PLOG_DEBUG << "accepted";
  do_read();
}

void Session::do_read() {
  PLOG_DEBUG << "reading...";
  ws.async_read(
      buffer, beast::bind_front_handler(&Session::on_read, shared_from_this()));
}

void Session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  PLOG_DEBUG << "got message size " << buffer.size();
  if (ec == http::error::end_of_stream) {
    return do_close();
  }
  if (ec) {
    PLOG_ERROR << "Failed to read " << ec.message();
    return;
  }
  handle_messages();
  return;
}

void Session::handle_messages() {
  if (!ws.got_text()) {
    PLOG_ERROR << "Unexpected message type ";
    return;
  }
  PLOG_DEBUG << "handle message";
  auto data = beast::buffers_to_string(buffer.data());

  try {
    auto obj = json::parse(data);
    int x = obj["x"].get<int>();
    int y = obj["y"].get<int>();
    int color = obj["color"].get<int>();
    tile_map->set(x, y, color);
  } catch (json::parse_error &e) {
    PLOG_ERROR << "Failed to parse json " << data << " " << e.what();
    return;
  }

  auto ok = boost::asio::buffer("OK", 2);
  ws.text(true);
  ws.async_write(
      ok, beast::bind_front_handler(&Session::on_write, shared_from_this()));
}

void Session::on_write(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    PLOG_ERROR << "Failed to write " << ec.message();
    return;
  }
  buffer.consume(buffer.size());
  do_read();
}
void Session::do_close() {
  beast::error_code ec;
  auto &stream = ws.next_layer();
  stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  if (ec) {
    PLOG_ERROR << "Could not shutdown socket " << ec.message();
  }
}
