#!/usr/bin/env python3
"""
从 CryptoBar組裝教學.pdf 提取图片到 docs/images/assembly/
需要安装: pip install pdf2image pillow
"""

import os
import sys

# 检查依赖
try:
    from pdf2image import convert_from_path
    from PIL import Image
except ImportError:
    print("❌ 缺少依赖库")
    print("\n请运行:")
    print("  pip install pdf2image pillow")
    print("\n还需要安装 poppler-utils:")
    print("  Ubuntu/Debian: sudo apt-get install poppler-utils")
    print("  macOS: brew install poppler")
    sys.exit(1)

# PDF路径和输出目录
PDF_PATH = 'docs/temp/CryptoBar組裝教學.pdf'
OUTPUT_DIR = 'docs/images/assembly'

# PDF页面到图片文件名的映射
PAGE_MAPPING = {
    1: 'step1_esp32_soldered.jpg',
    2: 'step2_5v_out_pad.jpg',
    3: 'step5_encoder_modified.jpg',
    4: 'step5_encoder_mounting_pin.jpg',
    5: 'step6_led_cut.jpg',
    6: 'step7_led_arrow.jpg',
    7: 'step8_led_on_lens.jpg',
    8: 'step9_m2_insert.jpg',
    9: 'step10_m3_insert.jpg',
    10: 'step11_magnet.jpg',
    11: 'step12_esp32_mounted.jpg',
    12: 'step13_led_lens_installed.jpg',
    13: 'step14_display_installed.jpg',
    14: 'step15_encoder_installed.jpg',
    16: 'step17_wiring_complete.jpg',
    17: 'step19_finished.jpg',
}

def main():
    print("🔄 正在从PDF提取图片...")
    print(f"PDF文件: {PDF_PATH}")
    print(f"输出目录: {OUTPUT_DIR}")
    print()

    # 检查PDF是否存在
    if not os.path.exists(PDF_PATH):
        print(f"❌ 找不到PDF文件: {PDF_PATH}")
        return

    # 确保输出目录存在
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    try:
        # 转换PDF为图片（DPI=150，生成中等质量的图片）
        print("⏳ 正在转换PDF页面为图片（这可能需要1-2分钟）...")
        images = convert_from_path(PDF_PATH, dpi=150)
        print(f"✅ 成功转换 {len(images)} 页")
        print()

        # 保存指定页面
        saved_count = 0
        for page_num, filename in PAGE_MAPPING.items():
            if page_num <= len(images):
                output_path = os.path.join(OUTPUT_DIR, filename)

                # 获取该页的图片
                img = images[page_num - 1]  # 页码从1开始，索引从0开始

                # 调整大小（如果宽度超过1200px）
                if img.width > 1200:
                    ratio = 1200 / img.width
                    new_height = int(img.height * ratio)
                    img = img.resize((1200, new_height), Image.Resampling.LANCZOS)

                # 保存为JPEG（质量85）
                img.save(output_path, 'JPEG', quality=85, optimize=True)

                # 获取文件大小
                size_kb = os.path.getsize(output_path) / 1024
                print(f"✅ 页面 {page_num:2d} → {filename} ({size_kb:.1f} KB)")
                saved_count += 1
            else:
                print(f"⚠️  页面 {page_num} 不存在（PDF只有{len(images)}页）")

        print()
        print("=" * 50)
        print(f"🎉 完成！成功提取 {saved_count}/{len(PAGE_MAPPING)} 张图片")
        print()
        print("下一步:")
        print("  1. 运行: bash docs/images/assembly/check_images.sh")
        print("  2. 如果检查通过，运行:")
        print("     git add docs/images/assembly/*.jpg")
        print("     git commit -m 'Add assembly guide images from PDF'")
        print("     git push")

    except Exception as e:
        print(f"❌ 错误: {e}")
        print("\n可能的原因:")
        print("  1. 没有安装 poppler-utils")
        print("  2. PDF文件损坏")
        print("  3. 内存不足")

if __name__ == '__main__':
    main()
