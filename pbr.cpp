#include "pbr.hpp"
#include "pbr_math.hpp"

using namespace pbr;

float* backbuffer;
Vector2u windowSize;

vector<Sphere> spheres;
vector<Plane> planes;

int MAX_DEPTH = 3;

void Init()
{
  backbuffer = (float*)malloc(windowSize.x * windowSize.y * 4 * sizeof(float));

  for (u32 i = 0; i < 10; ++i)
  {
    float angle = i * 2 * Pi / 10;
    spheres.push_back(Sphere(Vector3(10 * cos(angle), 0, 30 + 10 * sin(angle)), 2));
    spheres.back().material.diffuse = Color(0.1f, 0.2f, 0.4f);
  }

  Sphere center(Vector3(0, 0, 30), 5);
  center.material.emissive = Color(1, 1, 1);
  spheres.push_back(center);

  Plane plane(Vector3(0,1,0), 0);
  plane.material.diffuse = Color(0.5f, 0.5f, 0.5f);
  planes.push_back(plane);

}

void Close()
{
  free(backbuffer);
}

float Intersect(const Ray& r, const Plane& p)
{
  float vd = Dot(p.n, r.d);
  if (vd >= 0)
    return -1;

  float v0 = -(Dot(p.n, r.o) + p.d);

  return v0 / vd;
}

float Intersect(const Ray& r, const Sphere& s)
{
  // sphere intersection
  float a = Dot(r.d, r.d);
  float b = 2 * Dot(r.o - s.c, r.d);
  float c = Dot(r.o - s.c, r.o - s.c) - Sq(s.r);

  float disc = Sq(b) - 4 * a * c;
  if (disc < 0)
    return -1;

  disc = sqrtf(disc);
  float t0 = (-b - disc) / 2;
  if (t0 > 0)
    return t0;

  float t1 = (-b + disc) / 2;
  return t1;
}

Vector3 Normal(const Vector3& v, const Sphere& s)
{
  return Normalize(v - s.c);
}

Vector3 Normal(const Vector3& v, const Plane& p)
{
  return p.n;
}

Color Trace(const Ray& r, int depth)
{
  float tMin = 1e20;
  const Sphere* sphere = nullptr;
  const Plane* plane = nullptr;

  for (const Sphere& s : spheres)
  {
    float t = Intersect(r, s);
    if (t > 0 && t < tMin)
    {
      sphere = &s;
      tMin = t;
    }
  }

  for (const Plane& p : planes)
  {
    float t = Intersect(r, p);
    if (t > 0 && t < tMin)
    {
      sphere = nullptr;
      plane = &p;
      tMin = t;
    }
  }

  if (!sphere && !plane)
    return Color(0.1f, 0.1f, 0.1f);

  const Material& m = sphere ? sphere->material : plane->material;

  Color cur = (1.0f - depth/MAX_DEPTH) * (m.diffuse + m.emissive);

  if (depth >= MAX_DEPTH)
  {
    return cur;
  }

  // point on sphere
  Vector3 p = r.o + tMin * r.d;
  Vector3 n = sphere ? Normal(p, *sphere) : Normal(p, *plane);

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
      u32 iterations = 25;
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