# SpaceRogueLite Project Guidelines

## Important Guidelines

- Do not run builds unless explicitly asked. The build process is slow and should only be triggered when the user requests it.

## Project Overview

SpaceRogueLite is a networked, procedurally-generated roguelike game with client/server architecture. It uses an Entity Component System (ECS) pattern with EnTT.

## Tech Stack

- **Language:** C++20
- **Build System:** CMake 3.15+ with Conan 2.x package management
- **Graphics:** SDL3, SDL3_image, SDL3_ttf, ImGui
- **Networking:** Yojimbo (client/server)
- **ECS:** EnTT 3.15.0
- **Math:** GLM
- **Logging:** spdlog
- **Procedural Generation:** Wave Function Collapse (fast-wfc)

## Build Commands

```bash
# Install dependencies
./scripts/install.sh [--clean]

# Build packages (internal libraries)
./scripts/build_packages.sh --build-type=Debug

# Full build
./scripts/build.sh Debug

# Build individual components
cd client && conan build . -s build_type=Debug
cd server && conan build . -s build_type=Debug
```

## Project Structure

```
core/       - Core game logic library (ECS components, game base class, procedural generation)
net/        - Header-only networking library (messages, handlers, transmitters)
graphics/   - Rendering library (SDL3, ImGui, shaders, render layers)
client/     - Client application
server/     - Server application
assets/     - Game assets (textures, tilesets)
```

## Architecture Patterns

### Entity Component System
- Components are POD structs in `core/include/components.h`
- Use `entt::registry` for entity management
- Use `entt::locator<T>` for shared services

### Worker Pattern (Game Loop)
- Inherit from `Game` base class
- Register workers via `attachWorker(name, lambda)`
- Worker signature: `void(int64_t timeSinceLastFrame, bool& quit)`

### Message-Based Networking
- Messages derive from `Message` base class in `net/`
- Register messages via `MESSAGE_LIST(X)` macro in `messagefactory.h`
- Implement `Serialize()` template for Yojimbo serialization
- Support command parsing via `parseFromCommand()`

### Command Parser (CRTP)
- Commands inherit from `Command<T>` using CRTP
- Static `name()` and `parse()` methods required
- Located in `client/src/commands/`

### Render Layers
- Inherit from `RenderLayer` base class
- Use layer ordering: BACKGROUND=0, TILES=100, ENTITIES=200, EFFECTS=300, UI=1000

## Coding Conventions

### Naming
- Classes: `PascalCase`
- Functions/methods: `camelCase`
- Constants/enums: `UPPER_SNAKE_CASE`
- Namespace: `SpaceRogueLite`

### Style
- 4 spaces indentation
- 100 character line limit
- clang-format based on Google style (see `.clang-format`)

### Headers
- Use `#pragma once`
- Order: system headers, third-party, project headers

### Error Handling
- Use spdlog for logging (trace, debug, info, warn, critical)
- Return `bool` for initialization success
- Use `std::optional` for operations that may fail

## Key Files

- `core/include/components.h` - ECS components
- `core/include/game.h` - Base game class
- `net/include/messagefactory.h` - Message registration macros
- `graphics/include/window.h` - SDL3 window management
- `graphics/include/renderlayers/renderlayer.h` - Render layer base class
- `assets/textures.json` - Texture definitions
