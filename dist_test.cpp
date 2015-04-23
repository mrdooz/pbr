#include "pbr_math.hpp"

using namespace pbr;
extern Vector2u windowSize;

//---------------------------------------------------------------------------
void DisplaySamples(const vector<Vector2>& samples, sf::Texture* texture)
{
  int w = windowSize.x;
  int h = windowSize.y;

  vector<sf::Color> buf(w*h);
  memset(buf.data(), 0, w*h*4);

  for (const Vector2& v : samples)
  {
    // convert from [-1, +1], [-1, +1] to window size
    int x = w/2 + w/2 * v.x;
    int y = h/2 + h/2 * v.y;

    buf[y * w + x] = sf::Color::White;
  }
  texture->update((const sf::Uint8*)buf.data());
}

//---------------------------------------------------------------------------
void CalcDistribution(const vector<Vector2> points, float* mean, float* deviation)
{
  u32 numSamples = (u32)points.size();
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

      float tmp = (points[i] - points[j]).LengthSquared();
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
  *mean = (float)(sum / numSamples);

  // find standard deviation
  double d = 0;
  for (u32 i = 0; i < numSamples; ++i)
  {
    float dx = distances[i] - *mean;
    d += dx * dx;
  }

  *deviation = sqrtf((float)(d / numSamples));
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
void ShowDistribution(Texture& texture)
{
  Sampler* sampler = new PoissonSampler();
  int s = 1 << 12;
  sampler->Init(s);

  vector<Vector2> samples;
  samples.resize(s);

  for (int i = 0; i < s; ++i)
    samples[i] = sampler->NextSample();

  float mean, dev;
  CalcDistribution(samples, &mean, &dev);
  printf("mean: %.3f, stddev: %.3f\n", mean, dev);
  DisplaySamples(samples, &texture);
}
