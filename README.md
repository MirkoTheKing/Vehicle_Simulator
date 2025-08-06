# Vehicle Simulator - Computer Graphics Project

A sophisticated 3D vehicle simulation game built with Vulkan API, featuring realistic physics, collision detection, and a complete game progression system.

## 🎮 Game Overview

Vehicle Simulator is a driving game where players navigate through a detailed 3D city environment, visiting various locations in sequence while avoiding damage from collisions. The game features realistic car physics, dynamic camera controls, and multiple game states including splash screens, pause functionality, and win/lose conditions.

## 🚗 Key Features

### Core Gameplay
- **Realistic Vehicle Physics**: Speed-dependent steering, acceleration/deceleration, and momentum-based movement
- **Collision Detection**: Dynamic collision system with damage tracking (max 5 crashes before game over)
- **Progressive Mission System**: Visit 7 specific locations in sequence (Park → Bank → Parking → Factory → Store → Second Factory → Return to spawn)
- **Time Limit**: 5-minute timer adds urgency to complete all missions
- **Dynamic Camera**: Third-person camera that follows car rotation with manual camera controls

### Advanced Graphics Features
- **Vulkan-Based Rendering**: High-performance graphics using modern Vulkan API
- **Physically-Based Rendering (PBR)**: Realistic material rendering with metallic/roughness workflows
- **Dynamic Lighting**: Directional lighting with ambient lighting support
- **Multi-Pass Rendering**: Efficient render pipeline with multiple rendering passes
- **Text Rendering System**: Custom text rendering for UI elements and HUD

### Game States & UI
- **Splash Screen**: Welcome screen with game branding
- **Controls Screen**: Interactive tutorial showing game controls
- **Pause System**: ESC to pause, P to resume or restart
- **Real-time HUD**: Speed display, damage counter, mission instructions, and timer
- **Win/Lose Conditions**: Complete missions within time limit to win

## 🛠️ Technical Architecture

### Graphics Engine
- **Vulkan API**: Modern, low-level graphics API for maximum performance
- **Custom Engine Components**:
  - Scene management system with JSON-based level loading
  - Model loading (OBJ, GLTF, MGCG formats)
  - Texture management and sampling
  - Shader pipeline management
  - Descriptor set management

### Rendering Pipeline
1. **Geometry Pass**: Render 3D models with PBR shading
2. **UI Pass**: Overlay text and interface elements
3. **Post-processing**: Gamma correction and tone mapping

### Asset Management
- **3D Models**: City buildings, roads, car, and collectibles (.mgcg format)
- **Textures**: PBR materials, UI elements, and font atlases
- **Scene Definition**: JSON-based scene configuration system

## 🎯 Controls

| Key | Function |
|-----|----------|
| **W** | Accelerate forward |
| **S** | Brake/Reverse |
| **A/D** | Steer left/right (speed-dependent) |
| **Q/E** | Manual camera rotation |
| **SPACE** | Emergency brake |
| **P** | Progress through menus / Resume from pause / Restart game |
| **ESC** | Pause game (in-game) / Exit (when paused) |

## 🏗️ Project Structure

```
├── src/
│   ├── main.cpp           # Main application and game logic
│   ├── GameState.cpp      # Game state enumeration
│   ├── Timer.cpp          # Game timer implementation
│   └── Libs.cpp          # Library implementations
├── include/
│   └── modules/
│       ├── Starter.hpp    # Core Vulkan engine
│       ├── TextMaker.hpp  # Text rendering system
│       └── Scene.hpp      # Scene management
├── shaders/
│   ├── Mesh.vert/frag     # 3D model shaders (PBR)
│   ├── Text.vert/frag     # Text rendering shaders
│   └── Image.vert/frag    # Fullscreen image shaders
├── assets/
│   ├── models/            # 3D models and scene definition
│   └── textures/          # Textures and UI graphics
└── cmake-build-debug/     # Build output directory
```

## 🔧 Technical Implementation Details

### Physics System
The game implements realistic vehicle physics:
- **Acceleration**: Progressive speed buildup with maximum limits
- **Deceleration**: Natural friction when no input is provided
- **Speed-Dependent Steering**: Slower speeds allow tighter turns, faster speeds reduce steering responsiveness
- **Reverse Mechanics**: Backing up with inverted steering controls

### Collision Detection
- **Boundary-Based System**: Uses precise coordinate checking for road boundaries
- **Damage System**: Tracks collision count with visual feedback
- **Position Validation**: Prevents movement into invalid areas

### Rendering Features
- **PBR Shading**: Implements Cook-Torrance BRDF for realistic materials
- **Fresnel Reflections**: Accurate surface reflection calculations
- **Dynamic Uniform Buffers**: Per-object material properties
- **Multi-texture Support**: Diffuse, normal, and material property maps

### Memory Management
- **Descriptor Sets**: Efficient GPU resource management
- **Command Buffer Pooling**: Optimized command submission
- **Asset Streaming**: Intelligent loading of 3D models and textures

## 🎨 Visual Features

### Lighting Model
- **Directional Lighting**: Simulates sun lighting with proper shadow considerations
- **Ambient Lighting**: Global illumination for realistic scene brightness
- **Material Properties**: Metallic/roughness workflow for diverse surface types

### Post-Processing
- **Gamma Correction**: Proper color space handling for accurate display
- **Tone Mapping**: HDR to LDR conversion for realistic lighting

### UI Design
- **Real-time HUD**: Speed, damage, instructions, and timer
- **Responsive Text**: Scalable font rendering with multiple alignment options
- **State-Aware Interface**: Different UI layouts for different game states

## 🏆 Game Progression

### Mission Sequence
1. **Go to Small Park** - Tutorial area for learning controls
2. **Go to Bank** - Navigate through city streets
3. **Go to Parking Lot** - Precision driving challenge
4. **Go to Small Factory** - Industrial district navigation
5. **Go to Store (Next City)** - Cross-city travel
6. **Go to Second Factory** - Advanced navigation
7. **Return to Spawn** - Final challenge to complete the circuit

### Victory Conditions
- Complete all 7 missions in sequence
- Return to starting position within 5 minutes
- Sustain fewer than 5 collisions

### Failure Conditions
- Exceed 5 collision damage limit
- Run out of time (5-minute limit)

## 📋 Build Requirements

### Dependencies
- **Vulkan SDK** 1.4.304.0 or later
- **GLFW** 3.4+ for window management
- **GLM** for mathematics
- **CMake** 3.10+ for build system
- **C++17** compatible compiler

### Supported Platforms
- Windows (Primary)
- Linux (Ubuntu/similar distributions)
- macOS (with MoltenVK)

## 🚀 Building the Project

### Windows
```bash
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
cmake --build .
```

### Linux
```bash
sudo apt-get install vulkan-sdk libglfw3-dev libglm-dev
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

### macOS
```bash
brew install vulkan-sdk glfw glm
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

## 🔍 Asset Pipeline

### 3D Models
- **Format Support**: OBJ, GLTF, and custom MGCG formats
- **Compression**: MGCG format provides compressed, encrypted model storage
- **Scene Assembly**: JSON-based scene definition for flexible level design

### Textures
- **Format Support**: PNG with automatic SRGB detection
- **Mipmap Generation**: Automatic mipmap generation for optimal performance
- **Compression**: Efficient GPU texture formats

## 🏁 Getting Started

1. **Clone/Download** the project repository
2. **Install Dependencies** (Vulkan SDK, GLFW, GLM)
3. **Build** using CMake
4. **Run** the executable from the build directory
5. **Press P** to start from the splash screen
6. **Learn Controls** on the controls screen
7. **Begin Mission** and navigate to each location in sequence!

## 🎓 Educational Value

This project demonstrates:
- **Modern Graphics Programming** with Vulkan API
- **Game Engine Architecture** with modular design
- **3D Mathematics** for transformations and physics
- **Memory Management** in graphics applications
- **Asset Pipeline** development and optimization
- **User Interface** design for games
- **Game State Management** and progression systems

## 🤝 Contributing

This project was developed as part of a Computer Graphics course at Politecnico di Milano. The codebase showcases advanced graphics programming techniques and game development practices suitable for educational and portfolio purposes.

## 📜 License

This project is developed for educational purposes as part of a Computer Graphics course. All assets and code are intended for academic use and demonstration of graphics programming concepts.

---

