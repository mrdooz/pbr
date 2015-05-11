#pragma once

using namespace std;

#pragma pack(push, 1)
namespace protocol
{
  struct SceneBlob
  {
    char id[4];
    u32 fixupOffset;
    u32 nullObjectDataStart;
    u32 meshDataStart;
    u32 lightDataStart;
    u32 cameraDataStart;
    u32 materialDataStart;
    u32 numNullObjects;
    u32 numMeshes;
    u32 numLights;
    u32 numCameras;
    u32 numMaterials;
#pragma warning(suppress: 4200)
    char data[0];
  };

  struct BlobBase
  {
    const char* name;
    u32 id;
    u32 parentId;
    float mtx[12];
  };

  struct MeshBlob : public BlobBase
  {
    struct MaterialGroup
    {
      u32 materialId;
      u32 startIndex;
      u32 numIndices;
    };

    u32 numVerts;
    u32 numIndices;
    u32 numMaterialGroups;
    MaterialGroup* materialGroups;
    float* verts;
    float* normals;
    float* uv;
    u32* indices;

    // bounding sphere
    float sx, sy, sz, r;
  };

  struct NullObjectBlob : public BlobBase
  {

  };

  struct CameraBlob : public BlobBase
  {
    float verticalFov;
    float nearPlane, farPlane;
  };

  struct LightBlob : public BlobBase
  {
    int type;
    float colorRgb[3];
    float intensity;
  };

  struct MaterialBlob
  {
    struct MaterialComponent
    {
      float r, g, b;
      const char* texture;
      float brightness;
    };

    u32 blobSize;
    const char* name;
    u32 materialId;
    u32 flags;
    MaterialComponent* color;
    MaterialComponent* luminance;
    MaterialComponent* reflection;
  };
}
#pragma pack(pop)

namespace pbr
{
  struct Mesh;

  enum VertexFlags
  {
    VF_POS      = 1 << 0,
    VF_NORMAL   = 1 << 4,
    VF_TEX2_0   = 1 << 7,
  };

  struct MeshLoader
  {
    static u32 GetVertexFormat(const protocol::MeshBlob& mesh);

    bool Load(const char* filename);
    void ProcessFixups(u32 fixupOffset);

    vector<protocol::MeshBlob*> meshes;
    vector<protocol::NullObjectBlob*> nullObjects;
    vector<protocol::CameraBlob*> cameras;
    vector<protocol::LightBlob*> lights;
    vector<protocol::MaterialBlob*> materials;
    vector<char> buf;
  };

}
