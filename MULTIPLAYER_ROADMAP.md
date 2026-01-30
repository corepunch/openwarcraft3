# OpenWarcraft3 Multiplayer Roadmap

This document outlines what's left to implement for a **basic playable multiplayer game** in OpenWarcraft3.

## Table of Contents
1. [Current Status Overview](#current-status-overview)
2. [Critical Path Items](#critical-path-items)
3. [Core Gameplay Systems](#core-gameplay-systems)
4. [Polish & Quality of Life](#polish--quality-of-life)
5. [Implementation Estimates](#implementation-estimates)

---

## Current Status Overview

### ✅ What's Already Implemented

OpenWarcraft3 has solid **infrastructure** for multiplayer gaming:

#### Network Architecture
- **Client-Server Architecture**: Properly separated, server-authoritative design
- **Entity Synchronization**: Delta compression for efficient bandwidth usage
- **Player Management**: Support for up to 24 clients (MAX_CLIENTS)
- **Message System**: Serialization/deserialization with delta encoding
- **Netchan Layer**: Reliable message transmission framework

#### Game Systems
- **Unit Management**: Spawn, movement, selection (including multi-unit selection)
- **AI System**: Basic AI behaviors (idle, stand, birth, death, pain)
- **Pathfinding**: Heatmap-based pathfinding with flow direction algorithm
- **Physics**: Movement types (step, toss, link), collision detection
- **Entity System**: Spawning for doodads, destructibles, units
- **JASS Scripting**: Full virtual machine for game scripts
- **Event System**: Game triggers and event handling
- **Item System**: Inventory and item management framework
- **UI Framework**: Server-side UI generation and synchronization

#### Commands & Abilities
- **Movement Commands**: move, attack, patrol, hold position, stop
- **Unit Orders**: Attack ground, waypoint movement
- **Build/Train Framework**: Commands exist but incomplete
- **13+ Abilities Implemented**: Including harvest, repair, goldmine, devotion aura, holy light, etc.

#### Map & Rendering
- **Map Loading**: W3M/W3X map file support
- **Terrain Rendering**: Ground, cliffs, water
- **Model Support**: MDX (Warcraft III) and M3 (StarCraft II) formats
- **Particle System**: Client-side visual effects
- **Fog of War**: Basic rendering implementation

### ❌ What's Missing for Playable Multiplayer

The project has **excellent foundations** but lacks several **critical gameplay systems**:

---

## Critical Path Items

These items are **essential blockers** for basic playable multiplayer and should be implemented first.

### 1. Real Network Implementation (CRITICAL)

**Status**: SDL_net code exists but disabled; currently using loopback buffers

**Location**: `src/common/net.c` has both implementations behind `#ifdef USE_LOOPBACK`

**What's Needed**:
- [ ] Remove `USE_LOOPBACK` preprocessor flag
- [ ] Enable SDL_net TCP/IP implementation
- [ ] Implement server socket creation (`NET_InitServer`)
- [ ] Implement client connection (`NET_Connect`)
- [ ] Add connection timeout handling
- [ ] Add disconnect detection
- [ ] Test multi-client connections

**Files to Modify**:
- `src/common/net.c` - Enable SDL_net implementation
- `src/common/net.h` - Update function signatures if needed
- `src/common/common.c` - Remove USE_LOOPBACK define
- `Makefile` / `premake5.lua` - Add SDL_net as dependency

**Complexity**: Medium (2-3 days)
**Priority**: 🔴 CRITICAL - Required for any networked multiplayer

---

### 2. Resource System (Economy)

**Status**: No resource tracking or management

**What's Needed**:
- [ ] Add gold/lumber fields to player state
- [ ] Implement resource gathering (harvest ability exists but incomplete)
- [ ] Resource deposit mechanics (return to base)
- [ ] Cost checking for units/buildings
- [ ] Resource display in UI
- [ ] Starting resources configuration

**Files to Create/Modify**:
- `src/game/g_resources.c` - New file for resource management
- `src/game/g_resources.h` - Resource system API
- `src/server/server.h` - Add gold/lumber to player state
- `src/game/skills/a_harvest.c` - Complete harvesting implementation
- UI files for resource display

**Complexity**: Medium (3-4 days)
**Priority**: 🔴 CRITICAL - Can't play RTS without resources

---

### 3. Building & Construction System

**Status**: Build command exists (`a_build.c`) but incomplete

**What's Needed**:
- [ ] Building placement system (check terrain, clearance)
- [ ] Construction state and progress tracking
- [ ] Worker assignment to construction
- [ ] Building completion detection
- [ ] Cancel construction (with refund)
- [ ] Building vision/fog of war reveal
- [ ] Placement preview rendering (client-side)

**Files to Modify**:
- `src/game/skills/a_build.c` - Complete build ability
- `src/game/g_construction.c` - New file for construction logic
- `src/game/g_spawn.c` - Building spawn support
- `src/renderer/r_ents.c` - Construction progress rendering
- UI command card updates

**Complexity**: High (5-7 days)
**Priority**: 🔴 CRITICAL - Core RTS mechanic

---

### 4. Combat System

**Status**: Basic damage function exists (`T_Damage`) but no targeting/attacks

**What's Needed**:
- [ ] Attack targeting system (find target in range)
- [ ] Attack range checking
- [ ] Attack cooldown/attack speed
- [ ] Damage calculation (armor types, attack types)
- [ ] Projectile entities (for ranged units)
- [ ] Death handling and corpses
- [ ] Attack animations synchronization
- [ ] Auto-attack behavior

**Files to Create/Modify**:
- `src/game/g_combat.c` - New combat system
- `src/game/g_combat.h` - Combat API
- `src/game/g_phys.c` - Update T_Damage function
- `src/game/skills/a_attack.c` - Complete attack ability
- `src/game/g_unitdata.h` - Add attack stats to units
- `src/game/g_ai.c` - Auto-attack AI behavior

**Complexity**: High (5-7 days)
**Priority**: 🔴 CRITICAL - Can't have game without combat

---

### 5. Unit Training System

**Status**: Train command exists (`a_train.c`) but incomplete

**What's Needed**:
- [ ] Training queue system
- [ ] Training progress tracking
- [ ] Resource deduction for training
- [ ] Queue display in UI
- [ ] Cancel training (with refund)
- [ ] Rally point system
- [ ] Unit spawn at completion

**Files to Modify**:
- `src/game/skills/a_train.c` - Complete training implementation
- `src/game/g_training.c` - New file for queue management
- `src/server/server.h` - Add queue to building entities
- UI files for queue display

**Complexity**: Medium (3-4 days)
**Priority**: 🟡 HIGH - Needed to build armies

---

### 6. Victory/Defeat Conditions

**Status**: Not implemented

**What's Needed**:
- [ ] Player elimination detection (no buildings/units)
- [ ] Victory condition checking
- [ ] Game end state handling
- [ ] Score calculation
- [ ] End game UI screen
- [ ] Post-game statistics

**Files to Create**:
- `src/game/g_victory.c` - Victory/defeat logic
- `src/game/g_victory.h` - Victory system API
- UI files for game end screen

**Complexity**: Low (1-2 days)
**Priority**: 🟡 HIGH - Defines when game ends

---

## Core Gameplay Systems

These systems are important for a **complete** multiplayer experience but not strictly required for "basic playable."

### 7. Improved AI

**Status**: Basic AI exists, needs expansion

**What's Needed**:
- [ ] AI player support (computer opponents)
- [ ] Build order AI
- [ ] Attack/defend strategies
- [ ] Resource management AI
- [ ] Difficulty levels

**Complexity**: Very High (10-14 days)
**Priority**: 🟢 MEDIUM - Nice to have for single player

---

### 8. Chat System

**Status**: Not implemented

**What's Needed**:
- [ ] Chat message protocol
- [ ] Chat UI overlay
- [ ] Team/all chat toggle
- [ ] Chat history

**Files to Create**:
- `src/game/g_chat.c`
- `src/client/cl_chat.c`
- UI chat overlay

**Complexity**: Low (1-2 days)
**Priority**: 🟡 HIGH - Essential for multiplayer communication

---

### 9. Minimap

**Status**: Fog of war rendering exists, no minimap

**What's Needed**:
- [ ] Minimap terrain rendering
- [ ] Unit/building icons on minimap
- [ ] Minimap click-to-move
- [ ] Ping system
- [ ] Vision circles

**Files to Create/Modify**:
- `src/renderer/r_minimap.c` - Minimap rendering
- `src/client/cl_input.c` - Minimap click handling

**Complexity**: Medium (3-4 days)
**Priority**: 🟡 HIGH - Important for strategy

---

### 10. Hero System

**Status**: Not implemented

**What's Needed**:
- [ ] Hero unit type
- [ ] Experience and leveling
- [ ] Stat points allocation
- [ ] Hero abilities/spells
- [ ] Hero death/respawn
- [ ] Hero inventory (item system exists)

**Complexity**: Very High (14-21 days)
**Priority**: 🟢 MEDIUM - Not needed for basic gameplay

---

### 11. Advanced Abilities

**Status**: 13 basic abilities exist, many more needed

**What's Needed**:
- [ ] Spell casting framework
- [ ] Area of effect abilities
- [ ] Buff/debuff system
- [ ] Cooldown system
- [ ] Mana/energy system
- [ ] Channeling abilities
- [ ] Passive abilities

**Complexity**: Very High (14+ days)
**Priority**: 🟢 MEDIUM - Adds depth but not required

---

### 12. Advanced Unit Features

**Status**: Basic units work, advanced features missing

**What's Needed**:
- [ ] Unit upgrades
- [ ] Tech tree requirements
- [ ] Unit formations
- [ ] Unit groups/control groups
- [ ] Attack-move command
- [ ] Guard/follow commands

**Complexity**: High (7-10 days)
**Priority**: 🟢 MEDIUM - Improves gameplay feel

---

## Polish & Quality of Life

These features improve the player experience but aren't blocking basic functionality.

### 13. Latency Compensation

**What's Needed**:
- [ ] Client-side prediction for movement
- [ ] Server reconciliation
- [ ] Interpolation between updates
- [ ] Lag compensation for attacks

**Complexity**: Very High (10+ days)
**Priority**: 🟢 LOW - Improves feel but not required initially

---

### 14. Sound System

**Status**: No sound implementation

**What's Needed**:
- [ ] Sound effect playback
- [ ] 3D positional audio
- [ ] Music system
- [ ] Voice line playback
- [ ] Volume controls

**Complexity**: Medium (4-5 days)
**Priority**: 🟢 LOW - Enhances experience

---

### 15. Replay System

**What's Needed**:
- [ ] Record game input commands
- [ ] Replay playback mode
- [ ] Fast forward/rewind
- [ ] Replay UI controls

**Complexity**: High (7-10 days)
**Priority**: 🟢 LOW - Nice to have feature

---

### 16. Match Lobby & Game Setup

**What's Needed**:
- [ ] Pre-game lobby UI
- [ ] Map selection
- [ ] Team assignment
- [ ] Game settings (speed, resources, etc.)
- [ ] Player ready status
- [ ] Game start synchronization

**Complexity**: Medium (4-5 days)
**Priority**: 🟡 HIGH - Important for organized games

---

### 17. Settings & Options

**What's Needed**:
- [ ] Graphics settings UI
- [ ] Keybind configuration
- [ ] Game speed options
- [ ] Settings persistence

**Complexity**: Low (2-3 days)
**Priority**: 🟢 LOW - Convenience feature

---

## Implementation Estimates

### Phase 1: Minimum Viable Multiplayer (3-4 weeks)
**Goal**: Two players can connect, gather resources, build units, and fight

- [ ] Network implementation (2-3 days) - **CRITICAL**
- [ ] Resource system (3-4 days) - **CRITICAL**
- [ ] Building system (5-7 days) - **CRITICAL**
- [ ] Combat system (5-7 days) - **CRITICAL**
- [ ] Unit training (3-4 days) - **HIGH**
- [ ] Victory conditions (1-2 days) - **HIGH**
- [ ] Chat system (1-2 days) - **HIGH**

**Total: ~20-29 days**

---

### Phase 2: Enhanced Gameplay (2-3 weeks)
**Goal**: More complete RTS experience with strategy depth

- [ ] Minimap (3-4 days)
- [ ] Match lobby (4-5 days)
- [ ] Advanced unit features (7-10 days)
- [ ] Improved AI (10-14 days) - optional

**Total: ~14-23 days**

---

### Phase 3: Polish & Features (3-4 weeks)
**Goal**: Professional-feeling game experience

- [ ] Sound system (4-5 days)
- [ ] Hero system (14-21 days)
- [ ] Advanced abilities (14+ days)
- [ ] Latency compensation (10+ days)
- [ ] Settings/options (2-3 days)

**Total: ~44-59 days**

---

## Recommended Implementation Order

### Week 1: Foundation
1. 🔲 **Enable SDL_net networking** (remove USE_LOOPBACK)
2. 🔲 **Test multi-client connections**
3. 🔲 **Implement resource tracking**

### Week 2-3: Core Mechanics
4. 🔲 **Complete combat system** (targeting, attacks, damage)
5. 🔲 **Finish building placement** and construction
6. 🔲 **Complete unit training** with queue

### Week 4: Game Loop
7. 🔲 **Victory/defeat conditions**
8. 🔲 **Chat system**
9. 🔲 **Basic UI improvements** (resources, unit stats)

### Week 5+: Enhancement
10. 🔄 Minimap
11. 🔄 Match lobby
12. 🔄 Additional features as desired

---

## Testing Milestones

### Milestone 1: "First Connection" (when networking done)
- [ ] Two clients can connect to server
- [ ] Clients can see each other's units move
- [ ] Basic synchronization works

### Milestone 2: "First Game" (after Phase 1)
- [ ] Start with resources
- [ ] Build workers
- [ ] Gather resources
- [ ] Build barracks
- [ ] Train units
- [ ] Units can attack
- [ ] Game ends when one player eliminated

### Milestone 3: "Fun to Play" (after Phase 2)
- [ ] Good UI/UX with minimap
- [ ] Smooth gameplay feel
- [ ] Strategic depth with tech trees
- [ ] Stable networked play

---

## Technical Debt & Infrastructure

### Known Issues to Address
1. **No error handling for network failures** - needs graceful disconnect handling
2. **No client prediction** - movement feels laggy (can be addressed in Phase 3)
3. **No map start locations** - players spawn at same position
4. **Fog of war incomplete** - exists but not fully functional
5. **No team support** - allies vs enemies not fully implemented

### Code Quality Improvements
- Add more unit tests for game logic
- Document network protocol
- Refactor large files (sv_main.c, cl_main.c)
- Improve error logging
- Add crash reporting

---

## Conclusion

OpenWarcraft3 has **excellent architectural foundations** for multiplayer RTS gaming. The client-server separation, entity synchronization, and command processing are well-designed and functional.

**To reach "basic playable multiplayer"**, the project needs:
- ✅ **Critical Path** (Phase 1): ~3-4 weeks of focused development
- 🎯 **Priority systems**: Networking, resources, building, combat, training
- 📈 **Current state**: ~60% infrastructure, ~30% gameplay systems

**Estimated timeline**: A single full-time developer could achieve basic playable multiplayer in **1 month**, with a polished experience in **2-3 months**.

The codebase is clean, well-organized, and ready for these additions. The hardest parts (architecture, rendering, synchronization) are already done!

---

## Contributing

To contribute to these goals:

1. **Pick an item** from the Critical Path or Core Gameplay sections
2. **Check existing code** in the relevant files listed
3. **Follow the existing patterns** for consistency
4. **Test with multiple clients** to ensure multiplayer works
5. **Submit PR** with clear description of what was implemented

Happy coding! 🎮
