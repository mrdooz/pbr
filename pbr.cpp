#include "pbr.hpp"
#include "pbr_math.hpp"

using namespace pbr;

int main(int argc, char** argv)
{
  Vector3 a;
  a[0] = 0;
  a[1] = 1;

  a *= 10;

  auto x = 10 * a;

  return 0;
}