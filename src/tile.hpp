#ifndef TILE_HPP
#define TILE_HPP
#include "flatbuffers/buffer.h"
#include "protocol_generated.h"
#include <boost/date_time.hpp>
#include <boost/signals2.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <mutex>
#include <plog/Log.h>
#include <vector>

namespace signals = boost::signals2;
namespace posix_time = boost::posix_time;

#define UPDATE_INTERVAL posix_time::milliseconds(1000)
#define UPDATE_N_THRESH 10

/// todo: add write ahead log for better tile updates and replication later
class Tile : boost::enable_shared_from_this<Tile> {
public:
  Tile(int x, int y);
  // set a Tile to a color
  void set(int x, int y, u_int32_t color);
  signals::connection
  connect(const signals::signal<void()>::slot_type &subscriber);

  flatbuffers::Offset<protocol::TileMapUpdate>
  serialize(flatbuffers::FlatBufferBuilder &builder);

private:
  posix_time::ptime last_update;
  int count_updates;
  int x; // width
  int y; // height
  std::mutex mtx;
  std::vector<u_int32_t> tile_map; // holds colors, 3 bytes for RGB required
  signals::signal<void()> sig;
};

#endif // TILE_HPP
