#include "pbr.hpp"
#include "pbr_math.hpp"

using namespace pbr;
using sf::Vector2f;

float* backbuffer;
Vector2u windowSize;

vector<Geo*> objects;

int MAX_DEPTH = 3;

//---------------------------------------------------------------------------
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
  center->material.emissive = Color(0.75f, 0.75f, 0.75f);
  objects.push_back(center);

  Geo* plane = new Plane(Vector3(0,1,0), 0);
  plane->material.diffuse = Color(0.5f, 0.5f, 0.5f);
  objects.push_back(plane);
}

//---------------------------------------------------------------------------
void Close()
{
  free(backbuffer);
}

//---------------------------------------------------------------------------
Color PathTraceInner(const Ray& r, int depth)
{
  float tMin = 1e20;
  HitRec closest;

  for (Geo* obj : objects)
  {
    obj->Intersect(r, &tMin, &closest);
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
  return cur + PathTraceInner(Ray(p + 1e-4 * n, v), depth + 1);
}

//---------------------------------------------------------------------------
void PathTrace(const Camera& cam)
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
        tmp += PathTraceInner(r, 0);
      }

      *pp++ = tmp/(iterations*MAX_DEPTH);


      p.x += xInc;
    }
    tmp.y += yInc;
  }

}

//---------------------------------------------------------------------------
bool Intersect(const Ray& r, HitRec* hitRec)
{
  float tMin = 1e20;
  bool hit = false;

  for (Geo* obj : objects)
  {
    hit |= obj->Intersect(r, &tMin, hitRec);
  }

  return hit;
}

//---------------------------------------------------------------------------
void RayTrace(const Camera& cam)
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

  Sampler* sampler = new PoissonSampler();
  sampler->Init(64);

  // top left corner
  Vector3 p(cam.frame.origin - halfWidth * cam.frame.right + imagePlaneHeight/2 * cam.frame.up + cam.dist * cam.frame.dir);
  Vector3 tmp = p;

  Color* pp = (Color*)backbuffer;

  for (u32 y = 0; y < windowSize.y; ++y)
  {
    p = tmp;
    for (u32 x = 0; x < windowSize.x; ++x)
    {
      Color col(0,0,0);
      u32 numSamples = 4;
      for (u32 i = 0; i < numSamples; ++i)
      {
        // construct ray from eye pos through the image plane
        Vector2 ofs = sampler->NextSample();
        Ray r(cam.frame.origin, Normalize((p + Vector3(ofs.x * xInc, -ofs.y * yInc, 0)) - cam.frame.origin));

        HitRec closest;
        if (Intersect(r, &closest))
        {
          Vector3 p = closest.pos;
          Vector3 n = closest.normal;

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

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
float Dist(const Vector2f& a, const Vector2f& b)
{
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  return dx*dx + dy*dy;
}

//---------------------------------------------------------------------------
void CalcDistribution(const vector<Vector2f> points, float* mean, float* deviation)
{
  u32 numSamples = points.size();
  vector<float> distances(points.size());
  double sum = 0;

  for (u32 i = 0; i < numSamples; ++i)
  {
    // find distance to closest point
    float dist = FLT_MAX;
    u32 idx;
    for (u32 j = 0; j < numSamples; ++j)
    {
      if (i == j)
        continue;

      float tmp = Dist(points[i], points[j]);
      if (tmp < dist)
      {
        dist = tmp;
        idx = j;
      }
    }

    float d = sqrtf(dist);
    distances[i] = d;
    sum += d;
  }

  // find mean
  *mean = sum / numSamples;

  // find standard deviation
  double d = 0;
  for (u32 i = 0; i < numSamples; ++i)
  {
    float dx = distances[i] - *mean;
    d += dx * dx;
  }

  *deviation = sqrtf(d / numSamples);
}


//---------------------------------------------------------------------------
// Poisson distributed samples, from "Antialiased Images at Low Sampling Densities"
void PoissonSamples(u32 numSamples, sf::Texture* texture, vector<Vector2f>* samples)
{
  int w = windowSize.x;
  int h = windowSize.y;

  vector<sf::Color> buf(w*h);
  memset(buf.data(), 0, w*h*4);

  int f = sqrt(numSamples);

  int cellsX = w / f;
  int cellsY = h / f;

  int subCellsX = cellsX * 4;
  int subCellsY = cellsY * 4;

  // 2d space is divided into sqrt(numSamples) grid cells, and each grid cell further
  // divided into 4x4 cells
  vector<float> d(subCellsX*subCellsY);

  for (u32 i = 1; i < subCellsY; ++i)
  {
    for (u32 j = 1; j < subCellsX; ++j)
    {
      // calc T value for current cell
      float t = (4 * d[(j - 1) + (i) * subCellsX] + d[(j - 1) + (i - 1) * subCellsX] + 2 * d[(j) + (i - 1) * subCellsX] + d[(j + 1) + (i - 1) * subCellsX]) / 8;
      t += randf(1.f / 16 - 1.f / 64, 1.f / 16 + 1.f / 64);

      // determine if the current cell should have a pixel
      float s = t < 0.5f ? 0 : 1;
      d[j + i * subCellsX] = t - s;

      if (s > 0)
      {
        float x = j * (f/4);
        float y = i * (f/4);
        samples->push_back({x, y});
        buf[y * w + x] = sf::Color::White;
      }
    }
  }

  texture->update((const sf::Uint8*)buf.data());
}


//---------------------------------------------------------------------------
void RandomSamples(u32 numSamples, sf::Texture* texture, vector<Vector2f>* samples)
{
  int w = windowSize.x;
  int h = windowSize.y;

  vector<sf::Color> buf(w*h);
  samples->resize(numSamples);
  memset(buf.data(), 0, w*h*4);

  for (u32 i = 0; i < numSamples; ++i)
  {
    float x = rand() % windowSize.x;
    float y = rand() % windowSize.y;
    (*samples)[i] = {x, y};
    buf[y*w+x] = sf::Color::White;
  }

  texture->update((const sf::Uint8*)buf.data());
}

//---------------------------------------------------------------------------
void UniformSamples(u32 numSamples, sf::Texture* texture, vector<Vector2f>* samples)
{
  int w = windowSize.x;
  int h = windowSize.y;

  vector<sf::Color> buf(w*h);
  samples->resize(numSamples);
  memset(buf.data(), 0, w*h*4);

  int f = sqrt(numSamples);

  for (u32 i = 0; i < numSamples; ++i)
  {
    float x = rand() % windowSize.x;
    float y = rand() % windowSize.y;
    x = w / f * (i % f);
    y = h / f * (i / f);
    (*samples)[i] = {x, y};
    buf[y*w+x] = sf::Color::White;
  }

  texture->update((const sf::Uint8*)buf.data());
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
  width = CGDisplayPixelsWide(displayId);
  height = CGDisplayPixelsHigh(displayId);
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
  cam.frame = Frame(Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1), Vector3(0,5,-10));

  renderWindow.clear();
  renderWindow.display();

  renderWindow.clear();

  bool dist = false;

  if (dist)
  {
    vector<Vector2f> samples;
//  RandomSamples(1024, &texture, &samples);
    PoissonSamples(1024, &texture, &samples);

    float mean, dev;
    CalcDistribution(samples, &mean, &dev);
    printf("mean: %.3f, stddev: %.3f\n", mean, dev);
  }

  RayTrace(cam);
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