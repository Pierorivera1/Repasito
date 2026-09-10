# Omacalendar

A dead-simple spaced-repetition review agenda built with Qt Quick and C++ that automatically follows the Omarchy theme and system dark/light mode.

## Spaced Repetition Cadence

When you enter a topic (e.g. `Module 2 of Docker`), Omacalendar automatically schedules 3 review checkpoints:
- **R1 (+1 day)**: Review concepts 1 day after learning.
- **R2 (+3 days)**: Reinforce recall 3 days after learning.
- **R3 (+5 days)**: Lock into memory 5 days after learning.

Checking off the final review marks the topic completed and archives it.

## Features

- **Overdue Rollover**: Any missed reviews automatically roll into "Today" with an Overdue pill badge, preventing forgotten backlog.
- **Mini-Calendar Strip**: 7-day overview with review indicators for each day.
- **Markdown Notes**: Attach checklists, links, and notes to each topic.
- **Keyboard-First**:
  - `N` or `Ctrl+N`: Add new topic
  - `/`: Focus search filter
  - `J` / `K` or `Down` / `Up`: Navigate reviews
  - `Space`: Check off / toggle completion
  - `Esc`: Clear search / close modal
- **Theme Following**: Live tinting and dark/light switching from `~/.local/state/omarchy/current/theme/colors.toml`.
- **Zero Daemons**: No background services or spammy notifications. Launches instantly.

## Install

Build and install locally on Arch Linux / Omarchy:

```bash
./bin/install
```

Or compile manually:

```bash
./bin/build
./build/omacalendar
```

## Requirements

- Qt 6: `qt6-base`, `qt6-declarative`, `sqlite`
- `xdg-desktop-portal` and a portal backend

The iA Writer Mono font is bundled under the SIL Open Font License 1.1; see `fonts/OFL.txt`.

## License

MIT. See `LICENSE`.
