# BLEND DNA emit map - typed vs sidecar

Branch split (Arris policy):

| Layer | Branch | What lands here |
| --- | --- | --- |
| Assimp typed | `(improve)-blender-2x-5x` | `aiMesh` / `aiMaterial` / `aiTexture` / `aiLight` / `aiCamera` / `aiNode` / `aiBone` / `aiAnimation`+`aiNodeAnim` / node `aiMetadata` |
| Sidecar bags | `feature/blend-sidecar` (on top of improve) | `aiMaterialSidecar` / `aiAnimationSidecar` / `aiSceneSidecar` / Physics JSON (`BLEND_*`) |
| Integration | `LocalMergeForEasyRebuild` | merge only |

Presence: emit a key only when DNA/SDNA actually resolved it. No invent.

## Object classes

| DNA / Blender | Typed Assimp | Sidecar (feature) |
| --- | --- | --- |
| Mesh / CustomData / UV | `aiMesh` (+ UV names) | Mesh sidecar layers bag when needed |
| Material / Principled / maps | `aiMaterial` + embeds `aiTexture` | `BLEND_shader*` / network / volume JSON |
| Lamp | `aiLight` | Scene `lights[]` inventory (+ extras) |
| Camera (lens/sensor/ortho/clip) | `aiCamera` (`mHorizontalFOV`/`mAspect`/`mOrthographicWidth`) | `cameras[]` DOF / `sceneCamera` |
| Object TRS / hierarchy | `aiNode` | - |
| Armature / pose.bones Action | bone `aiNode` + `aiNodeAnim` | - |
| Camera / Empty TRS Action | `aiNodeAnim` | Modifier-prop / driver channels if not typed |
| World env image | typed env map when Assimp slot exists | Scene/World bag remainder |
| ParticleSettings / ParticleSystem | emitter `aiNode.mMetaData` `BLEND.ParticleSystem.*` (no `aiParticle*`) | `BLEND_physics_particles` + `BLEND_point_instancers` |
| Soft/Cloth/Fluid/Rigid/Ocean | - | Physics bags / KHR-shaped |
| Scene.active camera / world | - | `BLEND_scene` |

## ParticleSettings fields (emission / instance)

| DNA / RNA | Typed metadata key | Sidecar JSON key | Notes |
| --- | --- | --- | --- |
| `type` | `...type` | `part_type` | |
| `from` | `...from` | `from` | emit_from VERT/FACE/VOLUME |
| `distr` | `...distr` | `distr` | JIT/RAND/GRID |
| `ren_as` | `...ren_as` | `ren_as` | 7=Object 8=Collection |
| `phystype` | `...phystype` | `phystype` | Presence |
| `draw_as` | `...draw_as` | `draw_as` | Presence |
| `draw` | `...draw` | `draw` | Presence draw flags |
| `totpart` | `...totpart` | `totpart` | |
| `userjit` | `...userjit` | `userjit` | Presence; 0=auto |
| `grid_res` | `...grid_res` | `grid_res` | Presence |
| `grid_rand` | `...grid_rand` | `grid_rand` | Presence |
| `mass`/`size`/`randsize` | same | same | |
| `lifetime`/`sta`/`end` | same | `lifetime`/`frame_start`/`frame_end` | |
| `jitfac` | `...jitfac` | `jitfac` | |
| `randlife`/`randlength` | Presence | Presence | |
| `hair_step` | Presence | Presence | |
| `normfac` | `...normfac` | `normfac` | Presence |
| RNA `hair_length` | `...hair_length` | `hair_length` | `normfac * 4` when `normfac` present |
| `ParticleSystem.seed` | `...seed` | `seed` | |
| instance collection/object | `...collection` / `...instanceObject` | same | |
| Instance TRS arrays | - | `positions`/`orients`/`scales`/`protoIndices` + `arraysResolved` | only when DNA particles present or Arris bake |

