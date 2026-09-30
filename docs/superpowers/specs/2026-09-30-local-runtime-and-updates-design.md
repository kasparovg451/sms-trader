# Local Runtime and Updates Design

## Goal

SMSTrader must be easy to run on the developer's Windows computer, must keep the application open when registration or networking fails, must enter the chat immediately after successful registration, and must be packageable as an installable desktop application with future delta updates.

## Product modes

SMSTrader has two explicit modes:

1. **Local development mode.** A single launcher starts the existing HTTPS API server, waits until `/health` reports readiness, and then starts the GUI. The server is started with the repository root as its working directory so `db_config.json` and `certs/` resolve consistently.
2. **Installed client mode.** The Windows installer contains only the GUI and its Qt runtime. It connects to the configured central server. A production client must never start a private server on every user's computer.

The first deliverable makes local mode one-click and creates the installed-client packaging pipeline. Deployment of the central server to a VPS remains a separate operational step because it requires server credentials, DNS, and a trusted TLS certificate.

## Configuration

The GUI reads `app_config.json` from the application working directory. Its supported field is:

```json
{
  "api_base_url": "https://localhost:8080"
}
```

If the file is absent, malformed, or contains an unsupported URL, the GUI uses `https://localhost:8080`. The URL is normalized by removing trailing slashes. HTTP is accepted only for loopback development addresses; non-local servers require HTTPS.

The configuration reader is a small C++ unit independent of Qt networking so its behavior can be tested without a running server.

## Registration and login

The existing GUI already calls `/register` and then `/login`, but network failures are not distinguished from malformed server responses. Both synchronous authentication requests will:

- check `QNetworkReply::error()` before parsing JSON;
- return a readable Russian error while keeping the login dialog loop alive;
- accept only a non-empty JWT from `/login`;
- continue directly to the chat after a successful registration followed by login.

No password or token is written to logs or configuration files.

## Local launcher

`start-local.ps1` owns the local process lifecycle:

- locate Debug or Release binaries under `build/`;
- refuse to start with a clear message if `db_config.json`, certificates, or binaries are missing;
- reuse a healthy server already listening on the configured endpoint;
- otherwise start `smstrader_http.exe` in the repository root and write its output to `logs/local-server.log`;
- poll `/health` for a bounded period;
- start `smstrader_gui.exe` only after the server is ready;
- if the server it started exits early or never becomes healthy, show the log path and return a non-zero exit code.

`start-local.bat` is the double-click entry point and delegates to the PowerShell script without requiring the user to change execution policy globally.

The launcher does not install or initialize PostgreSQL. A missing or unhealthy database is reported as a startup failure rather than causing the GUI to appear broken.

## Packaging and updates

The installed client is assembled into `dist/app/` by CMake install rules plus Qt deployment. Mutable configuration and logs are kept outside Velopack's versioned `current` directory.

[Velopack](https://docs.velopack.io/getting-started/cpp) is used because it supports C++ applications, one-click Windows installers, GitHub Releases, and binary delta packages. The first packaging scripts will:

- install/use the pinned `vpk` .NET tool;
- package the staged GUI as application id `SMSTrader`;
- produce a development `Setup.exe`, portable ZIP, full package, and release metadata;
- generate delta packages when a previous release is supplied;
- leave signing optional for local development and document that public releases require Authenticode signing.

The GUI-side automatic update prompt is a later slice built on the same Velopack package identity. This delivery establishes a reproducible installer and delta-capable release artifacts without silently downloading or executing remote code during normal development.

## Testing and verification

- Unit tests cover configuration defaulting, normalization, validation, and explicit values.
- Authentication response parsing is tested independently of the network transport.
- Launcher behavior is exercised against controlled temporary executables or dry-run inputs; tests assert exit codes and process decisions rather than source text.
- Existing 33 CTest tests remain green.
- Debug GUI and HTTP server targets build successfully.
- The local launcher reaches `/health` and opens the GUI when PostgreSQL and configuration are available; otherwise its diagnostic output identifies the external blocker.
- Packaging is verified by producing local artifacts and inspecting their contents. Publishing and code signing are reported as externally blocked until credentials are supplied.

## Non-goals

- Bundling PostgreSQL into the desktop installer.
- Running the production server on each user's PC.
- Storing passwords or JWTs on disk.
- Implementing a custom update protocol.
- Publishing an unsigned installer as a trusted public production release.
