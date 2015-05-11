#include "pbr.hpp"
#include "pbr_math.hpp"
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "mesh_loader.hpp"
#include <stdio.h>
#include "glfw3/GLFW/glfw3.h"

using namespace pbr;

Vector2u windowSize;
vector<Geo*> objects;
vector<Geo*> emitters;

void PathTrace(const Camera& cam, const RenderSettings& settings, Color* buffer);

int MAX_DEPTH = 3;

// guess of average screen maximum brightness
const float DISPLAY_LUMINANCE_MAX = 200.0f;

// ITU-R BT.709 standard RGB luminance weighting
const Vector3 RGB_LUMINANCE(0.2126f, 0.7152f, 0.0722f);

// ITU-R BT.709 standard gamma
const float GAMMA_ENCODE = 0.45f;

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

vector<IsectTri> tris;

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

#define LOAD_MESH 1

#if LOAD_MESH
  MeshLoader loader;
  loader.Load("gfx/crystals_flat.boba");

  for (const protocol::MeshBlob* meshBlob : loader.meshes)
  {
    for (u32 i = 0; i < meshBlob->numIndices; i += 3)
    {
      u32 idx0 = meshBlob->indices[i + 0];
      u32 idx1 = meshBlob->indices[i + 1];
      u32 idx2 = meshBlob->indices[i + 2];

      Vector3 p0 = { meshBlob->verts[idx0 * 3 + 0], meshBlob->verts[idx0 * 3 + 1], meshBlob->verts[idx0 * 3 + 2] };
      Vector3 p1 = { meshBlob->verts[idx1 * 3 + 0], meshBlob->verts[idx1 * 3 + 1], meshBlob->verts[idx1 * 3 + 2] };
      Vector3 p2 = { meshBlob->verts[idx2 * 3 + 0], meshBlob->verts[idx2 * 3 + 1], meshBlob->verts[idx2 * 3 + 2] };

      tris.push_back({ p0, p1, p2 });
    }
  }

#else
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

  Geo* plane = new Plane(Vector3(0, 1, 0), 0);
  plane->material = new Material(planeDiffuse, planeSpec, zero);
  objects.push_back(plane);

  for (Geo* g : objects)
  {
    if (g->material->emissive.Max3() > 0)
      emitters.push_back(g);
  }
#endif

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
  float eps = (float)(1e-4);
  float adaptLuminance = eps;
  u32 numPixels = windowSize.x * windowSize.y;
  float sumOfLogs = 0.0;
  for (u32 i = 0; i < numPixels; ++i)
  {
    const float Y = pixels[i].r * RGB_LUMINANCE.x + pixels[i].g * RGB_LUMINANCE.y + pixels[i].b * RGB_LUMINANCE.z;
    // clamp luminance to a perceptual minimum
    sumOfLogs += log10f(max(eps, Y));
  }

  adaptLuminance = powf(10.0f, sumOfLogs / (float)numPixels);

  // make scale-factor from:
  // ratio of minimum visible differences in luminance, in display-adapted
  // and world-adapted perception (discluding the constant that cancelled),
  // divided by display max to yield a [0,1] range
  const float a = 1.219f + powf(DISPLAY_LUMINANCE_MAX * 0.25f, 0.4f);
  const float b = 1.219f + powf(adaptLuminance, 0.4f);

  return powf(a / b, 2.5f) / DISPLAY_LUMINANCE_MAX;
}

//---------------------------------------------------------------------------
void BufferToTexture(Buffer* buffer, bool doToneMapping, u8* dest)
{
  vector<Color32> buf(windowSize.x * windowSize.y);
  float toneMap = doToneMapping ? CalculateToneMapping(buffer->buffer) : 1;

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

  memcpy(dest, buf.data(), windowSize.x * windowSize.y * 4);
//  texture->update((const sf::Uint8*)buf.data());
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

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error %d: %s\n", error, description);
}

void UpdateTexture(GLuint& texture, const char* buf, int w, int h)
{
  if (texture != 0){
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  }
  else {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


int main(int, char**)
{
  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(1);
  GLFWwindow* window = glfwCreateWindow(1280, 720, "MangeTracer", NULL, NULL);
  glfwMakeContextCurrent(window);

  // Setup ImGui binding
  ImGui_ImplGlfw_Init(window, true);

  u32 width, height;
#ifdef _WIN32
  width = GetSystemMetrics(SM_CXFULLSCREEN);
  height = GetSystemMetrics(SM_CYFULLSCREEN);
#else
  auto displayId = CGMainDisplayID();
  width = (u32)CGDisplayPixelsWide(displayId);
  height = (u32)CGDisplayPixelsHigh(displayId);
#endif

  windowSize = { 512, 512 };

  Init();

  Camera cam;
  cam.fov = DegToRad(60);
  cam.dist = 1;
  cam.LookAt(Vector3(5, 5, -100), Vector3(0, 1, 0), Vector3(0, 0, 30));

  vector<u8> buf(windowSize.x*windowSize.y * 4, 0);

  //  RayTrace(cam);
  ImVec4 clear_color = ImColor(114, 144, 154);

  RenderSettings settings;

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    vector<u8> buf(windowSize.x*windowSize.y * 4);
    BufferToTexture(backbuffer, settings.toneMapping, buf.data());

    GLuint textureId = 0;
    UpdateTexture(textureId, (const char*)buf.data(), windowSize.x, windowSize.y);

    glfwPollEvents();
    ImGui_ImplGlfw_NewFrame();

    ImGui::Begin("MangeTracer");

    ImGui::Checkbox("tonemapping", &settings.toneMapping);
    ImGui::DragInt("samples", &settings.numSamples);
    if (ImGui::Button("GO!"))
    {
      PathTrace(cam, settings, backbuffer->buffer);
    }

    ImGui::Image((ImTextureID)textureId, ImVec2((float)windowSize.x, (float)windowSize.y));
    ImGui::End();

    // Rendering
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplGlfw_Shutdown();
  glfwTerminate();

  return 0;
}
