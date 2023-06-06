#include "httpserver.hpp"
#include <algorithm>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffers_generator.hpp>
#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/url.hpp>
#include <boost/url/src.hpp>
#include <chrono>
#include <iterator>
#include <memory>
#include <stdexcept>

#define ENDPOINT "tile"

template <class Body, class Allocator>
http::message_generator
handle_req(std::shared_ptr<Tile> &tile_map,
           http::request<Body, http::basic_fields<Allocator>> &&req) {
  // Returns a bad request response
  auto const bad_request = [&req]() {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  auto const not_found = [&req]() {
    http::response<http::string_body> res{http::status::not_found,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  };

  auto const server_error = [&req]() {
    http::response<http::string_body> res{http::status::internal_server_error,
                                          req.version()};
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return res;
  };

  if (req.method() != http::verb::post) {
    PLOG_INFO << req.method_string() << " " << req.target() << " wrong method";
    return bad_request();
  }
  try {
    auto res = boost::urls::parse_origin_form(req.target());

    if (res.has_value()) {
      boost::url url = *res;

      auto segments = url.segments();
      if (segments.size() != 4) {
        PLOG_INFO << req.method_string() << " " << req.target()
                  << " wrong path, segment size";
        return not_found();
      }

      auto it = segments.begin();
      if ((*it).compare(ENDPOINT) != 0) {
        PLOG_INFO << req.method_string() << " " << req.target()
                  << " wrong endpoint ";
        return not_found();
      }
      it++;
      auto x = std::stoi(*it);
      it++;
      auto y = std::stoi(*it);
      it++;
      auto c = std::stoi(*it);
      it++;

      PLOG_INFO << req.method_string() << " " << req.target() << " OK";
      tile_map->set(x, y, c);

      http::response<http::empty_body> res{};
      res.content_length(0);
      return res;
    } else {
      return not_found();
    }
  } catch (std::length_error &e) {
    PLOG_INFO << req.method_string() << " " << req.target() << " wrong path";
    return not_found();
  } catch (std::invalid_argument &e) {
    PLOG_INFO << req.method_string() << " " << req.target() << " bad arguments";
    return bad_request();
  } catch (std::out_of_range &e) {
    PLOG_INFO << req.method_string() << " " << req.target() << " bad arguments";
    return bad_request();
  } catch (std::exception &e) {
    PLOG_INFO << req.method_string() << " " << req.target() << " error";
    return server_error();
  }
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
  std::make_shared<Session>(std::move(socket), tile_map)->run();
  do_accept();
}

Session::Session(tcp::socket &&socket, std::shared_ptr<Tile> &tile_map)
    : stream(std::move(socket)), tile_map(tile_map) {}
void Session::run() {
  net::dispatch(
      stream.get_executor(),
      beast::bind_front_handler(&Session::do_read, shared_from_this()));
}
void Session::do_read() {
  req = {};
  stream.expires_after(std::chrono::seconds(30));
  http::async_read(
      stream, buffer, req,
      beast::bind_front_handler(&Session::on_read, shared_from_this()));
}
void Session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec == http::error::end_of_stream) {
    return do_close();
  }
  if (ec) {
    PLOG_ERROR << "Failed to read " << ec.message();
    return;
  }
  send_response(handle_req(tile_map, std::move(req)));
  return;
}

void Session::send_response(http::message_generator &&msg) {
  bool keep_alive = msg.keep_alive();
  beast::async_write(stream, std::move(msg),
                     beast::bind_front_handler(&Session::on_write,
                                               shared_from_this(), keep_alive));
}

void Session::on_write(bool keep_alive, beast::error_code ec,
                       std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    PLOG_ERROR << "Failed to write " << ec.message();
    return;
  }
  if (!keep_alive) {
    return do_close();
  }
  do_read();
}
void Session::do_close() {
  beast::error_code ec;
  stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  if (ec) {
    PLOG_ERROR << "Could not shutdown socket " << ec.message();
  }
}
