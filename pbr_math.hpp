#pragma once
#include <assert.h>
#include <math.h>

namespace pbr
{
  // Note, uses left-handed coordinate system

  const float Pi = 3.14159265359f;

  //---------------------------------------------------------------------------
  inline float randf(float mn, float mx)
  {
    float tmp = rand() / (float)RAND_MAX;
    return mn + (mx - mn) * tmp;
  }

  //---------------------------------------------------------------------------
  inline float DegToRad(float deg)
  {
    return deg / 180.0f * Pi;
  }

  //---------------------------------------------------------------------------
  inline float RadToDeg(float rad)
  {
    return rad / Pi * 180.0f;
  }

  //---------------------------------------------------------------------------
  struct Vector3
  {
    Vector3() {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) { assert(!HasNaNs()); }

    float operator[](int i) const { assert(i >= 0 && i < 2); return (&x)[i]; }
    float& operator[](int i) { assert(i >= 0 && i < 2); return (&x)[i]; }

    Vector3 operator+(const Vector3& rhs) const { return Vector3(x+rhs.x, y+rhs.y, z+rhs.z); }
    Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }

    Vector3 operator-(const Vector3& rhs) const { return Vector3(x-rhs.x, y-rhs.y, z-rhs.z); }
    Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }

    Vector3 operator*(float f) const { return Vector3(f*x, f*y, f*z); }
    Vector3& operator*=(float f) { x *= f; y *= f; z *= f; return *this; }

    Vector3 operator/(float f) const
    { 
      if (f == 0) 
        return Vector3(0,0,0); 
      float r = 1/f; 
      return Vector3(r*x, r*y, r*z); 
    }

    Vector3& operator/=(float f) 
    { 
      if (f == 0) 
      { 
        x = 0; 
        y = 0; 
        z = 0; 
      } 
      else 
      { 
        float r = 1/f; 
        x *= r; 
        y *= r; 
        z *= r; 
      } 
      return *this; 
    }

    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    float LengthSquared() const { return x*x + y*y + z*z; }
    float Length() const { return sqrtf(LengthSquared()); }

    bool HasNaNs() const { return isnan(x) || isnan(y) || isnan(z); }
    float x, y, z;
  };

  //---------------------------------------------------------------------------
  inline Vector3 operator*(float f, const Vector3& v) { return v*f; }

  inline float Dot(const Vector3& lhs, const Vector3& rhs)
  {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
  }

  inline Vector3 Cross(const Vector3& lhs, const Vector3& rhs)
  {
    return Vector3(
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x
      );
  }

  inline float Sq(float x)
  {
    return x * x;
  }

  inline Vector3 Normalize(const Vector3& v) { return v / v.Length(); }

  Vector3 RayInHemisphere(const Vector3& n);
  //---------------------------------------------------------------------------
  struct Ray
  {
    Ray(const Vector3& o, const Vector3& d) : o(o), d(d), minT(0.f), maxT(0.f), time(0.f), depth(0) {}
    Vector3 o;
    Vector3 d;
    float minT, maxT;
    float time;
    int depth;
  };

  //---------------------------------------------------------------------------
  struct Frame
  {
    Frame() {}
    Frame(const Vector3& r, const Vector3& u, const Vector3& d, const Vector3& o) : right(r), up(u), dir(d), origin(o) {}
    Vector3 right, up, dir;
    Vector3 origin;
  };

  //---------------------------------------------------------------------------
  struct Camera
  {
    Frame frame;
    float fov;
    // distance from eye pos to image plane
    float dist;

    float lensWidth;
  };

  //---------------------------------------------------------------------------
  struct Color
  {
    Color() {}
    Color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a) {}

    Color& operator+=(const Color& rhs) { r += rhs.r; g += rhs.g; b += rhs.b; a *= rhs.a; return *this; }
    float r, g, b, a;
  };

  inline Color operator+(const Color& lhs, const Color& rhs)
  {
    return Color(lhs.r+rhs.r, lhs.g+rhs.g, lhs.b+rhs.b, lhs.a*rhs.a);
  }

  inline Color operator/(const Color& c, float s)
  {
    return Color(c.r/s, c.g/s, c.b/s, c.a);
  }

  inline Color operator*(float s, const Color& c)
  {
    return Color(c.r*s, c.g*s, c.b*s, c.a);
  }

  //---------------------------------------------------------------------------
  struct Material
  {
    Material() : diffuse(0,0,0), emissive(0,0,0) {}
    Color diffuse;
    Color emissive;
  };

  struct Geo;
  struct HitRec
  {
    HitRec() : material(nullptr), geo(nullptr) {}
    Vector3 pos;
    Vector3 normal;
    Material* material;
    Geo* geo;
    float t;
  };

  //---------------------------------------------------------------------------
  struct Geo
  {
    virtual ~Geo() {}
    virtual bool Intersect(const Ray& ray, float* tClosest, HitRec* rec) = 0;
    Material material;
  };

  //---------------------------------------------------------------------------
  struct Sphere : public Geo
  {
    Sphere() {}
    Sphere(const Vector3& c, float r) : center(c), radius(r) {}
    virtual bool Intersect(const Ray& ray, float* tClosest, HitRec* rec);
    Vector3 center;
    float radius;
  };

  //---------------------------------------------------------------------------
  struct Plane : public Geo
  {
    Plane() {}
    Plane(const Vector3& n, float d) : normal(n), distance(d) {}
    virtual bool Intersect(const Ray& ray, float* tClosest, HitRec* rec);
    Vector3 normal;
    float distance;
  };

  //---------------------------------------------------------------------------
  void CreateCoordinateSystem(const Vector3& v1, Vector3* v2, Vector3* v3);

  // return n flipped if it doesn't lie in the same hemisphere as v
  inline Vector3 Faceforward(const Vector3& n, const Vector3& v)
  {
    return Dot(v, n) < 0.0f ? -n : n;
  }

  //---------------------------------------------------------------------------
  inline float Randf()
  {
    return rand() / (float)RAND_MAX;
  }

  //---------------------------------------------------------------------------
  // Returns points in [0..1], [0..1] distributed in a Poisson distribution
  struct PoissonSampler
  {
    PoissonSampler();
    void Init(u32 numSamples);
    Vector2f NextSample();
    Vector2f NextDiskSample();

    void MapSamplesToUnitDisk();

    vector<Vector2f> _samples;
    vector<Vector2f> _diskSamples;
    u32 _idx;
    u32 _idxDisk;
  };


}