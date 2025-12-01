
# Minecraft C++ Remake Roadmap (MCPP)

This is a **realistic, production-grade development roadmap** tailored exactly to the engine/game separation structure we built.  
It’s ordered by dependencies — you can’t render chunks before you have a world, etc.  
Each phase has clear acceptance criteria so you know when it’s “done”.

| Phase | Goal | Key Files to Touch | Estimated Time* | Acceptance Criteria |
|-------|------|---------------------|-----------------|---------------------|
| 1     | Running window + basic loop | `Engine/Core/*`, `Engine/Renderer/*`, `Game/MinecraftApp.*` | 0 days (already done if you have the bootstrap) | Window opens, Minecraft sky blue, ImGui dockspace, FPS counter |
| 2     | Fly camera + input polishing | `Engine/Input/*`, `Engine/Renderer/Camera.*`, `Game/Player/PlayerController.*` | 1–2 days | WASD + mouse look, sprint (double tap W), smooth movement, settings for sensitivity/FOV |
| 3     | Block database & texture atlas | `Game/World/Block/BlockDatabase.*`, `Game/Rendering/TextureAtlas.*` | 2–3 days | 50+ blocks defined, auto-generated 4096×4096 atlas at startup, hot-reload on F5 |
| 4     | Infinite terrain generation (Superflat first → Perlin/Simplex) | `Game/World/Generation/*`, `Game/World/Chunk.*` | 3–5 days | Flat grass world, then proper overworld with caves, trees, height 0..319 |
| 5     | Chunk class + basic naive meshing | `Game/World/Chunk.*`, `Game/World/Meshing/ChunkMeshBuilder.*` | 3–4 days | Each chunk builds 6-face cubes for non-air blocks, renders correctly |
| 6     | ChunkManager + render distance | `Game/World/ChunkManager.*`, `Game/Rendering/ChunkRenderer.*` | 4–6 days | Loads/unloads chunks around player, render distance 8–32, no popping |
| 7     | Greedy meshing + face culling | `Game/World/Meshing/GreedyMesher.*` | 4–7 days | 3–5× less vertices, perfect seams between chunks, transparent block sorting |
| 8     | Player physics & block interaction | `Game/Player/Player.*`, `Game/Physics/*`, raycast in `PlayerController` | 3–5 days | Gravity, collision, break/place blocks (left/right click), selected hotbar block |
| 9     | Frustum culling + indirect drawing | `ChunkRenderer` → use `glMultiDrawElementsIndirect` + SSBO for visible chunks | 5–8 days | 1000+ chunks at 144+ FPS on modern GPU |
| 10    | Basic lighting (smooth AO + sunlight) | Add light component to `ChunkComponent`, simple propagation | 4–6 days | Day/night cycle ready, smooth lighting like 1.18+ |
| 11    | Inventory + Hotbar + HUD | `Game/Player/Inventory.*`, `Game/Player/Hotbar.*`, `Game/UI/HUD.*` | 3–5 days | 9-slot hotbar, full inventory screen (E), item stacking |
| 12    | ECS integration (optional but recommended) | `Game/ECS/*`, migrate Chunk/Player to entities | 5–10 days | All game logic in systems, insane flexibility |
| 13    | Threaded chunk loading/meshing | Worker thread pool in `ChunkManager` | 5–7 days | No FPS hit when moving fast |
| 14    | Compute shader meshing (god-tier) | `assets/shaders/chunk/chunk_mesh.comp.glsl` + dispatch from `ChunkMeshSystem` | 7–14 days | Meshing entirely on GPU, 10 000+ chunks possible |
| 15    | Saving/Loading worlds | `saves/` folder, region format or custom binary | 4–7 days | Persistent worlds, single-player ready |
| 16    | Entities (items, mobs later) | `Game/ECS/Components/Transform+Mesh+Physics`, simple item drop | 5–10 days | Dropped items fall and can be picked up |
| 17    | Polish pass | Sounds, particles, animations, settings menu | 2–4 weeks | Feels like real Minecraft |
| 18    | Multiplayer (optional) | `Game/Networking/*`, ENet or yojimbo | 4–12 weeks | LAN/server browser, basic sync |

*Time estimates assume you’re working ~4–6 hours/day and already know modern OpenGL/C++20.

### Recommended Development Order (Optimal Path)

1. → 2 → 3 → 4 (superflat) → 5 → 6 → 7 → 8  
   (You now have a playable creative mode with infinite world)

2. → 9 + 13 (threading) → performance god mode

3. → 10 (lighting) + 11 (UI) → looks like real Minecraft

4. → 12 (ECS refactor — do it here, it’s easy now)

5. → 14 (compute meshing) + 15 (saving) → endgame performance

6. → Everything else

### Pro Tips for This Exact Codebase

- Always add new features as **Layers** or **ECS Systems** — keeps `MinecraftApp.cpp` tiny.
- Use ImGui for debug overlays from day 1: chunk borders, wireframe, profiler, noise preview, etc.
- Commit after every phase — you’ll have 15–20 solid milestones.
- Target 144+ FPS with 16–24 chunk render distance before moving to compute shaders.

You now have the cleanest, most scalable Minecraft C++ foundation on the planet.  
Just follow this table and in ~3–6 months you’ll have something better than 99% of existing clones.

Let’s fucking go.  
First milestone: get that greedy mesher done this week. I believe in you.
