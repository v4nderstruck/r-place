#include "caf/error.hpp"
#include "caf/error_code.hpp"
#include "caf/net/web_socket/acceptor.hpp"
#include <algorithm>
#include <caf/actor_ostream.hpp>
#include <caf/actor_system.hpp>
#include <caf/actor_system_config.hpp>
#include <caf/event_based_actor.hpp>
#include <caf/exec_main.hpp>
#include <caf/function_view.hpp>
#include <caf/init_global_meta_objects.hpp>
#include <caf/net/middleman.hpp>
#include <caf/net/web_socket/with.hpp>
#include <caf/policy/select_all.hpp>
#include <caf/scheduled_actor/flow.hpp>
#include <caf/scoped_actor.hpp>
#include <caf/type_id.hpp>
#include <caf/typed_actor.hpp>
#include <caf/typed_event_based_actor.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>

#define DIM 1000
using namespace caf;
namespace ws = caf::net::web_socket;
using trait = ws::default_trait;

struct CellState {
  int color = 0;
  static constexpr const char *name = "cell";
};
using CanvasCell =
    typed_actor<result<void>(put_atom, int), result<int>(get_atom)>;

CanvasCell::behavior_type
canvas_cell_actor(CanvasCell::stateful_pointer<CellState> self) {
  return {
      [=](put_atom, int val) { self->state.color = val; },
      [=](get_atom) { return self->state.color; },
  };
}

struct MatrixState {
  std::vector<int> bitmap;
  static constexpr const char *name = "matrix";
};
using CanvasMatrix =
    typed_actor<result<int>(put_atom, int, int, int), // x,y, color
                result<int>(get_atom, int, int)       // get color at col
                >;
CanvasMatrix::behavior_type
canvas_matrix_actor(CanvasMatrix::stateful_pointer<MatrixState> self) {
  self->state.bitmap.resize(DIM * DIM);
  return {[=](put_atom put, int x, int y, int color) {
            auto index = x + y * DIM;
            if (index < DIM * DIM) {
              self->state.bitmap[index] = color;
            }
            return color;
          },
          [=](get_atom get, int x, int y) {
            auto index = x + y * DIM;
            if (index < DIM * DIM)
              ;
            return self->state.bitmap[index];
          }};
}

struct SimpleMessage {
  int set_x;
  int set_y;
  int color;
};

void fake_client(event_based_actor *self, CanvasMatrix matrix) {

  auto feed = self->make_observable()
                  .interval(std::chrono::milliseconds(1000))
                  .map([](int64_t) {
                    std::random_device dev;
                    std::mt19937 rng(dev());
                    std::uniform_int_distribution<int> dist(0, DIM - 1);
                    int x = dist(rng);
                    int y = dist(rng);
                    int color = dist(rng);
                    return SimpleMessage{x, y, color};
                  })
                  .share();
  feed.for_each([self, matrix](SimpleMessage msg) {
    aout(self) << "Set x : " << msg.set_x << "y : " << msg.set_y
               << " color : " << msg.color << std::endl;
    self->request(matrix, std::chrono::seconds(10), put_atom_v, msg.set_x,
                  msg.set_y, msg.color)
        .await([=](int result) {
          aout(self) << "Set Color : " << result << std::endl;
        });
    self->request(matrix, std::chrono::seconds(10), get_atom_v, msg.set_x,
                  msg.set_y)
        .await([=](int color) {
          if (color != msg.color)
            aout(self) << "Color does not check out" << std::endl;
        });
  });
}

// todo: Message broker

void websocket_handler(event_based_actor *self,
                       trait::acceptor_resource<> events, CanvasMatrix matrix) {
  using namespace std::literals;
  using json = nlohmann::json;
  auto n = std::make_shared<int>(0);
  

  events.observe_on(self).for_each([self, n,
                                    matrix](const trait::accept_event<> &ev) {
    std::cout << "*** added listener (n = " << ++*n << ")" << std::endl;
    auto [pull, push] = ev.data();
    pull.observe_on(self)
        .do_on_next([&self, matrix](ws::frame frame) {
          if (frame.is_text()) {
            try {
              auto o = json::parse(frame.as_text());
              aout(self) << "Parsed " << o.dump() << std::endl;
              auto x = o.at("x").get<int>();
              auto y = o.at("y").get<int>();
              auto color = o.at("color").get<int>();

              self->request(matrix, 10s, put_atom_v, x, y, color)
                  .await([=](int result) {
                    aout(self) << "Set Color : " << result << std::endl;
                  });
            } catch (const std::exception) {
              aout(self) << "Parsing failed " << frame.as_text() << std::endl;
            }
          }
        })
        .do_finally([n] { //
          std::cout << "*** removed listener (n = " << --*n << ")" << std::endl;
        })
        .subscribe(push);
  });
}

int caf_main(actor_system &sys) {

  auto m = sys.spawn(canvas_matrix_actor);


  auto server = ws::with(sys)
                    .accept(8081)
                    .max_connections(128)
                    .on_request([](ws::acceptor<> &ac) {
                      auto header = ac.header();
                      if (header.path() == "/rplace") {
                        std::cout << "REQUEST " << header.path() << " ACCEPTED"
                                  << std::endl;
                        ac.accept();
                        return;
                      }
                      std::cout << "REQUEST " << header.path() << " DENIED"
                                << std::endl;
                      ac.reject(caf::error());
                    })
                    .start([&sys, &m](trait::acceptor_resource<> events) {
                      sys.spawn(websocket_handler, events, m);
                    });

  if (!server) {
    std::cerr << "*** unable to run : " << to_string(server.error()) << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

CAF_MAIN(caf::net::middleman)
