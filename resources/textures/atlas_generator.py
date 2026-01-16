#!/usr/bin/env python3
"""
8x8 纹理图集生成器
支持任意数量的纹理，自动计算最佳图集尺寸
"""

from PIL import Image
import os
import json
import math

class TextureAtlasBuilder:
    def __init__(self, texture_size=8, target_atlas_size=1024):
        self.texture_size = texture_size
        self.target_atlas_size = target_atlas_size
        self.textures_per_row = target_atlas_size // texture_size
        self.max_textures = self.textures_per_row ** 2
        
    def create_atlas(self, texture_files, output_path, metadata_path=None):
        """
        创建纹理图集
        
        参数:
            texture_files: 纹理文件路径列表
            output_path: 输出图集路径
            metadata_path: 元数据输出路径（JSON格式）
        """
        num_textures = len(texture_files)
        
        if num_textures > self.max_textures:
            print(f"警告: 纹理数量 ({num_textures}) 超过最大容量 ({self.max_textures})")
            print(f"将只处理前 {self.max_textures} 个纹理")
            texture_files = texture_files[:self.max_textures]
            num_textures = self.max_textures
        
        # 计算实际需要的图集大小（可以优化为更小的尺寸）
        textures_per_side = math.ceil(math.sqrt(num_textures))
        actual_atlas_size = textures_per_side * self.texture_size
        
        print(f"纹理尺寸: {self.texture_size}x{self.texture_size}")
        print(f"纹理数量: {num_textures}")
        print(f"图集布局: {textures_per_side}x{textures_per_side}")
        print(f"图集尺寸: {actual_atlas_size}x{actual_atlas_size}")
        print(f"利用率: {num_textures / (textures_per_side ** 2) * 100:.1f}%")
        
        # 创建图集
        atlas = Image.new('RGBA', (actual_atlas_size, actual_atlas_size), (0, 0, 0, 0))
        
        # 元数据：记录每个纹理的位置
        metadata = {
            'texture_size': self.texture_size,
            'atlas_size': actual_atlas_size,
            'textures_per_row': textures_per_side,
            'textures': {}
        }
        
        # 粘贴纹理
        for idx, tex_file in enumerate(texture_files):
            try:
                # 加载纹理
                tex = Image.open(tex_file).convert('RGBA')
                
                # 调整大小（如果需要）
                if tex.size != (self.texture_size, self.texture_size):
                    print(f"  调整 {os.path.basename(tex_file)} 从 {tex.size} 到 {self.texture_size}x{self.texture_size}")
                    tex = tex.resize((self.texture_size, self.texture_size), Image.NEAREST)
                
                # 计算位置
                row = idx // textures_per_side
                col = idx % textures_per_side
                x = col * self.texture_size
                y = row * self.texture_size
                
                # 粘贴到图集
                atlas.paste(tex, (x, y))
                
                # 计算 UV 坐标（OpenGL 坐标系，左下角为原点）
                # 注意：这里 y 需要翻转
                uv_min_x = x / actual_atlas_size
                uv_min_y = (actual_atlas_size - y - self.texture_size) / actual_atlas_size
                uv_max_x = (x + self.texture_size) / actual_atlas_size
                uv_max_y = (actual_atlas_size - y) / actual_atlas_size
                
                # 记录元数据
                tex_name = os.path.splitext(os.path.basename(tex_file))[0]
                metadata['textures'][tex_name] = {
                    'index': idx,
                    'position': [x, y],
                    'uv': {
                        'min': [uv_min_x, uv_min_y],
                        'max': [uv_max_x, uv_max_y]
                    }
                }
                
                if idx < 5 or idx % 100 == 0:
                    print(f"  [{idx}] {tex_name}: ({x}, {y}) -> UV({uv_min_x:.3f}, {uv_min_y:.3f}, {uv_max_x:.3f}, {uv_max_y:.3f})")
                
            except Exception as e:
                print(f"  错误: 无法加载 {tex_file}: {e}")
        
        # 保存图集
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        atlas.save(output_path)
        print(f"\n✓ 图集已保存: {output_path}")
        
        # 保存元数据
        if metadata_path:
            with open(metadata_path, 'w') as f:
                json.dump(metadata, f, indent=2)
            print(f"✓ 元数据已保存: {metadata_path}")
        
        return metadata

def main():
    import glob
    
    # 配置
    TEXTURE_SIZE = 16
    ATLAS_SIZE = 1024  # 可以是 512, 1024, 2048, 4096 等
    INPUT_DIR = "resources/textures/raw_blocks"  # 存放所有 16x16 纹理的目录
    OUTPUT_ATLAS = "resources/textures/blocks/block_atlas.png"
    OUTPUT_METADATA = "resources/textures/blocks/block_atlas.json"
    
    # 查找所有纹理文件
    texture_files = []
    for ext in ['*.png', '*.jpg', '*.jpeg']:
        texture_files.extend(glob.glob(os.path.join(INPUT_DIR, ext)))
    
    if not texture_files:
        print(f"错误: 在 {INPUT_DIR} 中没有找到纹理文件")
        return 1
    
    # 排序（可选，保持一致性）
    texture_files.sort()
    
    print(f"找到 {len(texture_files)} 个纹理文件")
    print("=" * 60)
    
    # 创建图集
    builder = TextureAtlasBuilder(texture_size=TEXTURE_SIZE, target_atlas_size=ATLAS_SIZE)
    metadata = builder.create_atlas(texture_files, OUTPUT_ATLAS, OUTPUT_METADATA)
    
    print("=" * 60)
    print("完成!")
    
    # 显示一些统计信息
    print(f"\n可以容纳最多 {builder.max_textures} 个纹理")
    print(f"实际使用 {len(metadata['textures'])} 个纹理")
    print(f"剩余容量: {builder.max_textures - len(metadata['textures'])} 个纹理")
    
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())
    