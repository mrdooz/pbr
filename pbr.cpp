#include "pbr.hpp"
#include "pbr_math.hpp"

using namespace pbr;

float* backbuffer;
Vector2u windowSize;

vector<Geo*> objects;

int MAX_DEPTH = 3;

void Init()
{
  backbuffer = (float*)malloc(windowSize.x * windowSize.y * 4 * sizeof(float));

  for (u32 i = 0; i < 10; ++i)
  {
    float angle = i * 2 * Pi / 10;
    objects.push_back(new Sphere(Vector3(10 * cos(angle), 0, 30 + 10 * sin(angle)), 2));
    objects.back()->material.diffuse = Color(0.1f, 0.2f, 0.4f);
  }

  Geo* center = new Sphere(Vector3(0, 0, 30), 5);
  center->material.emissive = Color(1, 1, 1);
  objects.push_back(center);

  Geo* plane = new Plane(Vector3(0,1,0), 0);
  plane->material.diffuse = Color(0.5f, 0.5f, 0.5f);
  objects.push_back(plane);
}

void Close()
{
  free(backbuffer);
}

Color Trace(const Ray& r, int depth)
{
  float tMin = 1e20;
  HitRec closest;
  HitRec hitRec;

  for (Geo* obj : objects)
  {
    if (obj->Intersect(r, &hitRec))
    {
      if (hitRec.t < tMin)
      {
        tMin = hitRec.t;
        closest = hitRec;
      }
    }
  }

  if (!closest.geo)
    return Color(0.1f, 0.1f, 0.1f);

  const Material* m = closest.material;
  Color cur = (1.0f - depth/MAX_DEPTH) * (m->diffuse + m->emissive);

  if (depth >= MAX_DEPTH)
  {
    return cur;
  }

  Vector3 p = closest.pos;
  Vector3 n = closest.normal;

  Vector3 v = RayInHemisphere(n);
  return cur + Trace(Ray(p + 1e-4*n, v), depth + 1);
}

void Render(const Camera& cam)
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

  // top left corner
  Vector3 p(cam.frame.origin - halfWidth * cam.frame.right + imagePlaneHeight/2 * cam.frame.up + cam.dist * cam.frame.dir);
  Vector3 tmp = p;

  Color* pp = (Color*)backbuffer;

  for (u32 y = 0; y < windowSize.y; ++y)
  {
    p = tmp;
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      // construct ray from eye pos through the image plane
      Ray r(cam.frame.origin, Normalize(p - cam.frame.origin));

      Color tmp(0,0,0);
      u32 iterations = 50;
      for (u32 i = 0; i < iterations; ++i)
      {
        tmp += Trace(r, 0);
      }

      *pp++ = tmp/(iterations*MAX_DEPTH);


      p.x += xInc;
    }
    tmp.y += yInc;
  }

}

void CopyToWindow(sf::Texture* texture)
{

  vector<sf::Color> buf(windowSize.x * windowSize.y);

  Color* pp = (Color*)backbuffer;

  for (u32 y = 0; y < windowSize.y; ++y)
  {
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      const Color &col = *pp++;
      buf[y * windowSize.x + x].r = col.r * 255;
      buf[y * windowSize.x + x].g = col.g * 255;
      buf[y * windowSize.x + x].b = col.b * 255;
      buf[y * windowSize.x + x].a = col.a * 255;
    }
  }

  texture->update((const sf::Uint8*)buf.data());
}



int main(int argc, char** argv)
{
  u32 width, height;
#ifdef _WIN32
  width = GetSystemMetrics(SM_CXFULLSCREEN);
  height = GetSystemMetrics(SM_CYFULLSCREEN);
#else
  auto displayId = CGMainDisplayID();
  width = CGDisplayPixelsWide(displayId);
  height = CGDisplayPixelsHigh(displayId);
#endif

  sf::ContextSettings settings;
  windowSize = Vector2u(8 * width / 10, 8 * height / 10);
  RenderWindow renderWindow(sf::VideoMode(windowSize.x, windowSize.y), "...", sf::Style::Default, settings);

  Init();

  bool done = false;

  Texture texture;
  texture.create(windowSize.x, windowSize.y);
  Sprite sprite;
  sprite.setTexture(texture);

  Camera cam;
  cam.fov = DegToRad(60);
  cam.dist = 10;
  cam.frame = Frame(Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1), Vector3(0,5,-10));

  renderWindow.clear();
  renderWindow.display();

  renderWindow.clear();
  Render(cam);
  CopyToWindow(&texture);
  renderWindow.draw(sprite);
  renderWindow.display();

  while (!done)
  {
    Event event;
    while (renderWindow.pollEvent(event))
    {
      if (event.type == Event::KeyReleased)
      {
        if (event.key.code == sf::Keyboard::Key::Escape)
        {
          done = true;
        }
      }
    }
  }

  Close();

  return 0;
}