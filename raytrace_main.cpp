#include "pbr_math.hpp"
#include "scene.hpp"

using namespace pbr;
extern Vector2u windowSize;

Scene scene;

//---------------------------------------------------------------------------
void RayTrace(const Camera& cam, Color* buffer)
{
  scene.Init();

  // Compute size of the image plane. This is the plane at distance d from the
  // camera that we will shoot rays through (without AA, one ray per pixel).
  // The size of the image plane depends on 'd' and the camera fov. For the y
  // size, the aspect ratio also matters.

  float halfWidth = cam.dist * tanf(cam.fov / 2);
  float imagePlaneWidth = 2 * halfWidth;
  float imagePlaneHeight = imagePlaneWidth * windowSize.y / windowSize.x;

  float xInc = imagePlaneWidth / (windowSize.x - 1);
  float yInc = -imagePlaneHeight / (windowSize.y - 1);

//  PoissonSampler sampler;
//  sampler.Init(64);

  // top left corner
  Vector3 p(cam.frame.origin - halfWidth * cam.frame.right + imagePlaneHeight/2 * cam.frame.up + cam.dist * cam.frame.dir);
  Vector3 tmp = p;

  Color* pp = buffer;

  Vector3 lightPos = Vector3{20, 20, 0};

  for (u32 y = 0; y < windowSize.y; ++y)
  {
    p = tmp;
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      Color col(0,0,0);

      // construct ray from eye pos through the image plane
      Ray r(cam.frame.origin, Normalize(p - cam.frame.origin));

      HitRec closest;
      if (scene.IntersectClosest(r, &closest))
      {
        Vector3 n = closest.normal;
        const Material* m = closest.material;
        Vector3 ll = Normalize(lightPos - closest.pos);
        col += Dot(n, ll) * m->diffuse;
      }
      else
      {
        col += Color(0.1f, 0.1f, 0.1f);
      }

      *pp++ = col;

      p.x += xInc;
    }
    tmp.y += yInc;
  }

}

#if 0
//---------------------------------------------------------------------------
void RayTraceOrg(const Camera& cam, Color* buffer)
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
      Color col(0,0,0);
      u32 numSamples = 1;
      for (u32 i = 0; i < numSamples; ++i)
      {
        // construct ray from eye pos through the image plane
        Vector2 ofs = sampler.NextSample();
        Ray r(cam.frame.origin, Normalize((p + Vector3(ofs.x * xInc, -ofs.y * yInc, 0)) - cam.frame.origin));

        HitRec closest;
        if (Intersect(r, &closest))
        {
          Vector3 n = closest.normal;

          // Found intersection, so illuminate via:
          // L(o) = Le(o) + HEMI(BDRF(o, i) * Li(i) * dot(n, i))

          // light ray

          const Material* m = closest.material;
          col += Dot(n, Vector3(0,1,0)) * m->diffuse + m->emissive;
        }
        else
        {
          col += Color(0.1f, 0.1f, 0.1f);
        }
      }

      *pp++ = col / (float)numSamples;
      
      p.x += xInc;
    }
    tmp.y += yInc;
  }
  
}
#endif