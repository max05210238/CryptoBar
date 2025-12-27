#!/bin/bash
# 优化组装指南图片大小（压缩超过1MB的图片）

set -e

IMG_DIR="docs/images/assembly"
BACKUP_DIR="${IMG_DIR}/originals"
TARGET_SIZE_KB=1000  # 目标：小于1MB

echo "🔧 正在优化组装指南图片..."
echo ""

# 检查ImageMagick是否安装
if ! command -v convert &> /dev/null; then
    echo "❌ ImageMagick未安装"
    echo ""
    echo "请安装ImageMagick:"
    echo "  Ubuntu/Debian: sudo apt-get install imagemagick"
    echo "  RHEL/CentOS: sudo yum install ImageMagick"
    echo "  macOS: brew install imagemagick"
    exit 1
fi

# 创建备份目录
mkdir -p "$BACKUP_DIR"

OPTIMIZED=0
SKIPPED=0

# 遍历所有jpg文件
for img in "$IMG_DIR"/*.jpg; do
    [ -f "$img" ] || continue

    filename=$(basename "$img")
    size_kb=$(du -k "$img" | cut -f1)

    if [ $size_kb -gt $TARGET_SIZE_KB ]; then
        # 备份原图
        cp "$img" "$BACKUP_DIR/$filename"

        # 优化图片
        convert "$img" -resize "1200x>" -quality 80 -strip "$img"

        new_size_kb=$(du -k "$img" | cut -f1)
        reduction=$((100 - (new_size_kb * 100 / size_kb)))

        echo "✅ $filename: ${size_kb}KB → ${new_size_kb}KB (-${reduction}%)"
        ((OPTIMIZED++))
    else
        echo "⏭️  $filename: ${size_kb}KB (已经够小，跳过)"
        ((SKIPPED++))
    fi
done

echo ""
echo "======================================"
echo "🎉 优化完成！"
echo "  优化: $OPTIMIZED 张"
echo "  跳过: $SKIPPED 张"
echo "======================================"

if [ $OPTIMIZED -gt 0 ]; then
    echo ""
    echo "下一步:"
    echo "  git add docs/images/assembly/*.jpg"
    echo "  git commit -m 'Optimize large assembly guide images'"
    echo "  git push"
fi
