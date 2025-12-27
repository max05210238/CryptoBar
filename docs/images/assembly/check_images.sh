#!/bin/bash
# 检查组装指南所需的所有图片文件

echo "🔍 检查组装指南图片文件..."
echo ""

REQUIRED_IMAGES=(
    "step1_esp32_soldered.jpg"
    "step2_5v_out_pad.jpg"
    "step5_encoder_modified.jpg"
    "step5_encoder_mounting_pin.jpg"
    "step6_led_cut.jpg"
    "step7_led_arrow.jpg"
    "step8_led_on_lens.jpg"
    "step9_m2_insert.jpg"
    "step10_m3_insert.jpg"
    "step11_magnet.jpg"
    "step12_esp32_mounted.jpg"
    "step13_led_lens_installed.jpg"
    "step14_display_installed.jpg"
    "step15_encoder_installed.jpg"
    "step17_wiring_complete.jpg"
    "step19_finished.jpg"
)

MISSING=0
FOUND=0

for img in "${REQUIRED_IMAGES[@]}"; do
    if [ -f "docs/images/assembly/$img" ]; then
        SIZE=$(ls -lh "docs/images/assembly/$img" | awk '{print $5}')
        echo "✅ $img ($SIZE)"
        ((FOUND++))
    else
        echo "❌ $img (缺失)"
        ((MISSING++))
    fi
done

echo ""
echo "======================================"
echo "总计: ${#REQUIRED_IMAGES[@]} 张图片"
echo "找到: $FOUND 张"
echo "缺失: $MISSING 张"
echo "======================================"

if [ $MISSING -eq 0 ]; then
    echo ""
    echo "🎉 所有图片都已就位！"
    echo ""
    echo "下一步："
    echo "  git add docs/images/assembly/*.jpg"
    echo "  git commit -m 'Add assembly guide images'"
    echo "  git push"
else
    echo ""
    echo "⚠️  还有 $MISSING 张图片缺失"
    echo ""
    echo "请将图片文件放在："
    echo "  $(pwd)/docs/images/assembly/"
    echo ""
    echo "需要的文件名："
    for img in "${REQUIRED_IMAGES[@]}"; do
        if [ ! -f "docs/images/assembly/$img" ]; then
            echo "  - $img"
        fi
    done
fi
