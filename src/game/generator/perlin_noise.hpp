#pragma once


#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace game::generator {

class PerlinNoise {
    private:
    std::vector<int> p; // 置换表

    public:
    // 构造函数，可以指定种子
    PerlinNoise(unsigned int seed = std::default_random_engine::default_seed) {
        p.resize(256);

        // 初始化置换表
        for (int i = 0; i < 256; i++) {
            p[i] = i;
        }

        // 使用种子打乱
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);

        // 复制一份，避免溢出
        p.insert(p.end(), p.begin(), p.end());
    }

    // 2D柏林噪声
    double noise(double x, double y) {
        // 找到单位网格的坐标
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;

        // 找到相对于网格的坐标
        x -= floor(x);
        y -= floor(y);

        // 计算淡入淡出曲线
        double u = fade(x);
        double v = fade(y);

        // 哈希四个角的坐标
        int A = p[X] + Y;
        int B = p[X + 1] + Y;

        // 插值计算
        return lerp(v,
        lerp(u, grad(p[A], x, y), grad(p[B], x - 1, y)),
        lerp(u, grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1)));
    }

    // 分形布朗运动（多层噪声叠加）
    double fbm(double x, double y, int octaves = 4, double persistence = 0.5) {
        double total     = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double maxValue  = 0.0;

        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency) * amplitude;

            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }

        return total / maxValue;
    }

    private:
    // 淡入淡出函数
    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // 线性插值
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    // 梯度函数
    double grad(int hash, double x, double y) {
        int h    = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x :
                                                    0;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
};

} // namespace game::generator