#!/bin/bash
# 从 CryptoBar組裝教學.pdf 提取图片到 docs/images/assembly/
# 使用 pdftoppm (来自 poppler-utils 包)

set -e

PDF_PATH="docs/temp/CryptoBar組裝教學.pdf"
OUTPUT_DIR="docs/images/assembly"
TEMP_DIR="/tmp/cryptobar_extract"

echo "🔄 正在从PDF提取组装指南图片..."
echo ""

# 检查PDF是否存在
if [ ! -f "$PDF_PATH" ]; then
    echo "❌ 找不到PDF文件: $PDF_PATH"
    exit 1
fi

# 检查pdftoppm是否安装
if ! command -v pdftoppm &> /dev/null; then
    echo "❌ pdftoppm 未安装"
    echo ""
    echo "请安装 poppler-utils:"
    echo "  Ubuntu/Debian: sudo apt-get install poppler-utils"
    echo "  RHEL/CentOS: sudo yum install poppler-utils"
    echo "  macOS: brew install poppler"
    exit 1
fi

# 创建临时目录
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"
mkdir -p "$OUTPUT_DIR"

echo "⏳ 正在转换PDF为图片（DPI=150）..."
pdftoppm -jpeg -r 150 "$PDF_PATH" "$TEMP_DIR/page"

# 检查生成的文件
PAGE_COUNT=$(ls "$TEMP_DIR"/page-*.jpg 2>/dev/null | wc -l)
echo "✅ 成功转换 $PAGE_COUNT 页"
echo ""

# PDF页面到目标文件名的映射
declare -A PAGE_MAP=(
    ["01"]="step1_esp32_soldered.jpg"
    ["02"]="step2_5v_out_pad.jpg"
    ["03"]="step5_encoder_modified.jpg"
    ["04"]="step5_encoder_mounting_pin.jpg"
    ["05"]="step6_led_cut.jpg"
    ["06"]="step7_led_arrow.jpg"
    ["07"]="step8_led_on_lens.jpg"
    ["08"]="step9_m2_insert.jpg"
    ["09"]="step10_m3_insert.jpg"
    ["10"]="step11_magnet.jpg"
    ["11"]="step12_esp32_mounted.jpg"
    ["12"]="step13_led_lens_installed.jpg"
    ["13"]="step14_display_installed.jpg"
    ["14"]="step15_encoder_installed.jpg"
    ["16"]="step17_wiring_complete.jpg"
    ["17"]="step19_finished.jpg"
)

# 复制并重命名文件
SAVED=0
for page in "${!PAGE_MAP[@]}"; do
    src="$TEMP_DIR/page-$page.jpg"
    dst="$OUTPUT_DIR/${PAGE_MAP[$page]}"

    if [ -f "$src" ]; then
        cp "$src" "$dst"
        # 优化图片大小
        if command -v convert &> /dev/null; then
            convert "$dst" -resize "1200x>" -quality 85 "$dst"
        fi
        SIZE=$(ls -lh "$dst" | awk '{print $5}')
        echo "✅ 页面 $page → ${PAGE_MAP[$page]} ($SIZE)"
        ((SAVED++))
    else
        echo "⚠️  页面 $page 不存在，跳过 ${PAGE_MAP[$page]}"
    fi
done

# 清理临时文件
rm -rf "$TEMP_DIR"

echo ""
echo "======================================"
echo "🎉 完成！成功提取 $SAVED/${#PAGE_MAP[@]} 张图片"
echo "======================================"
echo ""
echo "下一步:"
echo "  1. 检查图片: bash docs/images/assembly/check_images.sh"
echo "  2. 提交到git:"
echo "     git add docs/images/assembly/*.jpg"
echo "     git commit -m 'Add assembly guide images from PDF'"
echo "     git push"
