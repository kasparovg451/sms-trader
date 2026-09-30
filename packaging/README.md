# Windows packaging

The Windows package is produced with Velopack `vpk` 1.2.161, pinned in `.config/dotnet-tools.json`.

Create an unsigned development installer:

```powershell
.\scripts\package-windows.ps1 -Version 0.1.0
```

The script builds the Release GUI, stages it with its Qt runtime in `dist/app`, and writes Velopack artifacts to `dist/releases`.

Keep the previous release files in `dist/releases` before packaging a newer version. Velopack uses them to produce a delta package and falls back to the full package when a delta cannot be applied.

Unsigned packages are for local development. A public release must use Authenticode signing so Windows can identify the publisher and avoid unnecessary SmartScreen warnings.
