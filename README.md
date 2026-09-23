<div align="center">

# GhostFrame 👻
### Hardware-Level Screen Capture Cloaker for OBS, Discord & Windows
**Created by Sribalan (aka sriyogod)**

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-blue.svg)](#system-requirements)
[![UI](https://img.shields.io/badge/UI-DirectX%2011%20%2B%20Dear%20ImGui-1DB954.svg)](#interface)
[![License](https://img.shields.io/badge/License-PolyForm%20Noncommercial-green.svg)](LICENSE)

*Keep sensitive apps, private chats, passwords, and background windows completely invisible on OBS, Discord, and Zoom screen shares — while you can still see and interact with them normally on your monitor.*

---

</div>

## 💡 Why I Built GhostFrame

If you've ever streamed on Discord, presented in Zoom/Teams, or broadcasted via OBS, you've probably faced the classic dilemma:

1. **Window Capture** is clunky, doesn't capture popups or dropdown menus, and breaks when apps minimize.
2. **Display Capture** shows everything on your desktop — including your private Discord DMs, WhatsApp messages, Telegram alerts, sensitive code, and passwords.

I got tired of constantly worrying about accidental leaks during live coding and gaming sessions. I wanted a lightweight Windows utility that works directly at the **Windows Desktop Window Manager (DWM)** hardware compositor level.

With **GhostFrame**, any application you mark as cloaked simply disappears from all screen capture streams in real time. To your stream viewers or screen recording, the window does not exist — they only see your wallpaper or whatever is behind it. But on your physical monitor, the window is right in front of you, fully visible, clickable, and usable.

---

## 🤖 AI Collaboration Disclosure

In the spirit of complete engineering transparency:
- **~70% of the project** was architected, tested, and fine-tuned by me (**Sribalan / sriyogod**), including the cross-process injection architecture, Focus/Solo Mode logic, Spotify-inspired UI styling, and Windows shell filtering.
- **~30% AI assistance** (using Google Antigravity & Gemini) was leveraged for Win32 API research, low-level DWM edge-case documentation, and rapid library boilerplate.

---

## ✨ Features

### 1. 🥷 Hardware-Level Cloaking
- Uses the native Windows DWM display affinity flag (`WDA_EXCLUDEFROMCAPTURE = 0x00000011`).
- Hardware accelerated via the GPU compositor. Zero dropped frames, zero streaming lag.
- Protects against:
  - **OBS Studio:** Display Capture, Game Capture, and Window Capture.
  - **Discord:** Go Live screen share (full screen or app stream).
  - **Zoom / Microsoft Teams / Google Meet:** Any desktop presentation.
  - **Snipping Tool & Game Bar:** `Win + Shift + S` or background screenshots.

### 2. 🎯 Focus Mode ("Solo Streaming")
The inverse of cloaking individual apps. When you're ready to stream a game, IDE, or tutorial:
- Click **Solo** next to your target app.
- GhostFrame keeps **only that app visible** on your stream and automatically cloaks all other desktop windows!
- Click **Exit Solo** when you're done to restore all windows back to normal.

### 3. 🎨 Spotify-Inspired Modern Dark UI
- Built with custom **DirectX 11 + Dear ImGui**.
- Dark obsidian canvas (`#121212`), pitch-black sidebar (`#000000`), Spotify Green (`#1DB954`) badges, and crisp Segoe UI typography.
- Real-time window search and instant one-click toggle buttons.

### 4. ⚡ "Ghost The Ghost" (Self-Cloaking)
- GhostFrame has a dedicated **Hide This App** switch on the sidebar so you can manage your cloaked apps live during your stream without viewers ever seeing the GhostFrame dashboard itself.

### 5. 🛡️ Shell & Taskbar Protection
- Strict built-in process and class filters ensure `explorer.exe` (Windows Taskbar, Start Menu, Desktop) and system shell hosts are never touched, keeping your desktop stable and responsive.

### 6. 🔄 Auto-Hide Rules
- Set persistent rules for apps like `discord.exe`, `whatsapp.exe`, `telegram.exe`, or `chrome.exe` so they are automatically hidden the moment they launch.

---

## 🖥️ System Requirements

- **Operating System:** Windows 10 (Version 2004 / Build 19041 or higher) or Windows 11 (all versions).
- **Architecture:** 64-bit (x64).
- **Privileges:** Standard user privileges (Administrator only needed if you want to cloak elevated administrative apps).

---

## 🚀 Getting Started

### Option A: Using the Pre-Built Package
1. Head over to the [Releases](https://github.com) section.
2. Download `GhostFrame-v1.0-win64.zip` (portable) or run `GhostFrame_Setup_v1.0.exe` (installer).
3. Launch `ghostframe-gui.exe`.

### Option B: Building from Source
Building GhostFrame requires **MSVC (Visual Studio 2022 / 2026 Build Tools)** and **CMake**.

1. Clone this repository:
   ```bash
   git clone https://github.com/your-username/ghostframe.git
   cd ghostframe
   ```

2. Run the automated PowerShell build script:
   ```powershell
   .\build.ps1
   ```
   *(This script automatically fetches Dear ImGui, initializes the MSVC x64 environment, configures CMake, and builds the Release binaries.)*

3. Launch GhostFrame:
   ```powershell
   .\build\bin\Release\ghostframe-gui.exe
   ```

4. *(Optional)* To generate the portable zip package and Inno Setup installer:
   ```powershell
   .\package.ps1
   ```

---

## 🤝 Contributing & Collaboration

I built GhostFrame to solve a real problem for content creators, developers, and privacy-conscious users. I welcome contributions from the community!

- **Found a bug?** Please open an [Issue](../../issues) with your Windows version, OBS/Discord version, and steps to reproduce.
- **Have an idea?** Submit a [Feature Request](../../issues) or start a discussion.
- **Want to contribute code?**
  1. Fork the repo.
  2. Create a new branch (`git checkout -b feature/awesome-feature`).
  3. Commit your changes (`git commit -m "Add awesome feature"`).
  4. Push to the branch (`git push origin feature/awesome-feature`).
  5. Open a **Pull Request**.

---

## 📜 License

GhostFrame is released under the **[PolyForm Noncommercial License 1.0.0](LICENSE)**.

- **Free to Use:** Completely free for streamers, gamers, developers, students, and personal use.
- **Collaboration:** You are free to fork, study, modify, and submit pull requests.
- **Non-Commercial Restriction:** You may **NOT** sell this software, repackage it into paid commercial products, or monetize it without prior written permission from **Sribalan (sriyogod)**.

---

<div align="center">
  <b>Built with ❤️ by Sribalan (aka sriyogod)</b><br>
  <i>Empowering creators with real desktop privacy.</i>
</div>
