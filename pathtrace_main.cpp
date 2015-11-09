#if 0
#include <tbb/tbb.h>
#include "pbr_math.hpp"
#include "pbr.hpp"

using namespace pbr;
extern Vector2u windowSize;
extern vector<Geo*> objects;
extern vector<Geo*> emitters;
extern vector<IsectTri> tris;
extern bool Intersect(const Ray& r, HitRec* hitRec);

//---------------------------------------------------------------------------
Color Radiance(const Ray& r, int depth, bool emit = true)
{
  Color res(0,0,0);

  for (const IsectTri& tri : tris)
  {
    float t, u, v;
    if (RayTriIntersect(r, tri, &t, &u, &v))
      return Color(0.5f, 0.5f, 0.5f);
  }

  HitRec hitRec;
  if (!Intersect(r, &hitRec))
    return res;

  Vector3 x = hitRec.pos;
  Vector3 n = hitRec.normal;
  Vector3 nl =  (Dot(r.d, n) < 0 ? 1.f : -1.f) * n;
  Normalize(nl);
  Material* mat = hitRec.material;

  // Choose either diff or spec
  float diffP = mat->diffuse.Max3();
  float specP = mat->specular.Max3();
  float diffR = Randf() * (diffP + specP);

  bool diffuse = diffR < diffP;

  Color col = diffuse ? mat->diffuse : mat->specular;
  Color emitCol = emit ? mat->emissive : Color(0,0,0);
  ++depth;

  // Russian roulette based on max specular component
  float p = mat->specular.Max3();

  if (depth > 5 || p == 0.f)
  {
    float r = Randf();
    if (r < p && depth < 20)
      col *= (1 / p);
    else
      return emitCol;
  }

  if (diffuse)
  {
    // diffuse
    diffP = diffP / (diffP + specP);

    float r1 = (float)(2 * M_PI*Randf());
    float r2 = Randf();
    float r2s = sqrtf(r2);
    Vector3 w = nl, u, v;
    CreateCoordinateSystem(w, &u, &v);
    Vector3 d = Normalize((u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1 - r2)));

    Color e(0,0,0);
    // sample the emitters
    for (Geo* g : emitters)
    {
      if (g->type != Geo::Type::Sphere)
        continue;
      Sphere* s = static_cast<Sphere*>(g);

      // Sample a point on the emitter (this assumes spherical emitters)
      // See Realistic Ray Tracing, pp 197
      Vector3 sw = Normalize(s->center - x), su, sv;
      CreateCoordinateSystem(sw, &su, &sv);
      float dist = (x - s->center).LengthSquared();
      float cos_a_max = dist <= s->radiusSquared ? 0 : sqrtf(1 - s->radiusSquared / dist);
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
        Material* sm = s->material;

        // omega = pdf (rrt, 198)
        float omega = (float)(2 * M_PI*(1 - cos_a_max));
        // 1/pi for brdf (rrt, 165)
        e = e + (col * sm->emissive * Dot(l, nl) * omega) * (float)M_1_PI;
      }
    }
    return (emitCol + e + col * Radiance(Ray(x, d), depth, false)) / diffP;
  }
  else
  {
    // spec
    specP = specP / (diffP + specP);
    return (mat->emissive + col * Radiance(Ray(x, r.d - n * 2 * Dot(n, r.d)), depth)) / specP;
  }
}

struct ScanlineRender
{
  ScanlineRender(
    const Camera& camera,
    const Vector3& tmp,
    float xInc,
    float yInc, Color* buffer,
    const RenderSettings& settings,
    Vector2* samples)
      : cam(camera), tmp(tmp), xInc(xInc), yInc(yInc), buffer(buffer), settings(settings), samples(samples)
  {
  }

  void operator()(const tbb::blocked_range<int> &r) const
  {
    u32 numSamples = settings.numSamples;
    numSamples = 1;

    // r contains the scan lines to process
    Color* pp = &buffer[r.begin() * windowSize.x];
    Vector3 p;
    Vector3 tmp2 = tmp;
    tmp2.y += yInc * r.begin();

    for (int y = r.begin(); y < r.end(); ++y)
    {
      p = tmp2;

      for (u32 x = 0; x < windowSize.x; ++x)
      {
        // TODO: all the samples are uniform over the whole pixel. Try a stratisfied approach
        Color col(0,0,0);
        for (u32 i = 0; i < numSamples; ++i)
        {
          // construct ray from eye pos through the image plane
          Vector2 ofs = samples[(sampleIdx++) & 0xff];
          Ray r(cam.frame.origin, Normalize((p + Vector3(ofs.x * xInc, ofs.y * yInc, 0)) - cam.frame.origin));

          col += Radiance(r, 0);
        }

        *pp++ = col / (float)numSamples;

        p.x += xInc;
      }
    }
  }

  const Camera& cam;
  Vector3 tmp;
  float xInc, yInc;
  Color* buffer;
  Vector2* samples;
  RenderSettings settings;
  mutable u32 sampleIdx = 0;
};

//---------------------------------------------------------------------------
void PathTrace(const Camera& cam, const RenderSettings& settings, Color* buffer)
{
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
  sampler.Init(256);
  Vector2 samples[256];
  for (int i = 0; i < 256; ++i)
    samples[i] = sampler.NextSample();

  // top left corner
  Vector3 p(cam.frame.origin - halfWidth * cam.frame.right + imagePlaneHeight / 2 * cam.frame.up + cam.dist * cam.frame.dir);

  tbb::parallel_for(tbb::blocked_range<int>(0, windowSize.y),
    ScanlineRender(cam, p, xInc, yInc, buffer, settings, samples),
    tbb::auto_partitioner());
}
#endif