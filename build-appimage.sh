#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ── 1. Compilar em Release ─────────────────────────────────────────────────
echo "==> Compilando..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# ── 2. Baixar ferramentas se necessário ────────────────────────────────────
if [ ! -f linuxdeploy-x86_64.AppImage ]; then
    echo "==> Baixando linuxdeploy..."
    wget -q --show-progress \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

if [ ! -f linuxdeploy-plugin-qt-x86_64.AppImage ]; then
    echo "==> Baixando linuxdeploy-plugin-qt..."
    wget -q --show-progress \
        https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi

# ── 3. Extrair ambos os AppImages e corrigir strip ─────────────────────────
# O strip bundled é muito antigo para .relr.dyn (Arch/Manjaro binutils moderno).
# Cada AppImage tem strip próprio — precisamos corrigir os dois.

if [ ! -d squashfs-root-linuxdeploy ]; then
    echo "==> Extraindo linuxdeploy..."
    ./linuxdeploy-x86_64.AppImage --appimage-extract > /dev/null 2>&1
    mv squashfs-root squashfs-root-linuxdeploy
fi
yes | cp -f /usr/bin/strip squashfs-root-linuxdeploy/usr/bin/strip

if [ ! -d squashfs-root-qt ]; then
    echo "==> Extraindo linuxdeploy-plugin-qt..."
    ./linuxdeploy-plugin-qt-x86_64.AppImage --appimage-extract > /dev/null 2>&1
    mv squashfs-root squashfs-root-qt
fi
yes | cp -f /usr/bin/strip squashfs-root-qt/usr/bin/strip

# Wrapper que aponta para o plugin extraído (linuxdeploy busca "linuxdeploy-plugin-qt-x86_64" no PATH)
cat > /tmp/linuxdeploy-plugin-qt-x86_64 << EOF
#!/bin/bash
exec "$SCRIPT_DIR/squashfs-root-qt/AppRun" "\$@"
EOF
chmod +x /tmp/linuxdeploy-plugin-qt-x86_64

# ── 4. Verificar dependência jxrlib ───────────────────────────────────────
if ! ldconfig -p | grep -q libjxrglue; then
    echo ""
    echo "ERRO: libjxrglue não encontrada."
    echo "Instale com:  sudo pacman -S jxrlib"
    echo ""
    exit 1
fi

# ── 5. Gerar AppImage ──────────────────────────────────────────────────────
echo "==> Gerando AppImage..."
rm -rf AppDir

PATH="/tmp:$PATH" \
QMAKE=/usr/bin/qmake6 \
EXTRA_QT_MODULES=wayland \
"$SCRIPT_DIR/squashfs-root-linuxdeploy/AppRun" \
    --appdir AppDir \
    --executable build/stickies \
    --desktop-file stickies.desktop \
    --icon-file stickies.svg \
    --plugin qt \
    --output appimage

echo ""
echo "==> Pronto! AppImage gerado:"
ls -lh Stickies*.AppImage 2>/dev/null || ls -lh stickies*.AppImage 2>/dev/null
