#include "terrain_generator.hpp"

namespace game::generator {

/**
 * @brief Construct terrain generator with default parameters
 *
 * Default settings create moderately hilly terrain with:
 * - Scale 0.05: Gentle, rolling hills
 * - 4 octaves: Good detail without too much noise
 * - Persistence 0.5: Balanced roughness
 * - Base height 32: Mid-range elevation
 * - Max variation ±32: Height range [0, 64]
 * - Water level 28: Slightly below base height for lakes/rivers
 *
 * @param seed Random seed for reproducible worlds
 */
TerrainGenerator::TerrainGenerator(unsigned int seed)
: noise(seed),
  scale(0.05f),
  octaves(4),
  persistence(0.5f),
  baseHeight(32),
  maxHeight(32),
  waterLevel(28) {}

/**
 * @brief Calculate terrain surface height at given coordinates
 *
 * Process:
 * 1. Sample FBM noise at scaled coordinates
 * 2. Map noise value [-1, 1] to height range
 * 3. Add to base height for final Y coordinate
 *
 * Example with defaults:
 * - Noise returns -1.0 → height = 32 + (-1.0 * 32) = 0
 * - Noise returns 0.0 → height = 32 + (0.0 * 32) = 32
 * - Noise returns 1.0 → height = 32 + (1.0 * 32) = 64
 *
 * @param x World X coordinate
 * @param z World Z coordinate
 * @return Terrain surface Y coordinate
 */
int TerrainGenerator::getTerrainHeight(int x, int z) {
    // Generate FBM noise value in range [-1, 1]
    double noiseValue = noise.fbm(x * scale, z * scale, octaves, persistence);

    // Map to height range and add to baseline
    int height = baseHeight + static_cast<int>(noiseValue * maxHeight);

    return height;
}

/**
 * @brief Generate all blocks for a chunk
 *
 * A chunk is a vertical slice of the world from bedrock to sky.
 * This method generates terrain and water for a 16x16 (or custom size)
 * horizontal region.
 *
 * Generation process:
 * 1. Convert chunk coordinates to world coordinates
 * 2. For each XZ column in chunk:
 *    a. Calculate surface height from noise
 *    b. Generate blocks from Y=0 to surface height
 *    c. Fill water if below water level
 * 3. Log chunk statistics for debugging
 *
 * @param chunkX Chunk X index (multiply by chunkSize for world X)
 * @param chunkZ Chunk Z index (multiply by chunkSize for world Z)
 * @param chunkSize Chunk dimension in blocks (typically 16)
 * @return All terrain and water blocks in the chunk
 */
std::vector<TerrainBlock> TerrainGenerator::generateChunk(
int chunkX,
int chunkZ,
int chunkSize) {

    std::vector<TerrainBlock> blocks;

    // Convert chunk coordinates to world coordinates
    int startX = chunkX * chunkSize;
    int startZ = chunkZ * chunkSize;

    // Generate blocks for each XZ column in chunk
    for (int x = 0; x < chunkSize; x++) {
        for (int z = 0; z < chunkSize; z++) {
            int worldX = startX + x;
            int worldZ = startZ + z;

            // Get terrain surface height for this column
            int height = getTerrainHeight(worldX, worldZ);

            // Generate terrain blocks from bedrock to surface
            for (int y = 0; y <= height; y++) {
                uint32_t blockType = getBlockTypeAtPosition(worldX, y, worldZ, height);

                // Only add non-air blocks
                if (blockType != 0) {
                    blocks.emplace_back(glm::ivec3(worldX, y, worldZ), blockType);
                }
            }

            // Fill water if terrain is below water level
            if (height < waterLevel) {
                for (int y = height + 1; y <= waterLevel; y++) {
                    blocks.emplace_back(
                    glm::ivec3(worldX, y, worldZ),
                    ::game::blocks::BlockIDs::WATER);
                }
            }
        }
    }

    // Log chunk generation statistics
    if (blocks.size() > 0) {
        LOG_DEBUG("Chunk (", chunkX, ", ", chunkZ, ") height range: ",
        baseHeight, " +- ", maxHeight, ", blocks: ", blocks.size());
    }

    return blocks;
}

/**
 * @brief Generate flat rectangular terrain region
 *
 * Similar to chunk generation but creates a custom-sized rectangular
 * area centered at specified coordinates. Useful for:
 * - Initial spawn platforms
 * - Testing specific terrain features
 * - Pre-generating visible area around player
 *
 * @param sizeX Width of region in blocks
 * @param sizeZ Depth of region in blocks
 * @param centerX Center X world coordinate
 * @param centerZ Center Z world coordinate
 * @return All terrain blocks in the region (no water)
 */
std::vector<TerrainBlock> TerrainGenerator::generateFlatTerrain(
int sizeX,
int sizeZ,
int centerX,
int centerZ) {

    std::vector<TerrainBlock> blocks;

    // Calculate region bounds centered on specified point
    int startX = centerX - sizeX / 2;
    int startZ = centerZ - sizeZ / 2;

    // Generate terrain for rectangular region
    for (int x = 0; x < sizeX; x++) {
        for (int z = 0; z < sizeZ; z++) {
            int worldX = startX + x;
            int worldZ = startZ + z;

            int height = getTerrainHeight(worldX, worldZ);

            // Generate terrain layers (no water in this method)
            for (int y = 0; y <= height; y++) {
                uint32_t blockType = getBlockTypeAtPosition(worldX, y, worldZ, height);

                if (blockType != 0) {
                    blocks.emplace_back(glm::ivec3(worldX, y, worldZ), blockType);
                }
            }
        }
    }

    return blocks;
}

/**
 * @brief Determine appropriate block type based on position and context
 *
 * Implements layered terrain structure with biome-based surface blocks:
 *
 * Layer structure (from top to bottom):
 * 1. Surface (Y = surfaceHeight):
 *    - High altitude (>70% max height): Stone mountains
 *    - Near water (±2 blocks): Sandy beaches
 *    - Normal altitude: Grass-covered plains
 *
 * 2. Subsurface (1-3 blocks below surface):
 *    - Below beaches: Sand continues
 *    - Below grass: Dirt layer
 *
 * 3. Deep underground (>3 blocks below surface):
 *    - Stone
 *
 * 4. Bedrock (Y = 0):
 *    - Stone (representing unbreakable bottom)
 *
 * @param x World X coordinate (reserved for future biome logic)
 * @param y World Y coordinate (current depth)
 * @param z World Z coordinate (reserved for future biome logic)
 * @param surfaceHeight Surface Y coordinate for this column
 * @return Block type ID (AIR = 0 if no block needed)
 */
uint32_t TerrainGenerator::getBlockTypeAtPosition(
__attribute_maybe_unused__ int x,
__attribute_maybe_unused__ int y,
__attribute_maybe_unused__ int z,
int surfaceHeight) {

    using namespace ::game::blocks::BlockIDs;

    // Bedrock layer at bottom of world
    if (y == 0) {
        return STONE;
    }

    // Surface layer - biome determination
    if (y == surfaceHeight) {
        // High elevation mountains - bare stone
        if (y > baseHeight + maxHeight * 0.7) {
            return STONE;
        }
        // Beach biome - near water level
        else if (y <= waterLevel + 2) {
            return SAND;
        }
        // Grassland biome - normal elevation
        else {
            return GRASS;
        }
    }

    // Subsurface layer (1-3 blocks below surface)
    if (y > surfaceHeight - 3 && y < surfaceHeight) {
        // Sand continues below beaches
        if (surfaceHeight <= waterLevel + 2) {
            return SAND;
        }
        // Dirt layer below grassland
        else {
            return DIRT;
        }
    }

    // Deep underground - solid stone
    if (y < surfaceHeight - 3) {
        return STONE;
    }

    // Above surface - air
    return AIR;
}

} // namespace game::generator
