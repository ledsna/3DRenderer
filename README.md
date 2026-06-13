# 3D Renderer

<img width="1280" height="798" alt="image" src="https://github.com/user-attachments/assets/13a2d74a-f3c4-41a3-bf54-c14df4cccee0" />
<img width="1280" height="798" alt="image" src="https://github.com/user-attachments/assets/8573b2c6-dd8c-46d0-909d-4f2549817b2a" />
<img width="1280" height="798" alt="image" src="https://github.com/user-attachments/assets/9ca233fd-dcac-4ec3-a0e3-d7376abae980" />

CPU-based 3D rendering engine with Blinn-Phong shading, shadow mapping, and interactive UI.

## Prerequisites

- CMake 3.10+
- C++20 compatible compiler (GCC 8+, Clang 8+, MSVC 19.14+)
- Git

## Building

```bash
# Clone repository
git clone https://github.com/ledsna/3DRenderer.git
cd 3DRenderer

# Initialize submodules (SFML, Eigen)
git submodule update --init --recursive

# Configure and build
cmake -S . -B build
cmake --build build -j 6

# Run
./build/3d-renderer
```

## Features

- Full 3D rendering pipeline with perspective-correct rasterization
- Blinn-Phong (ambient, directional, point lights)
- Shadow mapping with PCF filtering
- Alpha blending
- Importing .obj & smooth normals generation
- Interactive scene view (WASD + mouse look)
- UI with transform gizmos
- Debug visualization modes (depth, normals, wireframe)

## Controls

**Camera:** WASD to move, Right-click + drag to look around, Shift/Space for up/down  
**Objects:** Left-click to select, drag gizmo arrows to move  
**UI:** Inspector (left), Hierarchy (right), View modes (bottom)

## Demo

... а пока нету лол
