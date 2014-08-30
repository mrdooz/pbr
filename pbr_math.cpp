#include "pbr_math.hpp"

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

    rec->pos = ray.o + t * ray.d;
    rec->normal = Normalize(rec->pos - center);
    rec->material = &material;
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

    rec->pos = ray.o + t * ray.d;
    rec->normal = normal;
    rec->material = &material;
    rec->geo = this;
    rec->t = t;
    return true;
  }

}
