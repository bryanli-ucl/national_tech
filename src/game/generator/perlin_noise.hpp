#pragma once

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace game::generator {

/**
 * @brief Perlin noise generator for procedural terrain generation
 *
 * Implements classic Perlin noise algorithm with:
 * - Permutation table for pseudo-random gradients
 * - Smooth interpolation using fade curves
 * - Fractal Brownian Motion (FBM) for multi-octave noise
 *
 * Uses Ken Perlin's improved noise function with smoothstep interpolation.
 */
class PerlinNoise {
    private:
    std::vector<int> p; // Permutation table (duplicated for overflow prevention)

    public:
    /**
     * @brief Construct a new Perlin Noise generator
     * @param seed Random seed for permutation table initialization
     */
    PerlinNoise(unsigned int seed = std::default_random_engine::default_seed);

    /**
     * @brief Generate 2D Perlin noise value
     *
     * Returns smooth, continuous pseudo-random values based on input coordinates.
     * Uses bilinear interpolation between gradient vectors at grid corners.
     *
     * @param x X coordinate in noise space
     * @param y Y coordinate in noise space
     * @return Noise value in range [-1, 1]
     */
    double noise(double x, double y);

    /**
     * @brief Generate Fractal Brownian Motion (FBM) noise
     *
     * Combines multiple octaves of Perlin noise at different frequencies
     * and amplitudes to create more natural-looking terrain patterns.
     * Each octave adds finer detail to the noise.
     *
     * @param x X coordinate in noise space
     * @param y Y coordinate in noise space
     * @param octaves Number of noise layers to combine (more = more detail)
     * @param persistence Amplitude multiplier per octave (controls roughness)
     * @return Normalized noise value in range [-1, 1]
     */
    double fbm(double x, double y, int octaves = 4, double persistence = 0.5);

    private:
    /**
     * @brief Fade function for smooth interpolation
     *
     * Uses 6t^5 - 15t^4 + 10t^3 curve (Perlin's improved smoothstep)
     * to create smooth transitions between grid cells.
     *
     * @param t Input value in range [0, 1]
     * @return Smoothed value in range [0, 1]
     */
    double fade(double t);

    /**
     * @brief Linear interpolation between two values
     * @param t Interpolation factor [0, 1]
     * @param a Start value
     * @param b End value
     * @return Interpolated value
     */
    double lerp(double t, double a, double b);

    /**
     * @brief Calculate gradient at grid point
     *
     * Computes dot product between pseudo-random gradient vector
     * and distance vector from grid point.
     *
     * @param hash Hash value determining gradient direction
     * @param x X distance from grid point
     * @param y Y distance from grid point
     * @return Gradient contribution
     */
    double grad(int hash, double x, double y);
};

} // namespace game::generator