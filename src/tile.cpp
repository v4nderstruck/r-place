#import "tile.hpp"
#include <mutex>

Tile::Tile(int x, int y) : x(x), y(y), tile_map(x * y, 0) {}

void Tile::set(int x, int y, int color) {
  if (x >= this->x || y >= this->y || color < 0 || color > 0xFFFFFF)
    return;

  std::lock_guard<std::mutex> lock(mtx);
  tile_map[x + y * this->x] = color;
  PLOG_DEBUG << "Set tile (" << x << ", " << y << ") to " << color;
}
