#include "pbr.hpp"
#include "pbr_math.hpp"

using namespace pbr;

float* backbuffer;
Vector2u windowSize;

Camera camera;

void Init()
{
  backbuffer = (float*)malloc(windowSize.x * windowSize.y * 4 * sizeof(float));

  camera.frame = Frame(Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1), Vector3(0,0,0));
  camera.fov = DegToRad(30);
}

void Close()
{
  free(backbuffer);
}

Color Trace(const Ray& r)
{
  Color res(0.1f,0.1f,0.1f);

  Vector3 center(-5, 0, 20);
  float radius = 2;

  // sphere intersection
  float a = Dot(r.d, r.d);
  float b = 2 * Dot(r.o - center, r.d);
  float c = Dot(r.o - center, r.o - center) - Sq(radius);

  float disc = Sq(b) - 4 * a * c;
  if (disc < 0)
    return res;

  disc = sqrtf(disc);
  float t0 = (-b - disc) / 2;
  float t1 = (-b + disc) / 2;

  if (max(t0, t1) > 0)
    return Color(1,1,1);

  return res;
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

      *pp++ = Trace(r);

      p.x += xInc;
    }
    tmp.y += yInc;
  }

}

void CopyToWindow(Texture* texture)
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
  cam.dist = 1;
  cam.frame = Frame(Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1), Vector3(0,0,0));

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

    renderWindow.clear();
    Render(cam);
    CopyToWindow(&texture);
    renderWindow.draw(sprite);
    renderWindow.display();
  }

  Close();

  return 0;
}