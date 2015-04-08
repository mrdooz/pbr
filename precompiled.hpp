#pragma once

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <CoreGraphics/CGDirectDisplay.h>
#include <vector>
#include <stdint.h>
#endif

#include <SFML/Graphics.hpp>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

namespace pbr
{
  using sf::RenderWindow;
  using sf::RenderTexture;
  using sf::Sprite;
  using sf::Texture;
  using sf::Event;
  using sf::Vector2f;
  using sf::Vector2i;
  using sf::Vector2u;

  using std::min;
  using std::max;

  using std::vector;
}