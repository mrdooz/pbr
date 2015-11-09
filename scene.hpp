#pragma once
#include "pbr_math.hpp"

namespace pbr
{
  struct Scene
  {
    void Init();
    bool IntersectClosest(const Ray& r, HitRec* hitRec);

    vector<Geo*> objects;
    vector<Geo*> emitters;
  };
}
