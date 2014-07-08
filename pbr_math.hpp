#pragma once
#include <assert.h>
#include <math.h>

namespace pbr
{
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

  inline Vector3 Normalize(const Vector3& v) { return v / v.Length(); }
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
  void CreateCoordinateSystem(const Vector3& v1, Vector3* v2, Vector3* v3);

  // return n flipped if it doesn't lie in the same hemispher are v
  inline Vector3 Faceforward(const Vector3& n, const Vector3& v)
  {
    return Dot(v, n) < 0.0f ? -n : n;
  }


}