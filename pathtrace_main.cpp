#include "pbr_math.hpp"

using namespace pbr;
extern Vector2u windowSize;
extern vector<Geo*> objects;
extern vector<Geo*> emitters;
extern bool Intersect(const Ray& r, HitRec* hitRec);

//---------------------------------------------------------------------------
Color Radiance(const Ray& r, int depth, bool emit)
{
  Color res;

  HitRec hitRec;
  if (!Intersect(r, &hitRec))
    return res;

  Vector3 x = hitRec.pos;
  Vector3 n =  (Dot(r.d, hitRec.normal) < 0 ? 1.f : -1.f) * hitRec.normal;
  Material* mat = hitRec.material;
  Color col = mat->diffuse;
  Color emitCol = emit ? mat->emissive : Color(0,0,0);
  ++depth;

  // Russian roulette based on max specular component
  float p = mat->specular.Max();

  if (++depth > 5 || p == 0.f)
  {
    float r = Randf();
    if (r < p && depth < 20)
      col *= (1 / p);
    else
      return emitCol;
  }

  if (mat->diffuse.Max() > 0)
  {
    float r1 = (float)(2 * M_PI*Randf());
    float r2 = Randf();
    float r2s = sqrtf(r2);
    Vector3 w = n, u, v;
    CreateCoordinateSystem(n, &u, &v);
    Vector3 d = Normalize((u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1 - r2)));

    Color e(0,0,0);
    // sample the emitters
    for (Geo* g : emitters)
    {
      Sphere* s = static_cast<Sphere*>(g);
      Material* sm = s->material;

      // Sample a point on the emitter (this assumes spherical emitters)
      // See Realistic Ray Tracing, pp 197
      Vector3 sw = n, su, sv;
      CreateCoordinateSystem(n, &su, &sv);
      float cos_a_max = sqrtf(1 - s->radius * s->radius / Dot(x - s->center, x - s->center));
      float eps1 = Randf();
      float eps2 = Randf();
      float cos_a = 1 - eps1 + eps1*cos_a_max;
      float sin_a = sqrtf(1 - cos_a*cos_a);
      float phi = (float)(2 * M_PI*eps2);
      Vector3 l = Normalize(su*cos(phi)*sin_a + sv*sin(phi)*sin_a + sw*cos_a);

      // shadow ray
      HitRec shadowHit;
      if (Intersect(Ray(x, l), &shadowHit) && shadowHit.geo == g)
      {
        // omega = pdf (rrt, 198)
        float omega = (float)(2 * M_PI*(1 - cos_a_max));
        // 1/pi for brdf (rrt, 165)
        e = e + (col * sm->emissive * Dot(l, n) * omega) * M_1_PI;
      }
    }
    return emitCol + e + col * Radiance(Ray(x, d), depth, false);
  }

  return Color(1,1,1,1);
}

//---------------------------------------------------------------------------
void PathTrace(const Camera& cam, Color* buffer)
{
  // Note, assumes camera frame is orthonormal

  // Compute size of the image plane. This is the plane at distance d from the
  // camera that we will shoot rays through (without AA, one ray per pixel).
  // The size of the image plane depends on 'd' and the camera fov. For the y
  // size, the aspect ratio also matters.

  float halfWidth = cam.dist * tanf(cam.fov / 2);
  float imagePlaneWidth = 2 * halfWidth;
  float imagePlaneHeight = imagePlaneWidth * windowSize.y / windowSize.x;

  float xInc = imagePlaneWidth / (windowSize.x - 1);
  float yInc = -imagePlaneHeight / (windowSize.y - 1);

  PoissonSampler sampler;
  sampler.Init(64);

  // top left corner
  Vector3 p(cam.frame.origin - halfWidth * cam.frame.right + imagePlaneHeight/2 * cam.frame.up + cam.dist * cam.frame.dir);
  Vector3 tmp = p;

  Color* pp = buffer;

  for (u32 y = 0; y < windowSize.y; ++y)
  {
    p = tmp;
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      // construct ray from eye pos through the image plane
      Ray r(cam.frame.origin, Normalize(p - cam.frame.origin));

      // TODO: all the samples are uniform over the whole pixel. Try a stratisfied approach
      Color col(0,0,0);
      u32 numSamples = 4;
      for (u32 i = 0; i < numSamples; ++i)
      {
        // construct ray from eye pos through the image plane
        Vector2 ofs = sampler.NextSample();
        Ray r(cam.frame.origin, Normalize((p + Vector3(ofs.x * xInc, -ofs.y * yInc, 0)) - cam.frame.origin));

        col += Radiance(r, 0, true);
      }

      *pp++ = col / (float)numSamples;

      p.x += xInc;
    }
    tmp.y += yInc;
  }
}
