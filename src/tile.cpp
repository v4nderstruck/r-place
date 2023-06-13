#import "tile.hpp"
#include "flatbuffers/buffer.h"
#include "flatbuffers/flatbuffer_builder.h"
#include "protocol_generated.h"
#include <boost/asio/buffer.hpp>
#include <mutex>
#include <vector>

Tile::Tile(int x, int y)
    : x(x), y(y), tile_map(x * y, 0), count_updates(0),
      last_update(posix_time::second_clock::local_time()) {}

void Tile::set(int x, int y, u_int32_t color) {
  if (x >= this->x || y >= this->y || color < 0 || color > 0xFFFFFFFF){

    PLOG_ERROR << "Invalid values";
    return;
  }
  std::lock_guard<std::mutex> lock(mtx);
  tile_map[x + (y * this->x)] = color;
  PLOG_DEBUG << "Set tile (" << x << ", " << y << ") to " << color;
  count_updates++;
  auto now = posix_time::second_clock::local_time();
  if ((now - last_update).total_milliseconds() > UPDATE_INTERVAL_MS ||
      count_updates >= UPDATE_N_THRESH) {
    sig();
    last_update = now;
    count_updates = 0;
  }
}

signals::connection
Tile::connect(const signals::signal<void()>::slot_type &subscriber) {
  return sig.connect(subscriber);
}

flatbuffers::Offset<protocol::TileMapUpdate>
Tile::serialize(flatbuffers::FlatBufferBuilder &builder) {
  auto f_tile_map =
      builder.CreateVector(reinterpret_cast<uint8_t *>(tile_map.data()),
                           tile_map.size() * sizeof(u_int32_t));
  auto update = protocol::CreateTileMapUpdate(builder, x, y, f_tile_map);
  return update;
}
