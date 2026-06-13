# Stickies

[Português](README.pt-BR.md) | **English**

![License](https://img.shields.io/github/license/povman/stickies)
![Release](https://img.shields.io/github/v/release/povman/stickies)
![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![C++](https://img.shields.io/badge/C++-17-blue)
![Wayland](https://img.shields.io/badge/Wayland-native-blueviolet)

Floating sticky notes for Linux, built with **Qt6/C++** with native **Wayland** support.

## Features

- Multiple simultaneous notes, always visible above other windows
- Rich text formatting: bold, italic, underline, strikethrough, font color and size
- **PT-BR spell checking** with right-click suggestions — dictionary bundled, no extra install needed
- Customizable note colors (yellow, blue, green, red, white, transparent)
- Drag and resize from any edge or corner
- Position, size and content automatically saved and restored
- **Auto-backup**: saves via `.tmp` → `.bak` → `.json`, protecting against data loss on crash
- Export notes to **HTML** or **Markdown**
- System tray icon with action menu
- Keyboard shortcuts: `Ctrl+N` new note, `Ctrl+Shift+S` save all, `Ctrl+Shift+W` hide all

## Download

Download the AppImage from the [Releases](https://github.com/povman/stickies/releases) page — no installation required.

```bash
chmod +x Stickies-x86_64.AppImage
./Stickies-x86_64.AppImage
```

## Build from source

**Dependencies:**

```bash
# Arch / Manjaro
sudo pacman -S qt6-base cmake hunspell
```

**Build:**

```bash
git clone https://github.com/povman/stickies.git
cd stickies
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/stickies
```

**Generate AppImage:**

```bash
bash build-appimage.sh
```

## Data files

Notes are saved at:

```
~/.config/stickies/notes.json
~/.config/stickies/notes.json.bak  ← automatic backup
```

## License

MIT © [Fabio Moraes](https://fabiomoraes.sisdigital.com.br)
