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
 
}
