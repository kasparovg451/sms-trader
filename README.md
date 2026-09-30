# SMSTrader

A C++20 and Qt 6 messenger project built as a learning journey toward a real client/server application.

## Run locally on Windows

Before the first run:

1. Copy `db_config.example.json` to `db_config.json` and set your PostgreSQL connection string and JWT secret.
2. Ensure `certs/server.crt` and `certs/server.key` exist.
3. Build `smstrader_http` and `smstrader_gui` with the `default` CMake preset.

Then double-click `start-local.bat`. It starts the HTTPS API server, waits for `/health`, and opens the Qt client only when the server and database are ready.

From a terminal you can validate the server without opening the GUI:

```powershell
.\scripts\start-local.ps1 -NoGui
```

The default client endpoint is `https://localhost:8080`. To use a different server, copy `app_config.example.json` to `app_config.json` and change `api_base_url`. Remote servers must use HTTPS.
