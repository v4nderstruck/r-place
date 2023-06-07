#ifndef TILE_HPP
#define TILE_HPP
#include <mutex>
#include <plog/Log.h>
#include <vector>

class Tile {
public:
  Tile(int x, int y);
  // set a Tile to a color
  void set(int x, int y, int color);

private:
  int x; // width
  int y; // height
  std::mutex mtx;
  std::vector<int> tile_map; // holds colors, 3 bytes for RGB required
};

#endif // TILE_HPP
