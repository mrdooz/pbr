#include "pbr_math.hpp"
#include <algorithm>
using namespace std;

namespace pbr
{

  //---------------------------------------------------------------------------
  void CreateCoordinateSystem(const Vector3& v1, Vector3* v2, Vector3* v3)
  {
    // create a coordinate system from v1 (which should be normalized)

    if (fabsf(v1.x) > fabs(v1.y))
    {
      // zero v1.y
      float invLen = 1.0f / sqrtf(v1.x*v1.x + v1.z*v1.z);
      *v2 = Vector3(-v1.z * invLen, 0, v1.x * invLen);
    }
    else
    {
      // zero v1.x
      float invLen = 1.0f / sqrtf(v1.y*v1.y + v1.z*v1.z);
      *v2 = Vector3(0, v1.z * invLen, -v1.y * invLen);
    }

    *v3 = Cross(v1, *v2);
  }

  //---------------------------------------------------------------------------
  Vector3 RayInHemisphere(const Vector3& n)
  {
    while (true) {
      float x = -1 + 2 * Randf();
      float y = -1 + 2 * Randf();
      float z = -1 + 2 * Randf();
      if (Sq(x) + Sq(y) + Sq(z) < 1)
        return Faceforward(Normalize(Vector3(x, y, z)), n);
    }
  }

  //---------------------------------------------------------------------------
  bool Sphere::Intersect(const Ray& ray, float* tClosest, HitRec* rec)
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

    if (tClosest)
    {
      if (t >= *tClosest)
        return false;
      *tClosest = t;
    }

    rec->pos = ray.o + t * ray.d;
    rec->normal = Normalize(rec->pos - center);
    rec->material = &material;
    rec->geo = this;
    rec->t = t;
    return true;
  }

  //---------------------------------------------------------------------------
  bool Plane::Intersect(const Ray& ray, float* tClosest, HitRec* rec)
  {
    float vd = Dot(normal, ray.d);
    if (vd >= 0)
      return false;

    float v0 = -(Dot(normal, ray.o) + distance);
    float t = v0 / vd;

    if (tClosest)
    {
      if (t >= *tClosest)
        return false;
      *tClosest = t;
    }

    rec->pos = ray.o + t * ray.d;
    rec->normal = normal;
    rec->material = &material;
    rec->geo = this;
    rec->t = t;
    return true;
  }

  //---------------------------------------------------------------------------
  PoissonSampler::PoissonSampler()
      : _idx(0)
      , _idxDisk(0)
  {
  }

  //---------------------------------------------------------------------------
  void PoissonSampler::Init(u32 numSamples)
  {
    int subCellsX = numSamples * 4;
    int subCellsY = numSamples * 4;

    // 2d space is divided into sqrt(numSamples) grid cells, and each grid cell further
    // divided into 4x4 cells
    vector<float> d(subCellsX*subCellsY);

    for (u32 i = 1; i < subCellsY; ++i)
    {
      for (u32 j = 1; j < subCellsX; ++j)
      {
        // calc T value for current cell
        float t = (4 * d[(j - 1) + (i) * subCellsX] + d[(j - 1) + (i - 1) * subCellsX] + 2 * d[(j) + (i - 1) * subCellsX] + d[(j + 1) + (i - 1) * subCellsX]) / 8;
        t += randf(1.f / 16 - 1.f / 64, 1.f / 16 + 1.f / 64);

        // determine if the current cell should have a pixel
        float s = t < 0.5f ? 0 : 1;
        d[j + i * subCellsX] = t - s;

        if (s > 0)
        {
          float x = (j-1) / (float)(subCellsX-2);
          float y = (i-1) / (float)(subCellsY-2);
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
      Vector2f s = 2.f * _samples[i] - Vector2f(-1, -1);
      float r, phi;

      if (s.x > -s.y)
      {
        if (s.x > s.y)
        {
          r = s.x;
          phi = s.y  / s.x;
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
  Vector2f PoissonSampler::NextSample()
  {
    u32 tmp = _idx;
    _idx = (_idx + 1) % _samples.size();
    return _samples[tmp];
  }

  //---------------------------------------------------------------------------
  Vector2f PoissonSampler::NextDiskSample()
  {
    u32 tmp = _idxDisk;
    _idxDisk = (_idxDisk + 1) % _diskSamples.size();
    return _diskSamples[tmp];
  }

}
