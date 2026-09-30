# Releasing SMSTrader for Windows

## Development installer

Run the packaging script from the repository root:

```powershell
.\scripts\package-windows.ps1 -Version 0.1.0
```

It creates a clean Release staging directory in `dist/app` and Velopack artifacts in `dist/releases`. The local installer is unsigned and intended only for development and testing.

## GitHub release

The workflow `.github/workflows/windows-release.yml` runs for semantic version tags:

```powershell
git tag v0.1.0
git push origin v0.1.0
```

The workflow builds and tests the Windows client, packages it with the pinned Velopack tool, uploads the artifacts, and creates the matching GitHub Release.

For version `0.1.1` and later, Velopack reads previous packages in the release directory and produces delta artifacts when appropriate. Clients can fall back to the full package if a delta is unavailable or unsuitable.

## Signing before public distribution

Do not present an unsigned development installer as a trusted public release. Before public distribution, obtain an Authenticode-capable certificate or configure Azure Artifact Signing, store its credentials as GitHub Actions secrets, and add the corresponding Velopack signing arguments to `scripts/package-windows.ps1`.

Never commit certificate private keys, passwords, signing tokens, `db_config.json`, or the JWT secret.

## Server deployment

The Windows package contains the client only. Deploy `smstrader_http` and PostgreSQL separately on the VPS, use a publicly trusted TLS certificate, and set the packaged `app_config.json` URL to that HTTPS endpoint before building the public release.
