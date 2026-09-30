# Local Runtime and Updates Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a one-click local SMSTrader runtime, resilient registration-to-login behavior, configurable server URL, and a reproducible Windows installer pipeline capable of Velopack delta packages.

**Architecture:** The GUI reads validated application configuration and keeps authentication errors inside the login loop. A Windows launcher owns the local HTTP server lifecycle and readiness check. CMake stages the GUI and Qt runtime, while a separate PowerShell packaging script invokes pinned Velopack tooling.

**Tech Stack:** C++20, Qt 6 Widgets/Network/WebSockets, CMake/CTest, PowerShell 7-compatible Windows PowerShell syntax, Velopack `vpk`, GitHub Releases.

**Spec:** `docs/superpowers/specs/2026-09-30-local-runtime-and-updates-design.md`

## Global Constraints

- Production server URLs require HTTPS; plain HTTP is allowed only for loopback development.
- Passwords and JWTs are never persisted or logged.
- Installed clients do not launch a local server.
- PostgreSQL is not bundled into the desktop installer.
- Public installers require Authenticode signing; unsigned artifacts are development builds.

---

### Task 1: Configurable API endpoint

**Files:**
- Create: `src/gui/app_config.h`
- Create: `src/gui/app_config.cpp`
- Create: `tests/app_config_test.cpp`
- Create: `app_config.example.json`
- Modify: `src/gui/network_client.h`
- Modify: `src/gui/network_client.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `AppConfig loadAppConfig(const std::filesystem::path&)`, where `AppConfig::apiBaseUrl` is a normalized `std::string`.
- Consumes: `NetworkClient` constructor converts the configured URL to `QString` and uses it for HTTP and WebSocket endpoints.

- [ ] Write tests for missing file fallback, explicit URL, trailing-slash normalization, malformed JSON fallback, rejection of remote HTTP, and acceptance of loopback HTTP.
- [ ] Build and run `app_config_test`; confirm it fails because the API does not exist.
- [ ] Implement the smallest configuration reader that makes the tests pass.
- [ ] Inject the loaded base URL into `NetworkClient` and derive both HTTPS and WebSocket URLs from it.
- [ ] Run the focused tests and build `smstrader_gui`.
- [ ] Commit the task.

### Task 2: Resilient registration-to-login flow

**Files:**
- Create: `src/gui/auth_response.h`
- Create: `src/gui/auth_response.cpp`
- Create: `tests/auth_response_test.cpp`
- Modify: `src/gui/network_client.cpp`
- Modify: `src/gui/main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: pure helpers that convert HTTP status, transport error, and response JSON into either a JWT or a user-facing error.
- Consumes: `NetworkClient::registerUser` and `NetworkClient::login` use those helpers; `main` keeps the dialog loop alive on every expected failure and opens `MainWindow` after registration plus login succeeds.

- [ ] Write failing tests for network failure, malformed JSON, rejected credentials, missing JWT, valid registration response, and valid login token.
- [ ] Run the focused test and verify expected failures.
- [ ] Implement response parsing without touching network transport.
- [ ] Wire both synchronous requests to check transport errors and use the tested parsing functions.
- [ ] Add a GUI-flow regression test proving a registration action requires registration then login and success proceeds without reopening the dialog.
- [ ] Run focused tests and all CTest tests.
- [ ] Commit the task.

### Task 3: One-click local launcher

**Files:**
- Create: `scripts/start-local.ps1`
- Create: `start-local.bat`
- Create: `tests/start_local.Tests.ps1`
- Modify: `README.md`

**Interfaces:**
- Produces: `start-local.bat` as the double-click entry point; `scripts/start-local.ps1` accepts optional `-BuildConfiguration`, `-HealthTimeoutSeconds`, and `-NoGui` test controls.
- Consumes: `build/<configuration>/smstrader_http.exe`, `smstrader_gui.exe`, root `db_config.json`, `certs/server.crt`, and `certs/server.key`.

- [ ] Write a launcher test harness with temporary fake server/client executables covering missing prerequisites, already-healthy server reuse, early server exit, timeout, and successful client launch.
- [ ] Run it and verify failure because the launcher is absent.
- [ ] Implement prerequisite checks, server start, bounded health polling, logging, and GUI launch.
- [ ] Add the batch wrapper and concise README instructions.
- [ ] Run launcher tests and a real `-NoGui` health check when the local database is available.
- [ ] Commit the task.

### Task 4: Staged application and Velopack package

**Files:**
- Create: `scripts/package-windows.ps1`
- Create: `.config/dotnet-tools.json`
- Create: `packaging/README.md`
- Modify: `CMakeLists.txt`
- Modify: `.gitignore`
- Modify: `README.md`

**Interfaces:**
- Produces: `dist/app/` staged client and `dist/releases/` Velopack artifacts; packaging script accepts `-Version`, optional `-PreviousReleaseDir`, and optional `-PackOnly`.
- Consumes: Release `smstrader_gui`, Qt deployment output, `app_config.example.json`, and Velopack `vpk` pinned through the local .NET tool manifest.

- [ ] Write a packaging smoke test that invokes the script in validation mode and fails when required staged files or semantic version are missing.
- [ ] Run it and verify the expected missing-script failure.
- [ ] Add CMake install/deploy rules for the GUI and required runtime assets.
- [ ] Pin Velopack and implement clean staging plus `vpk pack` invocation.
- [ ] Build Release, stage the application, and produce an unsigned development installer.
- [ ] Verify artifact names, package metadata, and staged runtime files.
- [ ] Commit the task.

### Task 5: Release automation foundation

**Files:**
- Create: `.github/workflows/windows-release.yml`
- Create: `docs/releasing.md`
- Modify: `README.md`

**Interfaces:**
- Produces: a tag-triggered Windows workflow that builds, tests, packages, and uploads Velopack assets to a GitHub Release.
- Consumes: repository tag `vX.Y.Z`, packaging script, and optional signing secrets documented but not required for local development.

- [ ] Add workflow validation conditions for semantic tags and a Release build/test/package sequence.
- [ ] Document signing inputs, stable channel, first full release, and subsequent delta release procedure.
- [ ] Validate workflow YAML locally by parsing it and run the packaging script in validation mode.
- [ ] Run the complete CTest suite and build GUI/HTTP targets.
- [ ] Refresh Graft, inspect the final diff, and commit/push the completed feature.
