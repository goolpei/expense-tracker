# Project Instructions: C++ Windows Expense Tracker

## 1. Token & Context Boundaries (Strict)
- **NEVER** read, index, search, or modify files inside `/vendor/` or `/build/`.
- Only read the specific files inside `/src/` relevant to the current task.
- Keep responses and edits focused on the requested module; do not rewrite unrelated files.

## 2. Tech Stack & API Conventions
- **Language:** C++20 (`std::vector`, `std::array`, `std::unique_ptr`, `std::optional`, `std::string_view`).
- **Platform & Rendering:** Windows Win32 + DirectX 11, Dear ImGui (Tables enabled), and **ImPlot v1.0+**.
- **ImPlot API Rule:** Use the modern `ImPlotSpec` struct API (`ImPlot::PlotLine("Label", x, y, count, spec)`). Do **NOT** use deprecated `ImPlot::SetNextLineStyle` or `ImPlot::SetNextFillStyle` calls.
- **Database:** SQLite3 C API (`sqlite3*`, `sqlite3_stmt*`) wrapped in RAII inside `src/data/database.cpp`. Always use parameterized queries (`sqlite3_prepare_v2` + `sqlite3_bind_*`).

## 3. Architecture & Separation of Concerns
Follow the strict 4-layer dependency direction (`ui` -> `analytics` -> `data` -> `core`):
- `src/core/`: Plain structs (`models.hpp`), centralized `AppState` (`app_state.hpp`), and formatting utilities. **Must NOT include `imgui.h`, `implot.h`, or `sqlite3.h`.**
- `src/data/`: Owns all SQLite queries and schema migrations.
- `src/analytics/`: Pure C++ statistical aggregation (`StatsEngine`). Outputs contiguous `std::vector<double>` buffers ready for ImPlot. **Must NOT include `imgui.h` or `implot.h`.**
- `src/ui/`: Stateless ImGui/ImPlot rendering functions. Views read `DashboardStats` and mutate `AppState`. When a view modifies data or filters, set `state.stats_dirty = true` so `AppShell` refreshes `StatsEngine` once per change.

## 4. Financial & Data Integrity Rules
- Store all monetary values as **`int64_t` cents** (e.g., `1250` = `$12.50`) in C++ structs and SQLite `INTEGER` columns. Convert to `double` only inside `StatsEngine` when populating chart arrays or formatting display strings.
- Store dates as ISO-8601 strings (`YYYY-MM-DD`) so SQLite sorting and range filtering work lexicographically.

## 5. UI Design System ("Obsidian Slate")
- Maintain a dark fintech aesthetic: deep charcoal canvas (`#0F1117`), elevated cards (`#161B26`) with `1px` borders (`#262E3D`), crisp primary text (`#F1F5F9`), muted labels (`#94A3B8`), emerald green (`#10B981`) for positive/under-budget states, and coral red (`#F43F5E`) for overspending/anomalies.
- Reuse shared widgets from `src/ui/components.hpp` (`KpiCard`, `CategoryPill`, `SectionHeader`) instead of duplicating styling boilerplate across views.
- When adding a new feature or screen, create a new dedicated file pair inside `src/ui/views/` and register it in `src/ui/app_shell.cpp`.