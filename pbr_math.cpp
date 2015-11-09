#include "pbr_math.hpp"
#include <algorithm>
using namespace std;

namespace pbr
{
  //---------------------------------------------------------------------------
  void Camera::LookAt(const Vector3& pos, const Vector3& up, const Vector3& target)
  {
    Vector3 d = Normalize(target - pos);
    Vector3 r = Normalize(Cross(up, d));
    d = Cross(r, up);
    frame = Frame(r, up, d, pos);
  }

  //---------------------------------------------------------------------------
  float Vector3::Max() const { return max(x, max(y, z)); }

  //---------------------------------------------------------------------------
  float Vector4::Max() const { return max(x, max(y, max(z, w))); }

  //---------------------------------------------------------------------------
  float Vector4::Max3() const { return max(x, max(y, z)); }

  //---------------------------------------------------------------------------
  void CreateCoordinateSystem(const Vector3& v1, Vector3* v2, Vector3* v3)
  {
    // create a coordinate system from v1 (which should be normalized)

    if (fabsf(v1.x) > fabs(v1.y))
    {
      // zero v1.y
      float invLen = 1.0f / sqrtf(v1.x * v1.x + v1.z * v1.z);
      *v2 = Vector3(-v1.z * invLen, 0, v1.x * invLen);
    }
    else
    {
      // zero v1.x
      float invLen = 1.0f / sqrtf(v1.y * v1.y + v1.z * v1.z);
      *v2 = Vector3(0, v1.z * invLen, -v1.y * invLen);
    }

    *v3 = Cross(v1, *v2);
  }

  //---------------------------------------------------------------------------
  Vector3 RayInHemisphere(const Vector3& n)
  {
    while (true)
    {
      float x = -1 + 2 * Randf();
      float y = -1 + 2 * Randf();
      float z = -1 + 2 * Randf();
      if (Sq(x) + Sq(y) + Sq(z) < 1)
        return Faceforward(Normalize(Vector3(x, y, z)), n);
    }
  }

  //---------------------------------------------------------------------------
  bool Sphere::Intersect(const Ray& ray, HitRec* rec)
  {
    // sphere intersection
    float a = Dot(ray.d, ray.d);
    float b = 2 * Dot(ray.o - center, ray.d);
    float c = Dot(ray.o - center, ray.o - center) - Sq(radius);

    float disc = Sq(b) - 4 * a * c;
    if (disc < 0)
      return false;

    disc = sqrtf(disc);
    float t0 = (-b - disc) / 2;
    float t = t0;
    if (t0 <= 0)
    {
      float t1 = (-b + disc) / 2;
      if (t1 <= 0)
        return false;
      t = t1;
    }

    if (t >= rec->t)
      return false;
    rec->t = t;

    rec->pos = ray.o + t * ray.d;
    rec->normal = Normalize(rec->pos - center);
    rec->material = material;
    rec->geo = this;
    rec->t = t;
    return true;
  }

  //---------------------------------------------------------------------------
  bool Plane::Intersect(const Ray& ray, HitRec* rec)
  {
    float vd = Dot(normal, ray.d);
    if (vd >= 0)
      return false;

    float v0 = -(Dot(normal, ray.o) + distance);
    float t = v0 / vd;

    if (t >= rec->t)
      return false;
    rec->t = t;

    rec->pos = ray.o + t * ray.d;
    rec->normal = normal;
    rec->material = material;
    rec->geo = this;
    rec->t = t;
    return true;
  }

  //---------------------------------------------------------------------------
  Vector2 RandomSampler::NextSample() { return Vector2(randf(-1.f, 1.f), randf(-1.f, 1.f)); }

  //---------------------------------------------------------------------------
  Vector2 RandomSampler::NextDiskSample()
  {
    while (true)
    {
      Vector2 v(randf(-1.f, 1.f), randf(-1.f, 1.f));
      float r = v.Length();
      if (r <= 1.f)
        return v;
    }
  }

  //---------------------------------------------------------------------------
  UniformSampler::UniformSampler() : _idx(0) {}

  void UniformSampler::Init(u32 numSamples)
  {
    int s = (int)sqrt(numSamples);

    float y = -1.f;
    float inc = 2.f / s;
    int idx = 0;
    _samples.resize(numSamples);

    for (int i = 0; i < s; ++i)
    {
      float x = -1.f;
      for (int j = 0; j < s; ++j)
      {
        _samples[idx++] = Vector2(x, y);
        x += inc;
      }
      y += inc;
    }
  }

  Vector2 UniformSampler::NextSample()
  {
    u32 tmp = _idx;
    _idx = (_idx + 1) % (u32)_samples.size();
    return _samples[tmp];
  }

  Vector2 UniformSampler::NextDiskSample() { return Vector2(0, 0); }

  //---------------------------------------------------------------------------
  PoissonSampler::PoissonSampler() : _idx(0), _idxDisk(0) {}

  //---------------------------------------------------------------------------
  void PoissonSampler::Init(u32 numSamples)
  {
    s64 subCellsX = numSamples * 4;
    s64 subCellsY = numSamples * 4;

    // 2d space is divided into sqrt(numSamples) grid cells, and each grid cell further
    // divided into 4x4 cells
    vector<float> d(subCellsX * subCellsY);

    for (s64 i = 1; i < subCellsY; ++i)
    {
      for (s64 j = 1; j < subCellsX; ++j)
      {
        // calc T value for current cell
        float t = (4 * d[(j - 1) + (i)*subCellsX] + d[(j - 1) + (i - 1) * subCellsX]
                      + 2 * d[(j) + (i - 1) * subCellsX]
                      + d[(j + 1) + (i - 1) * subCellsX])
                  / 8;
        t += randf(1.f / 16 - 1.f / 64, 1.f / 16 + 1.f / 64);

        // determine if the current cell should have a pixel
        float s = t < 0.5f ? 0.f : 1.f;
        d[j + i * subCellsX] = t - s;

        if (s > 0)
        {
          float x = -1 + 2 * ((j - 1) / (float)(subCellsX - 2));
          float y = -1 + 2 * ((i - 1) / (float)(subCellsY - 2));
          _samples.push_back({x, y});
        }
      }
    }

    random_shuffle(_samples.begin(), _samples.end());
    MapSamplesToUnitDisk();
  }

  //---------------------------------------------------------------------------
  void PoissonSampler::MapSamplesToUnitDisk()
  {
    // from "Ray Tracing from the Ground Up", page 123
    for (u32 i = 0; i < _diskSamples.size(); ++i)
    {
      Vector2 s = 2.f * _samples[i] - Vector2(-1, -1);
      float r, phi;

      if (s.x > -s.y)
      {
        if (s.x > s.y)
        {
          r = s.x;
          phi = s.y / s.x;
        }
        else
        {
          r = s.y;
          phi = 2 - s.x / s.y;
        }
      }
      else
      {
        if (s.x < s.y)
        {
          r = -s.x;
          phi = 4 + s.y / s.x;
        }
        else
        {
          r = -s.y;
          phi = s.y == 0 ? 0 : 6 - s.x / s.y;
        }
      }

      phi *= Pi / 4;
      _diskSamples.push_back({r * cosf(phi), r * sinf(phi)});
    }

    random_shuffle(_diskSamples.begin(), _diskSamples.end());
  }

  //---------------------------------------------------------------------------
  Vector2 PoissonSampler::NextSample()
  {
    // todo: make this thread safe
    u32 tmp = _idx;
    _idx = (_idx + 1) % _samples.size();
    return _samples[tmp];
  }

  //---------------------------------------------------------------------------
  Vector2 PoissonSampler::NextDiskSample()
  {
    u32 tmp = _idxDisk;
    _idxDisk = (_idxDisk + 1) % _diskSamples.size();
    return _diskSamples[tmp];
  }

  //---------------------------------------------------------------------------
  bool RayTriIntersect(const Ray& ray, const IsectTri& tri, float* t, float* u, float* v)
  {
    // Moller/Trombore

    const float eps = -1.e5;

    const Vector3& d = ray.d;
    const Vector3& o = ray.o;

    Vector3 e1 = tri.p1 - tri.p0;
    Vector3 e2 = tri.p2 - tri.p0;

    // note, this can be written as e1 . (rd x e2 ) = rd . (e2 x e1), so we
    // can precompute the cross product
    Vector3 q = Cross(d, e2);
    float a = Dot(e1, q);

    if (fabs(a) <= eps)
      return false;

    float f = 1 / a;
    Vector3 s = o - tri.p0;

    *u = f * Dot(s, q);
    if (*u < 0.f)
      return false;

    Vector3 r = Cross(s, e1);

    *v = f * Dot(d, r);
    if (*v < 0.f || *u + *v > 1.f)
      return false;

    *t = f * Dot(e2, r);

    return true;
  }
}
