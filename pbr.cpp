#include "pbr.hpp"
#include "pbr_math.hpp"


using namespace pbr;

Vector2u windowSize;
vector<Geo*> objects;
vector<Geo*> emitters;

void PathTrace(const Camera& cam, Color* buffer);

int MAX_DEPTH = 3;

struct Buffer
{
  Buffer(int width, int height) : width(width), height(height), buffer(new Color[width*height]) {}
  ~Buffer() { delete [] buffer; }
  Buffer(const Buffer&) = delete;
  Buffer& operator=(Buffer&) = delete;

  int width, height;
  Color* buffer;
};

Buffer* backbuffer;

//---------------------------------------------------------------------------
void Init()
{
  backbuffer = new Buffer(windowSize.x, windowSize.y);

  for (u32 i = 0; i < 10; ++i)
  {
    float angle = i * 2 * Pi / 10;
    Geo* g = new Sphere(Vector3(10 * cosf(angle), 0, 30 + 10 * sinf(angle)), 2);
    if (i & 1)
      g->material = new DiffuseMaterial(Color(0.1f, 0.2f, 0.4f));
    else
      g->material = new DiffuseMaterial(Color(0.1f, 0.2f, 0.4f), 1.f * Color(0.75f, 0.75f, 0.75f));
    objects.push_back(g);
  }

  Geo* center = new Sphere(Vector3(0, 0, 30), 5);

  center->material = new DiffuseMaterial(Color(0,0.2,0), 100.f * Color(0.75f, 0.75f, 0.75f));
  objects.push_back(center);

  Geo* plane = new Plane(Vector3(0,1,0), 0);
  plane->material = new DiffuseMaterial(Color(0.5f, 0.5f, 0.5f));
  objects.push_back(plane);

  for (Geo* g : objects)
  {
    if (g->material->emissive.Max() > 0)
      emitters.push_back(g);
  }

}

//---------------------------------------------------------------------------
void Close()
{
  free(backbuffer);
}

//---------------------------------------------------------------------------
void BufferToTexture(Buffer* buffer, sf::Texture *texture)
{
  vector<sf::Color> buf(windowSize.x * windowSize.y);

  Color* pp = buffer->buffer;
  for (u32 y = 0; y < windowSize.y; ++y)
  {
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      const Color &col = *pp++;
      buf[y * windowSize.x + x].r = (u8)(Clamp(col.r) * 255);
      buf[y * windowSize.x + x].g = (u8)(Clamp(col.g) * 255);
      buf[y * windowSize.x + x].b = (u8)(Clamp(col.b) * 255);
      buf[y * windowSize.x + x].a = (u8)(Clamp(col.a) * 255);
    }
  }

  texture->update((const sf::Uint8*)buf.data());
}

//---------------------------------------------------------------------------
bool Intersect(const Ray& r, HitRec* hitRec)
{
  bool hit = false;
  for (Geo* obj : objects)
  {
    hit |= obj->Intersect(r, hitRec);
  }
  return hit;
}

//---------------------------------------------------------------------------
int main(int argc, char** argv)
{
  u32 width, height;
#ifdef _WIN32
  width = GetSystemMetrics(SM_CXFULLSCREEN);
  height = GetSystemMetrics(SM_CYFULLSCREEN);
#else
  auto displayId = CGMainDisplayID();
  width = (u32)CGDisplayPixelsWide(displayId);
  height = (u32)CGDisplayPixelsHigh(displayId);
#endif

  sf::ContextSettings settings;
  windowSize = Vector2u(8 * width / 10, 8 * height / 10);
  windowSize = { 1024, 768 };
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
  cam.frame = Frame(
      Vector3(1,0,0),
      Vector3(0,1,0),
      Vector3(0,0,1),
      Vector3(5,10,-10));

  renderWindow.clear();
  renderWindow.display();

  renderWindow.clear();

  PathTrace(cam, backbuffer->buffer);
//  RayTrace(cam);
  BufferToTexture(backbuffer, &texture);

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
