# Next Steps: Event System, ImGui, and Component Structure

This is a beginner-friendly plan for adding three things to the engine: Dear
ImGui (a debug UI), a proper event system, and a real entity/component
structure — with a bigger push into actually getting 3D models on screen,
since that's currently 100% unwired dead code.

Every section explains **what the thing is and why it exists**, not just
what to type, since the goal is understanding, not just a working build.
There's a day-by-day plan near the top if you just want to start typing.

---

## Jargon glossary (read this first if any of these are fuzzy)

- **ECS (Entity Component System)** — instead of a `Player` class with
  fields for position, mesh, health, etc., you split it up: an **entity**
  is just an ID number (nothing else), a **component** is a plain struct of
  data attached to that ID (`TransformComponent`, `MeshComponent`), and a
  **system** is code that loops over "all entities that have components X
  and Y" and does something with them. Think of it like a spreadsheet: the
  entity is the row number, each component type is a column, and a system
  is a formula that only reads certain columns. You already have this via
  `entt::registry` — you're just not using very many "columns" yet.
- **Push constants** — a tiny (~128 byte) chunk of data you attach directly
  to a draw call, the fastest way to hand a shader a few numbers (like a
  transform matrix). `QuadRenderer` already uses these.
- **Descriptor pool / descriptor set** — Vulkan wants to know *in advance*
  how many textures/uniform-buffers/etc. your program will ever bind, so it
  can reserve the bookkeeping upfront. A descriptor pool is that
  reservation — like a parking garage that pre-allocates a fixed number of
  spots. ImGui needs one; your engine doesn't have one yet.
- **Staging buffer** — GPU-only memory is fast for the GPU to read but the
  CPU can't write into it directly. So you first write your data into a
  slower CPU-visible "staging" buffer, then issue a GPU-side copy from that
  staging buffer into the fast GPU-only buffer. `QuadRenderer::Render`
  already does this for the quad mesh — it's the pattern to reuse for real
  3D meshes.
- **Depth buffer** — an extra per-pixel image (alongside the color image)
  that stores "how far away is the triangle currently drawn at this pixel."
  Without one, triangles just paint over each other in whatever order you
  submit them, so a mesh behind another mesh can show through in front of
  it. **Your engine does not have a depth buffer yet** — this is required
  before 3D meshes will look correct.
- **MVP matrix** — Model matrix (where is this object in the world) ×
  View matrix (where is the camera, which way is it looking) × Projection
  matrix (the "lens" — field of view, near/far clipping — that turns 3D
  coordinates into a 2D image). Every vertex gets multiplied by this
  combined matrix in the vertex shader.
- **`entt::dispatcher`** — think of it as a group chat. Some code
  "triggers" a message (`dispatcher.trigger<KeyPressedEvent>(...)`), and
  any code that previously "connected" as a listener gets called
  automatically. Neither side needs to know about the other directly.
  Already bundled inside the `entt.hpp` you've vendored — no new library
  needed.
- **Dynamic rendering** (`VK_KHR_dynamic_rendering`, core since Vulkan 1.3)
  — an alternative to the classic `VkRenderPass`/`VkFramebuffer` setup.
  Instead of pre-declaring attachments and subpasses in a render pass
  object, you just call `vkCmdBeginRendering` each frame with a plain list
  of image views to draw into. Fewer objects to manage, and adding a new
  attachment (like a depth buffer) doesn't require touching render-pass or
  framebuffer creation. The plan below switches to this early, since it's
  simpler to build the rest of the week on top of than the classic path.

---

## Suggested one-week plan

Assumes roughly one evening/session per day. Each day builds on the last,
but you can reorder freely — this is a plan, not a rulebook.

| Day | Focus | Outcome |
|---|---|---|
| 1 | Switch to dynamic rendering, then bring up ImGui | Render pass/framebuffer objects removed; ImGui demo window renders through the new path |
| 2 | Depth buffer + `Camera3D` | 3D-correct rendering infrastructure and a real camera with view/projection matrices |
| 3 | `TransformComponent`, `MeshComponent`, shared mesh upload | Data model ready for 3D entities; no more copy-pasted GPU upload code |
| 4 | 3D shaders + `MeshRenderer` pipeline | A pipeline that can actually draw a `Mesh` with lighting-ready vertex data |
| 5 | Wire `ModelLoader` → entity → `MeshRenderer` | Your `.glb` files in `Assets/Models/` actually appear on screen |
| 6 | Event system + input/resize | Callback-driven input, `entt::dispatcher`, window resizing works |
| 7 | Polish | ImGui entity inspector, fix Release/Win32 build config gaps, cleanup |

Sections below are ordered to match this table, with full explanations and
example code for each day.

---

## Day 1 — Switch to dynamic rendering, then bring up ImGui

This is the biggest single day this week — it touches the core render
loop — but doing it now avoids writing render-pass-based pipeline/ImGui
code today and then rewriting it later. Budget extra time for this one if
needed; everything after it (depth buffer, `MeshRenderer` pipeline) is
noticeably simpler once this is done.

### Part A: switch the engine to dynamic rendering

Right now `VulkanEngine::Init` ([src/Engine/VulkanEngine.cpp:15](src/Engine/VulkanEngine.cpp#L15))
calls `CreateRenderPass()` and `CreateFramebuffers()`, and `BeginFrame()`/
`EndFrame()` begin/end that render pass each frame. Dynamic rendering
replaces all of that with a per-frame call describing what to draw into
directly — no render pass or framebuffer objects at all.

1. **Enable the feature.** Check `CreateInstance`
   ([src/Engine/VulkanEngine.cpp:303](src/Engine/VulkanEngine.cpp#L303))
   for the `apiVersion` passed in `VkApplicationInfo` — bump it to
   `VK_API_VERSION_1_3` if it isn't already (you're building against
   Vulkan SDK 1.4, so 1.3 is available). Then in `CreateLogicalDevice`,
   chain a `VkPhysicalDeviceDynamicRenderingFeatures` (or
   `VkPhysicalDeviceVulkan13Features` with `dynamicRendering = VK_TRUE`)
   into `VkDeviceCreateInfo::pNext` when creating the device.
2. **Delete the render pass/framebuffer setup.** Remove the calls to
   `CreateRenderPass()`/`CreateFramebuffers()` from `Init()` (the
   functions themselves can go too, along with `m_RenderPass` and
   `m_SwapChainFramebuffers`).
3. **Rewrite `BeginFrame()`/`EndFrame()`.** This is the part that
   surprises people coming from render passes: a render pass used to
   handle image layout transitions for you automatically (that's what the
   `VkSubpassDependency` in the old `CreateRenderPass` was doing). With
   dynamic rendering, you're responsible for that yourself via explicit
   pipeline barriers:

   ```cpp
   // BeginFrame(), after acquiring the swapchain image:
   TransitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

   VkRenderingAttachmentInfo colorAttachment{};
   colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
   colorAttachment.imageView = m_SwapChainImageViews[m_CurrentImageIndex];
   colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

   VkRenderingInfo renderingInfo{};
   renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
   renderingInfo.renderArea = { {0, 0}, m_SwapChainExtent };
   renderingInfo.layerCount = 1;
   renderingInfo.colorAttachmentCount = 1;
   renderingInfo.pColorAttachments = &colorAttachment;

   vkCmdBeginRendering(commandBuffer, &renderingInfo);
   ```

   ```cpp
   // EndFrame(), before present:
   vkCmdEndRendering(commandBuffer);
   TransitionImageLayout(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
   ```

   `TransitionImageLayout` here is a small helper you'll want to add (a
   `vkCmdPipelineBarrier` with the right `VkImageMemoryBarrier` src/dst
   access masks and stage masks for each transition) — it's the same kind
   of barrier code Vulkan tutorials use for texture uploads, just applied
   to the swapchain image instead.
4. **Update pipeline creation.** `QuadRenderer::CreatePipeline` currently
   passes a `VkRenderPass` handle into `VkGraphicsPipelineCreateInfo`.
   With dynamic rendering, pipelines instead declare their attachment
   *formats* via a `VkPipelineRenderingCreateInfo` chained into
   `pNext`, and `renderPass`/`subpass` are left as `VK_NULL_HANDLE`/`0`:

   ```cpp
   VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
   pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
   pipelineRenderingInfo.colorAttachmentCount = 1;
   pipelineRenderingInfo.pColorAttachmentFormats = &m_SwapChainImageFormat;
   // depthAttachmentFormat added on Day 2 once the depth buffer exists

   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.pNext = &pipelineRenderingInfo;
   pipelineInfo.renderPass = VK_NULL_HANDLE;
   // ...rest unchanged
   ```

Once `QuadRenderer` renders correctly through this path, the engine no
longer has a legacy render pass anywhere — everything from here on
(ImGui, the depth buffer, `MeshRenderer`) builds on dynamic rendering
directly instead of needing a render-pass version and a dynamic-rendering
version.

### Part B: bring up Dear ImGui

**Why this matters:** it gives you an in-engine debug UI you'll lean on
for every day after this (inspecting transforms, camera position, entity
lists), and it's a good "does my build still work" checkpoint before
moving on.

### What's already there

Full ImGui source + Vulkan/GLFW backends are vendored at `External/Imgui/`
(it even has its own `.git`), and `External\imgui` is already on your
include path. But **none of the ImGui `.cpp` files are registered to
compile**, so right now it contributes nothing to the build.

### Step 1: register the source files

Add these to the `<ClCompile>` group in `VulkanEngine.vcxproj`:

```
External\Imgui\imgui.cpp
External\Imgui\imgui_draw.cpp
External\Imgui\imgui_tables.cpp
External\Imgui\imgui_widgets.cpp
External\Imgui\imgui_demo.cpp          (optional — has ImGui::ShowDemoWindow(), handy for learning the API)
External\Imgui\backends\imgui_impl_glfw.cpp
External\Imgui\backends\imgui_impl_vulkan.cpp
```

Add `External\Imgui\backends` to `AdditionalIncludeDirectories` (currently
only `External\imgui` root is listed).

⚠️ Only `Debug|x64` currently has working include/lib paths configured.
`Release|x64` and both `Win32` configs are missing them — fine to ignore
for now if you only build `Debug|x64`, but you'll want to fix this
eventually (see Day 7).

### Step 2: descriptor pool

ImGui's Vulkan backend needs a `VkDescriptorPool`. None exists anywhere in
your codebase yet (`QuadRenderer` only uses push constants), so this is a
new thing:

```cpp
VkDescriptorPoolSize poolSizes[] = {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
};

VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
poolInfo.maxSets = 1;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = poolSizes;

vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_ImGuiDescriptorPool);
```

### Step 3: init

```cpp
ImGui::CreateContext();
ImGui_ImplGlfw_InitForVulkan(engine.GetWindow(), true); // "true" = ImGui installs its own GLFW callbacks — see Day 6 note

VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
pipelineRenderingInfo.colorAttachmentCount = 1;
pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat; // engine.GetSwapChainFormat()

ImGui_ImplVulkan_InitInfo initInfo{};
initInfo.Instance              = engine.GetInstance();
initInfo.PhysicalDevice        = engine.GetPhysicalDevice();
initInfo.Device                = engine.GetDevice();
initInfo.Queue                 = engine.GetGraphicsQueue();
initInfo.DescriptorPool        = m_ImGuiDescriptorPool;
initInfo.MinImageCount         = engine.GetSwapChainImageCount();
initInfo.ImageCount            = engine.GetSwapChainImageCount();
initInfo.MSAASamples           = VK_SAMPLE_COUNT_1_BIT;    // engine has no MSAA today
initInfo.UseDynamicRendering   = true;
initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo; // no RenderPass/Subpass fields needed now

ImGui_ImplVulkan_Init(&initInfo);
```

All of these getters (`GetInstance()`, `GetPhysicalDevice()`, etc.) already
exist on `VulkanEngine` — you don't need to add anything to that class for
this step. Check `External/Imgui/backends/imgui_impl_vulkan.h` for your
vendored version's exact field names — `UseDynamicRendering` and
`PipelineRenderingCreateInfo` were added in a mid-2023 ImGui release, so
this assumes a reasonably current checkout (which yours is, since it has
its own `.git` and was likely cloned recently).

Font upload: check `External/Imgui/backends/imgui_impl_vulkan.h` for your
vendored version's API — older ImGui versions require an explicit
`ImGui_ImplVulkan_CreateFontsTexture()` call (you can reuse the existing
`BeginSingleTimeCommands`/`EndSingleTimeCommands` helpers on
`VulkanEngine` for this instead of letting ImGui manage its own command
buffer); newer versions build the font atlas lazily on the first
`NewFrame()` and need nothing extra.

### Step 4: per-frame calls

`RenderSystem::RenderFrame` currently does
`BeginFrame() → QuadRenderer::Render() → EndFrame()`.

- `ImGui_ImplVulkan_NewFrame()` / `ImGui_ImplGlfw_NewFrame()` /
  `ImGui::NewFrame()` go before any game code calls ImGui widget
  functions — the top of `Game::Update` is a natural spot.
- `ImGui::Render()` + `ImGui_ImplVulkan_RenderDrawData(...)` go **after**
  `QuadRenderer::Render()` but **before** `EndFrame()`, because
  `vkCmdEndRendering` hasn't been called yet at that point — you're still
  inside the same `vkCmdBeginRendering`/`vkCmdEndRendering` scope:

```cpp
// somewhere after QuadRenderer::Render(registry) in RenderFrame:
ImGui::Render();
ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Engine->GetCurrentCommandBuffer());
```

### Step 5: prove it works

Call `ImGui::ShowDemoWindow()` between `NewFrame()` and `Render()`. If you
see the standard ImGui demo window with all its widgets, everything above
is wired correctly. Remove it once you've got your own panels (Day 7).

### Cleanup

Mirror the engine's existing shutdown order: `ImGui_ImplVulkan_Shutdown()`,
`ImGui_ImplGlfw_Shutdown()`, `ImGui::DestroyContext()`, then destroy the
descriptor pool.

---

## Day 2 — Depth buffer + a real 3D camera

**Why this matters:** this is the part that's easy to skip and then wonder
why your 3D model looks like a broken mess of overlapping triangles. The
engine currently has zero depth testing — fine for flat 2D sprites (which
is all it draws today) but it will break visibly the moment you draw a
mesh with triangles facing away from or behind each other.

Because Day 1 already switched the engine to dynamic rendering, adding a
depth buffer here is noticeably less work than it would've been with the
old render-pass path — no `CreateRenderPass`/`CreateFramebuffers` editing
needed, since there's no render pass or framebuffer objects left to edit.

### What needs to change in `VulkanEngine`

1. Create a depth image (`VkImage` + `VkDeviceMemory` + `VkImageView`),
   typically format `VK_FORMAT_D32_SFLOAT`, sized to match the swapchain
   extent. This is conceptually identical to how the color swapchain
   images work, just a different format/usage.
2. In `BeginFrame()`, add a `VkRenderingAttachmentInfo` for depth and
   point `VkRenderingInfo::pDepthAttachment` at it, alongside the color
   attachment from Day 1:

   ```cpp
   VkRenderingAttachmentInfo depthAttachment{};
   depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
   depthAttachment.imageView = m_DepthImageView;
   depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
   depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // don't need it after the frame
   depthAttachment.clearValue.depthStencil = { 1.0f, 0 };      // 1.0 = maximally far away

   renderingInfo.pDepthAttachment = &depthAttachment;
   ```

   You'll also need one more `TransitionImageLayout` call for the depth
   image (to `VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL`) the first time it's
   used — the same barrier helper from Day 1, just with depth-appropriate
   aspect mask/access flags instead of color.
3. Recreate the depth image whenever `RecreateSwapChain()` runs, same as
   the color images.
4. Any pipeline that wants depth testing (your future `MeshRenderer`
   pipeline, see Day 4) needs a `VkPipelineDepthStencilStateCreateInfo`
   with `depthTestEnable = VK_TRUE` and `depthWriteEnable = VK_TRUE`, plus
   `depthAttachmentFormat = VK_FORMAT_D32_SFLOAT` added to that pipeline's
   `VkPipelineRenderingCreateInfo` from Day 1. `QuadRenderer`'s pipeline
   can leave depth testing off since 2D sprites don't need it.

### `Camera3D`

Right now [src/Game/Camera3D.h](src/Game/Camera3D.h) and
[src/Game/Camera3D.cpp](src/Game/Camera3D.cpp) are completely empty. A
minimal camera just needs enough state to build a view matrix and a
projection matrix:

```cpp
class Camera3D
{
public:
    glm::vec3 Position{ 0.0f, 0.0f, 3.0f };
    float Yaw   = -90.0f; // degrees, facing -Z
    float Pitch = 0.0f;
    float FovDegrees = 60.0f;

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + GetForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 GetProjectionMatrix(float aspectRatio) const
    {
        glm::mat4 proj = glm::perspective(glm::radians(FovDegrees), aspectRatio, 0.1f, 100.0f);
        proj[1][1] *= -1.0f; // GLM assumes OpenGL's Y-flip; Vulkan needs this negated
        return proj;
    }

    glm::vec3 GetForward() const
    {
        return glm::normalize(glm::vec3(
            cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)),
            sin(glm::radians(Pitch)),
            sin(glm::radians(Yaw)) * cos(glm::radians(Pitch))));
    }
};
```

The `proj[1][1] *= -1.0f` line is a very common gotcha — GLM's projection
math was written for OpenGL's coordinate conventions, and Vulkan's clip
space is flipped on Y relative to OpenGL. Forgetting this line renders
your scene upside down.

You don't need to hook up camera movement yet — that's Day 5/6 once you
have something worth flying around.

---

## Day 3 — `TransformComponent`, `MeshComponent`, shared mesh upload

### Components

`SpriteComponent` currently has position baked directly into it. Pull a
shared `TransformComponent` out so both sprites and meshes use the same
spatial data instead of duplicating fields:

```cpp
struct TransformComponent
{
    glm::vec3 Position{ 0.0f };
    glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale{ 1.0f };

    glm::mat4 GetMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), Position)
             * glm::mat4_cast(Rotation)
             * glm::scale(glm::mat4(1.0f), Scale);
    }
};

struct MeshComponent
{
    Mesh* MeshData = nullptr; // fine for now; swap for an asset handle later if you add asset management
};
```

### Stop copy-pasting the GPU upload code

Right now the vertex/index buffer upload logic (staging buffer → copy →
device-local buffer, see the glossary above) is written directly inside
`QuadRenderer::Render` ([src/Renderer/QuadRenderer.cpp:26-91](src/Renderer/QuadRenderer.cpp#L26-L91)).
`Mesh` ([src/Components/Mesh.h](src/Components/Mesh.h)) already holds the
`vertexBuffer`/`indexBuffer` handles but has no method that fills them in —
`QuadRenderer` does it inline instead. Before writing `MeshRenderer`
(Day 4), pull that logic out into a method both renderers can call:

```cpp
// Mesh.h
void UploadToGPU(VulkanEngine* engine);
bool IsUploaded() const { return vertexBuffer != VK_NULL_HANDLE; }
```

```cpp
// Mesh.cpp — this is exactly QuadRenderer::Render's staging-buffer logic,
// just moved here and made generic (uses Vertices/Indices instead of
// m_QuadMesh->Vertices/Indices)
void Mesh::UploadToGPU(VulkanEngine* engine)
{
    if (IsUploaded()) return;

    VkDeviceSize vertexBufferSize = sizeof(Vertices[0]) * Vertices.size();
    VkBuffer stagingBuffer; VkDeviceMemory stagingMemory;
    engine->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(engine->GetDevice(), stagingMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, Vertices.data(), vertexBufferSize);
    vkUnmapMemory(engine->GetDevice(), stagingMemory);

    engine->CreateBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    engine->CopyBuffer(stagingBuffer, vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(engine->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(engine->GetDevice(), stagingMemory, nullptr);

    // ...repeat the same pattern for Indices / indexBuffer
}
```

Then go back and simplify `QuadRenderer::Render` to just call
`m_QuadMesh->UploadToGPU(m_Engine)` instead of the ~65 lines it has today.
This is the single most valuable cleanup this week — it turns "3D mesh
upload" from new code you have to write into code you already wrote once
and can reuse.

---

## Day 4 — 3D shaders + `MeshRenderer`

### Why you need new shaders

`Shaders/` currently only has `ui_sprite.vert`/`.frag`, built for flat 2D
sprites with push-constant transform + tint color — no lighting, no depth.
`Vertex` ([src/Components/Vertex.h](src/Components/Vertex.h)) already has
`position`, `color`, `normal`, and `texCoord`, so the vertex *data* is
ready; you just need a shader pair that actually uses `normal` for
lighting and the MVP matrix for 3D projection.

A minimal `mesh.vert`:

```glsl
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} pc;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;

void main() {
    gl_Position = pc.proj * pc.view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = mat3(transpose(inverse(pc.model))) * inNormal; // world-space normal
}
```

A minimal `mesh.frag` with a hardcoded directional light (good enough to
prove the mesh renders — swap for something fancier later):

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3));
    float diffuse = max(dot(normalize(fragNormal), -lightDir), 0.2); // 0.2 = ambient floor
    outColor = vec4(fragColor * diffuse, 1.0);
}
```

Note the push constant here is three 4x4 matrices (192 bytes) — over the
128-byte minimum guaranteed push constant size on some hardware. If you
hit validation errors about push constant size, switch `model`/`view`/`proj`
to a small uniform buffer instead (more setup, but standard practice for
per-frame camera data anyway — a good "if I have extra time" upgrade).

Compile these the same way `ui_sprite` is compiled — check
`compile_shaders.bat`/`.ps1` and add the new files there.

### `MeshRenderer`

[src/Renderer/MeshRenderer.h](src/Renderer/MeshRenderer.h) and its `.cpp`
are currently empty stubs. Structure it exactly like `QuadRenderer`
(constructor takes `VulkanEngine*`, `Init()`/`Render()`/`Shutdown()`), but:

- Pipeline uses `mesh.vert`/`mesh.frag`, `Vertex::getBindingDescription()`/
  `getAttributeDescriptions()` (already 4 attributes wide — position,
  color, normal, texCoord — so no changes needed there).
- Pipeline enables depth testing (the `VkPipelineDepthStencilStateCreateInfo`
  from Day 2) and sets `depthAttachmentFormat` on its
  `VkPipelineRenderingCreateInfo` (the dynamic-rendering pipeline info
  from Day 1) — this is the one field `QuadRenderer`'s pipeline doesn't
  need, since it skips depth testing entirely.
- `Render(entt::registry&)` iterates
  `registry.view<TransformComponent, MeshComponent>()`, calls
  `mesh.MeshData->UploadToGPU(m_Engine)` once per mesh (guarded by
  `IsUploaded()`), binds vertex/index buffers, pushes
  `{ transform.GetMatrix(), camera.GetViewMatrix(), camera.GetProjectionMatrix(aspect) }`,
  and calls `vkCmdDrawIndexed`.
- Wire it into `RenderSystem::RenderFrame` alongside `QuadRenderer::Render`
  (both run inside the same `BeginFrame()`/`EndFrame()` pair — order
  matters less now that depth testing exists, but drawing opaque 3D meshes
  before UI-space sprites is the conventional order).

---

## Day 5 — get an actual model on screen

`ModelLoader` ([src/Components/ModelLoader.cpp](src/Components/ModelLoader.cpp))
already works — it uses Assimp to load `.obj`/`.fbx`/`.glb`/`.gltf`/etc.,
triangulates, generates normals if missing, and fills a `Mesh`'s
`Vertices`/`Indices`. It's just never called from `Game.cpp`. You have two
real test assets sitting in `Assets/Models/`: `Test1.glb` and
`cartoon_lowpoly_trees_blend.glb`.

Wiring it up, roughly where `Game::CreateInitialEntities()` currently
creates sprite entities:

```cpp
Mesh* treeMesh = new Mesh(ModelLoader::LoadModel("Assets/Models/cartoon_lowpoly_trees_blend.glb"));

entt::entity treeEntity = m_Registry->create();
m_Registry->emplace<TransformComponent>(treeEntity); // defaults: origin, no rotation, scale 1
m_Registry->emplace<MeshComponent>(treeEntity, treeMesh);
```

(A raw `new` here is fine as a first pass to get something on screen —
don't spend Day 5 building an asset-ownership system. If the mesh needs to
outlive the function or be shared/reused, that's a good Day 7+ cleanup, not
a blocker to seeing it render.)

If the model doesn't show up, check in this order — these are the classic
first-3D-model gotchas:

1. **Is the camera positioned/aimed at the model?** `glTF`/`.glb` models
   are often centered at the origin — if `Camera3D::Position` is also at
   the origin, you're inside the mesh. Back it up along Z.
   `aiProcess_FlipUVs` in `ModelLoader` handles texture coordinates, but
   nothing currently handles unit scale — some exporters produce models
   that are enormous or tiny relative to a `1.0f` default scale.
2. **Is the depth buffer actually wired up** (Day 2)? Without it, you may
   see flickering or the model rendering "inside out."
3. **Is `proj[1][1] *= -1.0f` present** in `Camera3D`? Its absence renders
   things upside down, which can look like "nothing is there" if the
   camera is also aimed wrong.
4. **Winding order / backface culling** — if the pipeline culls back faces
   and the model's winding order doesn't match Vulkan's expectations, you
   might be looking at the inside of the mesh. Try
   `VK_CULL_MODE_NONE` temporarily to rule this out while debugging.

Once one model renders, loading `Test1.glb` as a second entity is the same
three lines — a good moment to confirm multiple `MeshComponent` entities
render independently with their own transforms.

---

## Day 6 — event system

### Define event types

```cpp
struct KeyPressedEvent   { int Key; int Action; };
struct MouseMovedEvent   { double X, Y; };
struct WindowResizeEvent { int Width, Height; };
```

### Own a dispatcher

Add `entt::dispatcher m_Dispatcher` alongside the registry on `Game` (or a
small wrapper struct if you'd rather keep `Game` thin).

### Feed it from GLFW callbacks

Input is currently pure polling in `Game::HandleInput`
([src/Game/Game.cpp:77](src/Game/Game.cpp#L77)) via `glfwGetKey`, and no
GLFW callbacks are registered anywhere. Switch to callback-driven input in
`VulkanEngine::CreateWindow`:

```cpp
glfwSetWindowUserPointer(m_Window, this);

glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int, int action, int) {
    auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    engine->GetDispatcher().trigger<KeyPressedEvent>({ key, action });
});

glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
    auto* engine = static_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    engine->GetDispatcher().trigger<WindowResizeEvent>({ width, height });
});
```

### Subscribe

```cpp
dispatcher.sink<WindowResizeEvent>().connect<&RenderSystem::OnResize>(*this);
```

### ImGui callback collision

`ImGui_ImplGlfw_InitForVulkan(window, true)` (Day 1) auto-installs its own
GLFW callbacks. Now that you're installing your own too, pick one:

- Keep ImGui's `true` and forward events into your dispatcher from inside
  your own callback chain (call ImGui's installed callback first, then
  yours), or
- Pass `false` to `InitForVulkan` and call `ImGui_ImplGlfw_*Callback`
  functions manually at the top of your own callbacks.

Either works; just don't end up with two GLFW callbacks silently
overwriting each other (GLFW only keeps the most recently registered one
per event type).

### Enable window resizing

`GLFW_RESIZABLE` is currently hardcoded to `GLFW_FALSE` in `CreateWindow`,
even though `RecreateSwapChain()` already exists and works. Flip that hint,
subscribe `VulkanEngine`'s swapchain-recreation logic to
`WindowResizeEvent`, and you get live window resizing for free — the hard
part was already written, it just isn't triggered by anything.

---

## Day 7 — polish

- Build a small ImGui panel that iterates
  `registry.view<TransformComponent>()` and shows `ImGui::DragFloat3` for
  position/scale per entity — the fastest way to visually confirm Days 2-6
  are all working together, and genuinely useful for the rest of the
  project going forward.
- Fix the `Release|x64` / `Win32` vcxproj configs to have the same
  include/lib paths as `Debug|x64`, if you plan to build those.
- Remove `ImGui::ShowDemoWindow()` once you don't need it as a reference
  anymore (or keep it behind a debug toggle — genuinely handy for
  remembering what widgets exist).

---

## Checklist

- [ ] Day 1 — engine switched to dynamic rendering (no more render
      pass/framebuffers); ImGui compiles, links, demo window renders
- [ ] Day 2 — depth image created and attached via `vkCmdBeginRendering`;
      `Camera3D` produces view + projection matrices
- [ ] Day 3 — `TransformComponent`, `MeshComponent` added;
      `Mesh::UploadToGPU` extracted and reused by `QuadRenderer`
- [ ] Day 4 — `mesh.vert`/`mesh.frag` written and compiled;
      `MeshRenderer` has a working pipeline with depth testing on
- [ ] Day 5 — `ModelLoader` wired into entity creation; a `.glb` from
      `Assets/Models/` renders on screen
- [ ] Day 6 — `entt::dispatcher` added; GLFW input switched to callbacks;
      window resizing re-enabled and working
- [ ] Day 7 — ImGui entity inspector; Release/Win32 build configs fixed

---

## After the week: the road to a shippable Steam game

Everything above gets you a working renderer with debug tooling and a real
entity/component/event foundation. It does **not** yet get you a game you
could ship, and it doesn't yet consider two things you mentioned wanting
eventually: Steam integration and multiplayer. Both are much cheaper if a
few architectural decisions are made **now**, while the codebase is small,
rather than retrofitted once you have a real game's worth of gameplay code
built on top.

This section is a roadmap, not a day plan — treat it as "things to decide
and build, roughly in this order" rather than a strict schedule.

### Why multiplayer readiness is an architecture question, not a feature

The instinct is to treat multiplayer as a system you bolt on later ("add
networking"). In practice, the thing that makes multiplayer painful to
retrofit isn't the networking code itself — it's that single-player games
are usually written in a way that makes network sync *structurally hard*:
gameplay logic tangled with rendering, input handled by mutating state
directly instead of producing an intent, no clean point to snapshot "what
is the current game state," and no fixed simulation rate to keep clients
in sync against. None of that is Vulkan/rendering work, so it costs you
nothing to get right now, and it's exactly the kind of thing that's a full
rewrite to fix later. The four disciplines below are the actual
investment — adopt them while building single-player content and
multiplayer becomes "add a transport and a reconciliation step," not "redo
the game."

#### 1. Separate "engine" from "simulation"

Draw a hard line between things that only make sense on a machine with a
GPU and a window (`VulkanEngine`, `QuadRenderer`, `MeshRenderer`, ImGui)
and things that define what the game *is* (entity state, gameplay rules,
win conditions). Concretely: introduce a `GameSimulation` (or `World`)
class that owns the `entt::registry` and gameplay systems, and make sure
none of those systems ever touch Vulkan/GLFW/ImGui types. `RenderSystem`
*reads* from the registry to draw; it never has gameplay logic living
inside it. `Game` becomes the glue that owns both a `VulkanEngine` and a
`GameSimulation` and drives them each frame.

The payoff: a dedicated multiplayer server build later is just "compile
`GameSimulation` + networking code, without linking `VulkanEngine` or
ImGui at all" — a second small executable target, not a fork of the whole
codebase. Even if you never ship a dedicated server (peer-hosted is also
valid for a solo-dev game), this separation is what lets you unit-test
gameplay logic without spinning up a window, which you'll want regardless.

#### 2. Fixed-timestep simulation, decoupled from render framerate

Right now `Game::Run()` computes `deltaTime` from wall-clock time and
feeds it straight into `Update`. That's normal for single-player-only, but
any form of client-server sync (even just replay/rollback for a
single-player "undo," which is a nice side benefit) wants gameplay state
to advance in fixed, predictable ticks rather than however long the last
frame happened to take:

```cpp
const float FixedTimestep = 1.0f / 60.0f;
float accumulator = 0.0f;

while (m_IsRunning) {
    float frameTime = ComputeDeltaTime();
    accumulator += frameTime;

    while (accumulator >= FixedTimestep) {
        m_Simulation->Tick(FixedTimestep); // gameplay logic — deterministic, fixed rate
        accumulator -= FixedTimestep;
    }

    m_RenderSystem->RenderFrame(*m_Registry); // rendering still runs every frame, interpolated if needed
}
```

This alone is most of what "deterministic simulation" means in practice —
you're not required to go as far as full lockstep determinism (matching
floating-point results bit-for-bit across machines) unless you specifically
want peer-to-peer lockstep netcode. For a client-server model (the
recommended default below), fixed-tick is enough.

#### 3. Input becomes commands, not direct mutation

`Game::HandleInput` currently reads GLFW state and would naturally want to
mutate a `TransformComponent` directly. Instead, have input produce small
data structs describing *intent*:

```cpp
struct MoveCommand { glm::vec2 Direction; uint32_t Tick; };
```

Locally, these get pushed into a queue and applied to the simulation on
the next fixed tick. This is a small indirection for single-player, but
it's the single highest-leverage choice for multiplayer: a `MoveCommand`
is exactly what a client would later send to a server, and "replay the
last N commands from a known state" is the standard technique behind both
client-side prediction and a single-player rewind/replay feature. Building
input this way from day one means you're not restructuring input handling
when networking arrives — you're just adding a step that serializes the
same struct onto a socket.

#### 4. Keep components as plain, serializable data (you're already doing this)

`SpriteComponent`/`NameTag` are already POD structs with no pointers or
logic — keep every future component (`TransformComponent`, `MeshComponent`,
gameplay components like `HealthComponent`) the same way. EnTT ships
`entt::snapshot`/`entt::snapshot_loader` specifically for serializing an
entire registry's state to a byte stream and back — the same mechanism
works for save games *and* for sending/receiving world state over a
network. Avoid raw owning pointers inside gameplay components (`MeshComponent`
is an exception — meshes are render data, not gameplay state, so that's
fine); if a gameplay component needs to reference another entity, store an
`entt::entity` handle, which serializes as a plain integer.

#### 5. Decide the network model early, even if you build it late

For a solo dev shipping on Steam, **client-server with a server-authoritative
simulation** is the pragmatic default — the server (which can be one of the
players' machines acting as host, it doesn't need to be a dedicated
rented server) runs the real `GameSimulation`, clients send `Command`
structs and receive periodic state snapshots, and cheating/desync is much
easier to reason about than peer-to-peer lockstep. Steam's own networking
layer (Steam Networking Sockets / Steam Datagram Relay, part of
Steamworks) directly supports this model and additionally handles NAT
traversal for you — worth knowing now because it means you likely won't
need to write raw socket/UDP-hole-punching code yourself later, but it
does mean the "transport" you eventually plug into the command/snapshot
scheme above will come from the Steamworks SDK rather than a
general-purpose networking library.

None of this needs building now. The point of this section is only: build
the four things above while you build single-player content, and the
actual netcode becomes an additive feature months from now instead of a
rewrite.

### Steam integration

Steamworks integration is normally one of the later steps (you need a
Steam App ID, which requires an active developer account and, in
practice, a game far enough along to submit), but two things are worth
setting up early because they're cheap now and annoying to retrofit:

- **Wrap platform services behind your own interface.** Rather than
  calling `SteamAPI_*` functions directly from gameplay/UI code, define a
  small interface (`IPlatformServices` or similar) with methods like
  `UnlockAchievement(id)`, `SaveCloudFile(...)`, `IsOverlayOpen()`. Give it
  a Steam-backed implementation once you integrate the SDK, and a no-op
  implementation for everyday development (so you're not fighting the
  Steam overlay / requiring `steam_appid.txt` just to run the game while
  building gameplay features). This mirrors the engine/simulation split
  above — gameplay code shouldn't know or care which storefront it's
  running under.
- **`SteamAPI_Init()`/`SteamAPI_RunCallbacks()` placement.** When you do
  integrate, `RunCallbacks()` needs to run once per frame — the natural
  spot is right alongside `PollEvents()` in `Game::Run()`, so keep that
  loop structure in mind as you build it out this week.

Achievements, Steam Cloud saves, and rich presence are all straightforward
additions once the scene-serialization system (see below) exists, since
cloud saves are just "upload the same save file format you already have."

### The other systems a real game needs (roughly in priority order)

These aren't networking/Steam-specific, but they're the gap between "a
renderer with an ECS" and "a game," and worth sequencing deliberately:

1. **Scene serialization / save system.** Load initial entity state from a
   data file (JSON is fine to start) instead of hardcoding entities in
   `Game::CreateInitialEntities()`. This is also your save-game system and
   your future network-snapshot mechanism, so it's worth building once,
   well, rather than three separate times.
2. **Asset management.** Right now a `Mesh` is loaded with a raw `new`.
   Once you have more than a couple of models/textures, you'll want a
   simple registry that loads each asset once and hands out shared
   handles (`AssetId` or `std::shared_ptr<Mesh>`) instead of re-loading
   from disk per entity.
3. **Audio.** No audio system exists yet at all — even a minimal
   wrapper around a library like miniaudio (single-header, easy to vendor
   the same way `stb_image` is) unblocks a lot of "does this feel like a
   game" polish.
4. **Gameplay-facing UI.** ImGui is a *developer* tool — immediate-mode,
   not designed for a shipping game's main menu/HUD (no easy theming,
   awkward for controller navigation, etc.). Plan on a separate,
   lightweight UI approach for player-facing screens once you get there;
   don't build the real game UI in ImGui and expect to ship it as-is.
5. **Basic physics/collision**, scoped to whatever your game actually
   needs (full physics engine like Jolt/PhysX vs. simple AABB checks) —
   easy to over-build here, so let the game design decide how much you
   need rather than adding a physics engine speculatively.

### Suggested phase ordering after week one

| Phase | Focus | Notes |
|---|---|---|
| 2 | Materials/textures, lighting, basic physics/collision, audio | Round out the renderer + feel of the game |
| 3 | Gameplay layer, built with the command-driven, fixed-tick simulation split from day one | This is where "architecture for multiplayer" actually gets applied, even though multiplayer itself isn't being built yet |
| 4 | Scene serialization, asset management | Unlocks building actual content instead of hardcoded test entities |
| 5 | Gameplay-facing UI, polish, vertical slice | The point where it starts looking like a real game |
| 6 | Steamworks integration (achievements, cloud saves, App ID setup) | Cheap once phase 4's save system exists |
| 7 | Multiplayer (transport + reconciliation on top of the phase 3 architecture) | Only after there's a real game worth playing together — and only if the project still wants it at this point |
