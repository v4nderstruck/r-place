#import "tile.hpp"
#include <boost/asio/buffer.hpp>
#include <vector>
#include <mutex>

Tile::Tile(int x, int y)
    : x(x), y(y), tile_map(x * y, 0), count_updates(0),
      last_update(posix_time::second_clock::local_time()) {}

void Tile::set(int x, int y, int color) {
  if (x >= this->x || y >= this->y || color < 0 || color > 0xFFFFFF)
    return;
  std::lock_guard<std::mutex> lock(mtx);
  tile_map[x + y * this->x] = color;
  PLOG_DEBUG << "Set tile (" << x << ", " << y << ") to " << color;
  count_updates++;
  auto now = posix_time::second_clock::local_time();
  if (now - last_update > UPDATE_INTERVAL || count_updates >= UPDATE_N_THRESH) {
    sig();
    last_update = now;
    count_updates = 0;
  }
}

signals::connection
Tile::connect(const signals::signal<void()>::slot_type &subscriber) {
  return sig.connect(subscriber);
}

boost::asio::mutable_buffer Tile::get_tile() {
  auto buf = boost::asio::buffer(this->tile_map.data(), tile_map.size() * sizeof(int));
  return buf;
    
}
