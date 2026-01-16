#pragma once

#include "game/blocks/blocks.hpp"
#include "game/blocks/blocks_types.hpp"
#include "game/generator/perlin_noise.hpp"
#include "utils/logger/logger.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace game::generator {

/**
 * @brief Represents a single block in generated terrain
 * Contains world position and block type identifier
 */
struct TerrainBlock {
    glm::ivec3 position;  // World position (x, y, z)
    uint32_t blockTypeId; // Block type identifier

    TerrainBlock(const glm::ivec3& pos, uint32_t id)
    : position(pos), blockTypeId(id) {}
};

/**
 * @brief Procedural terrain generator using Perlin noise
 *
 * Generates realistic 3D terrain with:
 * - Height-based biomes (grass, sand, stone)
 * - Multi-layer subsurface structure (grass/dirt/stone)
 * - Water bodies at configurable sea level
 * - Chunk-based generation for efficient world streaming
 *
 * Uses Fractal Brownian Motion (FBM) for natural-looking height variation.
 */
class TerrainGenerator {
    private:
    game::generator::PerlinNoise noise; // Noise generator for height maps

    // Terrain generation parameters
    float scale;       // Noise sampling scale (smaller = more zoomed out)
    int octaves;       // Number of noise layers (more = more detail)
    float persistence; // Amplitude decay per octave (controls roughness)
    int baseHeight;    // Baseline terrain height (sea level offset)
    int maxHeight;     // Maximum height variation above/below baseline
    int waterLevel;    // Y-level for water surface

    public:
    /**
     * @brief Construct a new Terrain Generator
     * @param seed Random seed for reproducible terrain
     */
    TerrainGenerator(unsigned int seed = 12345);

    // Parameter setters for terrain customization
    void setScale(float s) { scale = s; }
    void setOctaves(int o) { octaves = o; }
    void setPersistence(float p) { persistence = p; }
    void setBaseHeight(int h) { baseHeight = h; }
    void setMaxHeight(int h) { maxHeight = h; }
    void setWaterLevel(int w) { waterLevel = w; }

    /**
     * @brief Calculate terrain height at given coordinates
     *
     * Uses FBM noise to generate smooth, natural-looking height variation.
     * Height is mapped from noise range [-1, 1] to [baseHeight - maxHeight, baseHeight + maxHeight].
     *
     * @param x World X coordinate
     * @param z World Z coordinate
     * @return Terrain surface height (Y coordinate)
     */
    int getTerrainHeight(int x, int z);

    /**
     * @brief Generate terrain blocks for a chunk
     *
     * Creates all blocks within a chunk region, including:
     * - Terrain from bedrock to surface
     * - Water bodies where terrain is below water level
     * - Proper block types based on height and depth
     *
     * @param chunkX Chunk X coordinate (in chunk space)
     * @param chunkZ Chunk Z coordinate (in chunk space)
     * @param chunkSize Size of chunk in blocks (default 16x16)
     * @return Vector of all blocks in the chunk
     */
    std::vector<TerrainBlock> generateChunk(int chunkX, int chunkZ, int chunkSize = 16);

    /**
     * @brief Generate flat rectangular terrain region
     *
     * Similar to chunk generation but creates a rectangular area
     * centered at specified coordinates. Useful for initial spawn areas.
     *
     * @param sizeX Width in blocks
     * @param sizeZ Depth in blocks
     * @param centerX Center X coordinate (default 0)
     * @param centerZ Center Z coordinate (default 0)
     * @return Vector of all blocks in the region
     */
    std::vector<TerrainBlock> generateFlatTerrain(
    int sizeX,
    int sizeZ,
    int centerX = 0,
    int centerZ = 0);

    private:
    /**
     * @brief Determine block type based on position and terrain height
     *
     * Implements biome and layer logic:
     * - Y=0: Bedrock (stone)
     * - Surface: Grass, sand, or stone based on elevation
     * - Subsurface (1-3 blocks deep): Dirt or sand
     * - Deep underground: Stone
     *
     * Biome rules:
     * - High elevation (>70% of max height): Stone mountains
     * - Near water level (±2 blocks): Sandy beaches
     * - Normal elevation: Grass-covered terrain
     *
     * @param x World X coordinate (unused in current implementation)
     * @param y World Y coordinate (depth)
     * @param z World Z coordinate (unused in current implementation)
     * @param surfaceHeight Height of terrain surface at this column
     * @return Block type ID (or AIR if no block should be placed)
     */
    uint32_t getBlockTypeAtPosition(
    __attribute_maybe_unused__ int x,
    __attribute_maybe_unused__ int y,
    __attribute_maybe_unused__ int z,
    int surfaceHeight);
};

} // namespace game::generator