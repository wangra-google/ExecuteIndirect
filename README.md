# Modified version of Direct3D 12 execute indirect sample

### Changes

- Pre pass to convert textures to byte address buffer
- The compute shader would change following resources
  - Root shader resource view: texture
    - Since ExecuteIndirect can only change SRVs in root, but SRVs cannot be textures or typed buffer, the textures are read during loading time to be converted into raw buffers so that pixel shaders can read them
  - Vertex Buffer & Index Buffer
    - Switching between quad and triangles
  - Root Constant Buffer
    - This is used to calculate the postion of the mesh (from original demo)
  - Root constants
    - This is used to calculate the screen space texture coordinate for meshes
  - Index count per instance
    - This is needed since different meshes have different index count


### Controls

SPACE bar - toggles the compute shader on and off.
