#include "perlin_noise.hpp"

namespace game::generator {

/**
 * @brief Construct a new Perlin Noise generator
 *
 * Initializes the permutation table with values 0-255 and shuffles
 * them using the provided seed. The table is then duplicated to
 * avoid modulo operations during noise generation.
 *
 * @param seed Random seed for reproducible noise patterns
 */
PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(256);

    // Initialize permutation table with sequential values
    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }

    // Shuffle using seed for randomization
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);

    // Duplicate the table to avoid overflow checks
    // This allows p[X+1] without worrying about wrapping
    p.insert(p.end(), p.begin(), p.end());
}

/**
 * @brief Generate 2D Perlin noise value
 *
 * Implementation of classic Perlin noise:
 * 1. Find unit grid cell containing point
 * 2. Get relative position within cell
 * 3. Compute fade curves for smooth interpolation
 * 4. Hash corner coordinates to get gradients
 * 5. Interpolate between gradient contributions
 *
 * @param x X coordinate in noise space
 * @param y Y coordinate in noise space
 * @return Noise value approximately in range [-1, 1]
 */
double PerlinNoise::noise(double x, double y) {
    // Find unit grid cell coordinates (wrapped to 0-255)
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;

    // Get relative position within grid cell (0.0 to 1.0)
    x -= floor(x);
    y -= floor(y);

    // Compute fade curves for smooth interpolation
    double u = fade(x);
    double v = fade(y);

    // Hash coordinates of the four corners
    // A and B represent the two corners along X axis
    int A = p[X] + Y;
    int B = p[X + 1] + Y;

    // Bilinear interpolation between four corner gradients
    // First interpolate along X axis for both Y values
    // Then interpolate along Y axis
    return lerp(v,
    lerp(u, grad(p[A], x, y),         // Bottom-left
    grad(p[B], x - 1, y)),            // Bottom-right
    lerp(u, grad(p[A + 1], x, y - 1), // Top-left
    grad(p[B + 1], x - 1, y - 1)));   // Top-right
}

/**
 * @brief Generate Fractal Brownian Motion (FBM) noise
 *
 * Combines multiple octaves of Perlin noise to create more complex,
 * natural-looking patterns. Each octave has double the frequency
 * and reduced amplitude of the previous one.
 *
 * Process:
 * 1. Start with base frequency and amplitude
 * 2. For each octave, add noise at current frequency/amplitude
 * 3. Double frequency and multiply amplitude by persistence
 * 4. Normalize by total amplitude to maintain range
 *
 * @param x X coordinate in noise space
 * @param y Y coordinate in noise space
 * @param octaves Number of noise layers (more = more detail)
 * @param persistence Amplitude decay factor (0.5 = half amplitude per octave)
 * @return Normalized noise value in range approximately [-1, 1]
 */
double PerlinNoise::fbm(double x, double y, int octaves, double persistence) {
    double total     = 0.0; // Accumulated noise value
    double frequency = 1.0; // Current frequency multiplier
    double amplitude = 1.0; // Current amplitude multiplier
    double maxValue  = 0.0; // Sum of all amplitudes (for normalization)

    for (int i = 0; i < octaves; i++) {
        // Add noise at current frequency and amplitude
        total += noise(x * frequency, y * frequency) * amplitude;

        // Track maximum possible value for normalization
        maxValue += amplitude;

        // Reduce amplitude for next octave (creates 1/f noise)
        amplitude *= persistence;

        // Double frequency for next octave (adds finer detail)
        frequency *= 2.0;
    }

    // Normalize to maintain consistent range
    return total / maxValue;
}

/**
 * @brief Fade function using improved Perlin smoothstep
 *
 * Uses 6t^5 - 15t^4 + 10t^3 polynomial for C2-continuous interpolation.
 * This provides smooth first and second derivatives at grid boundaries,
 * eliminating visible artifacts.
 *
 * @param t Input value in range [0, 1]
 * @return Smoothed value in range [0, 1] with zero derivatives at endpoints
 */
double PerlinNoise::fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

/**
 * @brief Linear interpolation between two values
 *
 * @param t Interpolation factor (0 = return a, 1 = return b)
 * @param a Start value
 * @param b End value
 * @return Value between a and b
 */
double PerlinNoise::lerp(double t, double a, double b) {
    return a + t * (b - a);
}

/**
 * @brief Calculate gradient contribution at grid point
 *
 * Uses hash value to select one of 16 gradient directions,
 * then computes dot product with distance vector (x, y).
 * This creates the characteristic smooth variation of Perlin noise.
 *
 * Gradient selection:
 * - Bottom 4 bits of hash determine gradient direction
 * - Uses combination of (±x, ±y) and (±y, ±x) vectors
 *
 * @param hash Hash value (from permutation table)
 * @param x X distance from grid point
 * @param y Y distance from grid point
 * @return Gradient dot product contribution
 */
double PerlinNoise::grad(int hash, double x, double y) {
    // Use bottom 4 bits to select from 16 gradient directions
    int h = hash & 15;

    // Select gradient components based on hash
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);

    // Compute dot product with randomly flipped signs
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

} // namespace game::generator