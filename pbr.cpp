#include "pbr.hpp"
#include "pbr_math.hpp"


using namespace pbr;

Vector2u windowSize;
vector<Geo*> objects;
vector<Geo*> emitters;

void PathTrace(const Camera& cam, Color* buffer);

int MAX_DEPTH = 3;

// guess of average screen maximum brightness
const float DISPLAY_LUMINANCE_MAX = 200.0;

// ITU-R BT.709 standard RGB luminance weighting
const Vector3 RGB_LUMINANCE(0.2126, 0.7152, 0.0722);

// ITU-R BT.709 standard gamma
const float GAMMA_ENCODE = 0.45;


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

  float lumScale = 1.f;
  Color ballDiffuse(0.1f, 0.4f, 0.4f);
  Color ballSpec(0.2f, 0.2f, 0.2f);
  Color ballEmit(0.75f, 0.75f, 0.75f);

  Color planeDiffuse(0.5, 0, 0);
  Color planeSpec(0.1f, 0.1f, 0.1f);
  ballEmit = lumScale * ballEmit;
  Color zero(0, 0, 0);

#if 0
  int numBalls = 10;
  for (u32 i = 0; i < numBalls; ++i)
  {
    float angle = i * 2 * Pi / numBalls;
    Geo* g = new Sphere(Vector3(10 * cosf(angle), 0, 30 + 10 * sinf(angle)), 2);
    if (i & 1)
      g->material = new Material(ballDiffuse, ballSpec, zero);
    else
      g->material = new Material(ballDiffuse, ballSpec, ballEmit);
    objects.push_back(g);
  }

  Geo* center = new Sphere(Vector3(0, 50, 30), 15);

  center->material = new Material(ballDiffuse, zero, ballEmit);
  objects.push_back(center);
#else
  {
    Geo* g = new Sphere(Vector3(-10, 10, 30), 7);
    g->material = new Material(ballDiffuse, ballSpec, ballEmit);
    objects.push_back(g);
  }

  {
    Geo* g = new Sphere(Vector3(0, 0, 30), 5);
    g->material = new Material(ballDiffuse, ballSpec, zero);
    objects.push_back(g);
  }

  {
    Geo* g = new Sphere(Vector3(10, 0, 30), 2);
    g->material = new Material(ballDiffuse, ballSpec, zero);
    objects.push_back(g);
  }
#endif

  Geo* plane = new Plane(Vector3(0,1,0), 0);
  plane->material = new Material(planeDiffuse, planeSpec, zero);
  objects.push_back(plane);

  for (Geo* g : objects)
  {
    if (g->material->emissive.Max3() > 0)
      emitters.push_back(g);
  }

}

//---------------------------------------------------------------------------
void Close()
{
  free(backbuffer);
}

//---------------------------------------------------------------------------
float CalculateToneMapping(Color* pixels)
{
  // calculate estimate of world-adaptation luminance
  // as log mean luminance of scene
  float adaptLuminance = 1e-4;
  u32 numPixels = windowSize.x * windowSize.y;
  float sumOfLogs = 0.0;
  for (u32 i = 0; i < numPixels; ++i)
  {
    const float Y = pixels[i].r * RGB_LUMINANCE.x + pixels[i].g * RGB_LUMINANCE.y + pixels[i].b * RGB_LUMINANCE.z;
    // clamp luminance to a perceptual minimum
    sumOfLogs += log10(Y > 1e-4 ? Y : 1e-4);
  }

  adaptLuminance = pow(10.0, sumOfLogs / (float)numPixels);

  // make scale-factor from:
  // ratio of minimum visible differences in luminance, in display-adapted
  // and world-adapted perception (discluding the constant that cancelled),
  // divided by display max to yield a [0,1] range
  const float a = 1.219f + powf(DISPLAY_LUMINANCE_MAX * 0.25f, 0.4f);
  const float b = 1.219f + powf(adaptLuminance, 0.4f);

  return powf(a / b, 2.5f) / DISPLAY_LUMINANCE_MAX;
}

//---------------------------------------------------------------------------
void BufferToTexture(Buffer* buffer, sf::Texture *texture)
{
  vector<sf::Color> buf(windowSize.x * windowSize.y);
  float toneMap = CalculateToneMapping(buffer->buffer);

  Color* pp = buffer->buffer;
  for (u32 y = 0; y < windowSize.y; ++y)
  {
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      const Color &col = *pp++;
      buf[y * windowSize.x + x].r = (u8)(Clamp(powf(col.r * toneMap, GAMMA_ENCODE)) * 255);
      buf[y * windowSize.x + x].g = (u8)(Clamp(powf(col.g * toneMap, GAMMA_ENCODE)) * 255);
      buf[y * windowSize.x + x].b = (u8)(Clamp(powf(col.b * toneMap, GAMMA_ENCODE)) * 255);
      buf[y * windowSize.x + x].a = 255;
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
  windowSize = { 512, 512 };
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
  cam.LookAt(Vector3(5, 5, 10), Vector3(0, 1, 0), Vector3(0, 0, 30));

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
