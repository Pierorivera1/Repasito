# Omacalendar

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Qt 6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![SQLite](https://img.shields.io/badge/SQLite-3-003B57.svg)](https://www.sqlite.org/)
[![Arch Linux](https://img.shields.io/badge/Platform-Arch%20Linux-1793D1.svg)](https://archlinux.org/)

A dead-simple, distraction-free spaced-repetition review agenda built with **Qt 6 Quick** and **C++**. Designed for technical learners and developers who want a fast, keyboard-first review workflow without heavy web frameworks, account logins, or background daemons.

Omacalendar automatically synchronizes with your system's dark/light appearance and follows your active **Omarchy** desktop theme.

---

## The Spaced Repetition Cadence

Whenever you study a new module, chapter, or topic, Omacalendar automatically schedules three spaced checkpoints to lock knowledge into long-term memory:

```text
 Study Day (Day 0)
    │
    ├──► R1 (+1 Day): Initial recall & concept verification
    │
    ├──► R2 (+3 Days): Reinforce recall & fill knowledge gaps
    │
    └──► R3 (+5 Days): Retention verification & topic completion
```

Checking off the final **R3** review completes the topic and archives it.

---

## Key Features

- **Automated Review Cadence**: Enter a topic once; Omacalendar calculates and tracks R1, R2, and R3 review dates automatically.
- **Overdue Rollover**: Any missed review dates automatically roll into today's agenda with an amber `Overdue` pill, ensuring backlogs are never forgotten.
- **7-Day Mini-Calendar Strip**: Instant weekly overview displaying daily review counts. Click any day to isolate its schedule, or view your full chronological agenda.
- **Decoupled Search & Filtering**: Type `/` to search across topic titles and notes independently from your selected calendar day.
- **Markdown Notes & Checklists**: Attach markdown checklists (`- [ ]`), links, and key takeaways directly to each topic.
- **Keyboard-First Ergonomics**: Full Vim-inspired and standard keyboard control for rapid task management.
- **Omarchy Theme Synchronization**: Watches `~/.local/state/omarchy/current/theme/colors.toml` and system dark/light mode via `xdg-desktop-portal` DBus signals for instantaneous, live color changes.
- **Zero Daemons & Ultra-Lightweight**: No background processes, telemetries, or notification bloat. Launches in milliseconds from a clean local SQLite database.

---

## Keyboard Shortcuts

| Shortcut | Scope | Action |
|:---|:---|:---|
| <kbd>N</kbd> or <kbd>Ctrl</kbd>+<kbd>N</kbd> | Global | Open Add Topic dialog |
| <kbd>/</kbd> | Global | Focus and select search bar |
| <kbd>Esc</kbd> | Global | Clear search bar / deselect date filter / close modal |
| <kbd>J</kbd> or <kbd>Down</kbd> | Agenda | Select next review card |
| <kbd>K</kbd> or <kbd>Up</kbd> | Agenda | Select previous review card |
| <kbd>H</kbd> | Agenda | Navigate to previous day in day strip |
| <kbd>L</kbd> | Agenda | Navigate to next day in day strip |
| <kbd>Space</kbd> | Agenda | Check off / toggle completion for selected review |
| <kbd>D</kbd> or <kbd>Delete</kbd> | Agenda | Delete selected topic (with confirmation dialog) |
| <kbd>Tab</kbd> | Add Topic Modal | Jump directly from topic title to markdown notes |
| <kbd>Return</kbd> | Add Topic Modal | Schedule topic review (when in notes or title) |
| <kbd>Shift</kbd>+<kbd>Return</kbd> | Add Topic Modal | Insert newline in markdown notes |

---

## Installation & Build

### Requirements

Ensure you have the required Qt 6 development packages installed:

- **Arch Linux / Omarchy**:
  ```bash
  sudo pacman -S base-devel qt6-base qt6-declarative sqlite xdg-desktop-portal
  ```

The bundled **iA Writer Mono** font is licensed under the SIL Open Font License 1.1 (see `fonts/OFL.txt`).

---

### Quick Install (Arch Linux / Omarchy)

To build and install the binary, desktop entry, and application icon into your user environment (`~/.local`):

```bash
./bin/install
```

To build a standalone Arch Linux package with `makepkg`:

```bash
cd pkgbuild
makepkg -si
```

---

### Manual Compilation

Compile with `qmake6`:

```bash
./bin/build
```

Or run step-by-step:

```bash
mkdir -p build
cd build
qmake6 ../omacalendar.pro
make -j$(nproc)
./omacalendar
```

---

### Running the Test Suite

Omacalendar includes an automated test suite verifying SQLite operations, spaced repetition cadences, search/filter decoupling, and UI shortcuts:

```bash
./bin/test
```

---

## Architecture & Storage

```text
omacalendar/
├── bin/                 # Build, test, and install helper scripts
├── fonts/               # Bundled iA Writer Mono font assets
├── pkgbuild/            # Arch Linux PKGBUILD, desktop file, SVG icon
├── src/
│   ├── main.cpp         # Application entrypoint & engine initialization
│   ├── backend.h/.cpp   # Core QML/C++ bridge, state, and business logic
│   ├── database.h/.cpp  # SQLite persistence engine & cadence scheduler
│   ├── systemtheme.h/.cpp # Omarchy TOML & XDG portal theme listener
│   ├── resources.qrc    # Embedded QML components & icon resources
│   ├── Main.qml         # Main application window & keyboard navigation
│   ├── DayStrip.qml     # 7-day visual overview strip
│   ├── ReviewCard.qml   # Review item card with status pills & markdown notes
│   ├── AddTopicModal.qml # Topic scheduling modal
│   └── EditNotesModal.qml# In-place notes editor modal
└── tests/               # QtTest unit & integration test suite
```

### Data Storage

- **Database**: SQLite database stored locally at:
  ```text
  ~/.local/share/omacalendar/omacalendar.db
  ```
- **Geometry & Window State**: Automatically restored via `QSettings` at `~/.config/omacalendar/omacalendar.conf`.
- **Theme Source**: Monitored via file watcher at:
  ```text
  ~/.local/state/omarchy/current/theme/colors.toml
  ```

---

## License

This project is open source and available under the [MIT License](LICENSE).
Font files in `fonts/` are licensed under the SIL Open Font License 1.1 (`fonts/OFL.txt`).
