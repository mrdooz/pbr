#pragma once
#include <assert.h>
#include <math.h>
#include "precompiled.hpp"

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

  inline float Clamp(float v)
  {
    return min(1.f, max(0.f, v));
  }

  //---------------------------------------------------------------------------
  inline float RadToDeg(float rad)
  {
    return rad / Pi * 180.0f;
  }

  struct Vector2
  {
    Vector2() {}
    Vector2(float x, float y) : x(x), y(y) {}

    float operator[](int i) const { assert(i == 0 || i == 1); return (&x)[i]; }
    float& operator[](int i) { assert(i == 0 || i == 1); return (&x)[i]; }

    Vector2 operator+(const Vector2& rhs) const { return Vector2(x+rhs.x, y+rhs.y); }
    Vector2& operator+=(const Vector2& rhs) { x += rhs.x; y += rhs.y; return *this; }

    Vector2 operator-(const Vector2& rhs) const { return Vector2(x-rhs.x, y-rhs.y); }
    Vector2& operator-=(const Vector2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }

    Vector2 operator*(float f) const { return Vector2(f*x, f*y); }

    float LengthSquared() const { return x*x + y*y; }
    float Length() const { return sqrtf(LengthSquared()); }

    float x, y;
  };

  inline Vector2 operator*(float f, const Vector2& v) { return v*f; }

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
      *this = *this / f;
      return *this;
    }

    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    float LengthSquared() const { return x*x + y*y + z*z; }
    float Length() const { return sqrtf(LengthSquared()); }

    bool HasNaNs() const { return isnan(x) || isnan(y) || isnan(z); }
    float Max() const;
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

  //---------------------------------------------------------------------------
  struct Vector4
  {
    Vector4() {}
    Vector4(float x, float y, float z, float w = 1) : x(x), y(y), z(z), w(w) { assert(!HasNaNs()); }

    float operator[](int i) const { assert(i >= 0 && i < 3); return (&x)[i]; }
    float& operator[](int i) { assert(i >= 0 && i < 3); return (&x)[i]; }

    Vector4 operator+(const Vector4& rhs) const { return Vector4(x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w); }
    Vector4& operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }

    Vector4 operator-(const Vector4& rhs) const { return Vector4(x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w); }
    Vector4& operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; return *this; }

    Vector4 operator*(float f) const { return Vector4(f*x, f*y, f*z, f*w); }
    Vector4& operator*=(float f) { x *= f; y *= f; z *= f; w *= f; return *this; }

    Vector4 operator/(float f) const
    {
      if (f == 0)
        return Vector4(0,0,0, 0);
      float r = 1/f;
      return Vector4(r*x, r*y, r*z, r*w);
    }

    Vector4& operator/=(float f)
    {
      *this = *this / f;
      return *this;
    }


    bool HasNaNs() const { return isnan(x) || isnan(y) || isnan(z) || isnan(w); }
    float Max() const;
    float Max3() const;
    union {
      struct { float x; float y; float z; float w; };
      struct { float r; float g; float b; float a; };
    };
  };

  inline Vector4 operator*(float f, const Vector4& v) { return v*f; }
  inline Vector4 operator*(const Vector4& lhs, const Vector4& rhs)
  {
    return Vector4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
  }
  typedef Vector4 Color;

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
    void LookAt(const Vector3& pos, const Vector3& up, const Vector3& target);
    Frame frame;
    float fov;
    // distance from eye pos to image plane
    float dist;

    float lensWidth;
  };

//  //---------------------------------------------------------------------------
//  struct Color
//  {
//    Color() : r(0), g(0), b(0), a(0) {}
//    Color(float r, float g, float b, float a = 1) : r(r), g(g), b(b), a(a) {}
//
//    Color& operator+=(const Color& rhs) { r += rhs.r; g += rhs.g; b += rhs.b; a *= rhs.a; return *this; }
//    float r, g, b, a;
//  };
//
//  inline Color operator+(const Color& lhs, const Color& rhs)
//  {
//    return Color(lhs.r+rhs.r, lhs.g+rhs.g, lhs.b+rhs.b, lhs.a*rhs.a);
//  }
//
//  inline Color operator/(const Color& c, float s)
//  {
//    return Color(c.r/s, c.g/s, c.b/s, c.a);
//  }
//
//  inline Color operator*(float s, const Color& c)
//  {
//    return Color(c.r*s, c.g*s, c.b*s, c.a);
//  }

  //---------------------------------------------------------------------------
  struct Material
  {
    Material(const Color& diffuse, const Color& specular, const Color& emissive)
        : diffuse(diffuse)
        , specular(specular)
        , emissive(emissive) {}
    Color emissive;
    Color diffuse;
    Color specular;
  };

//  //---------------------------------------------------------------------------
//  struct DiffuseMaterial : public Material
//  {
//    DiffuseMaterial(const Color& diffuse, const Color& emissive = Color(0,0,0)) : Material(diffuse, diffuse, emissive) {}
//  };

  //---------------------------------------------------------------------------
  struct Geo;
  struct HitRec
  {
    HitRec() : material(nullptr), geo(nullptr), t(FLT_MAX) {}
    Vector3 pos;
    Vector3 normal;
    Material* material;
    Geo* geo;
    float t;
  };

  //---------------------------------------------------------------------------
  struct Geo
  {
    enum class Type { Sphere, Plane };
    Geo(Type type) : type(type) {}
    virtual ~Geo() {}
    virtual bool Intersect(const Ray& ray, HitRec* rec) = 0;
    Material* material = nullptr;
    Type type;
  };

  //---------------------------------------------------------------------------
  struct Sphere : public Geo
  {
    Sphere() : Geo(Geo::Type::Sphere) {}
    Sphere(const Vector3& c, float r) : Geo(Geo::Type::Sphere), center(c), radius(r), radiusSquared(r*r) {}
    virtual bool Intersect(const Ray& ray, HitRec* rec);
    Vector3 center;
    float radius;
    float radiusSquared;
  };

  //---------------------------------------------------------------------------
  struct Plane : public Geo
  {
    Plane() : Geo(Geo::Type::Plane) {}
    Plane(const Vector3& n, float d) : Geo(Geo::Type::Plane), normal(n), distance(d) {}
    virtual bool Intersect(const Ray& ray, HitRec* rec);
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
  struct Sampler
  {
    virtual ~Sampler() {}
    virtual void Init(u32 numSamples) {};
    virtual Vector2 NextSample() = 0;
    virtual Vector2 NextDiskSample() = 0;
  };

  //---------------------------------------------------------------------------
  struct RandomSampler : public Sampler
  {
    virtual Vector2 NextSample();
    virtual Vector2 NextDiskSample();
  };

  struct UniformSampler : public Sampler
  {
    UniformSampler();
    virtual void Init(u32 numSamples);
    virtual Vector2 NextSample();
    virtual Vector2 NextDiskSample();

    vector<Vector2> _samples;
    u32 _idx;
  };

  //---------------------------------------------------------------------------
  // Returns points in [0..1], [0..1] distributed in a Poisson distribution
  struct PoissonSampler : public Sampler
  {
    PoissonSampler();
    virtual void Init(u32 numSamples);
    virtual Vector2 NextSample();
    virtual Vector2 NextDiskSample();

    void MapSamplesToUnitDisk();

    vector<Vector2> _samples;
    vector<Vector2> _diskSamples;
    u32 _idx;
    u32 _idxDisk;
  };


}