#include "scene.hpp"

using namespace pbr;

//---------------------------------------------------------------------------
void Scene::Init()
{
  float lumScale = 1.f;
  Color ballDiffuse(0.1f, 0.4f, 0.4f);
  Color ballSpec(0.2f, 0.2f, 0.2f);
  Color ballEmit(0.75f, 0.75f, 0.75f);

  Color planeDiffuse(0.5, 0, 0);
  Color planeSpec(0.1f, 0.1f, 0.1f);
  ballEmit = lumScale * ballEmit;
  Color zero(0, 0, 0);

#define LOAD_MESH 0

  int numBalls = 10;
  for (u32 i = 0; i < numBalls; ++i)
  {
    float angle = i * 2 * Pi / numBalls;
    Geo* g = new Sphere(Vector3(10 * cosf(angle), 1, 30 + 10 * sinf(angle)), 2);
    if (i & 1)
      g->material = new Material(ballDiffuse, ballSpec, zero);
    else
      g->material = new Material(ballDiffuse, ballSpec, ballEmit);
    objects.push_back(g);
  }

  Geo* center = new Sphere(Vector3(0, 50, 30), 15);

  center->material = new Material(ballDiffuse, zero, ballEmit);
  objects.push_back(center);

  Geo* plane = new Plane(Vector3(0, 1, 0), 0);
  plane->material = new Material(planeDiffuse, planeSpec, zero);
  objects.push_back(plane);

  for (Geo* g : objects)
  {
    if (g->material->emissive.Max3() > 0)
      emitters.push_back(g);
  }
}

//---------------------------------------------------------------------------
bool Scene::IntersectClosest(const Ray& r, HitRec* hitRec)
{
  float closest = FLT_MAX;
  float eps = 0.00001f;
  for (Geo* obj : objects)
  {
    if (obj->Intersect(r, hitRec))
    {
      if (hitRec->t < closest && hitRec->t >= eps)
      {
        closest = hitRec->t;
      }
    }
  }

  return closest != FLT_MAX;
}
