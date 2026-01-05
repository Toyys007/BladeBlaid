# BladeBlaid - Windows Chromium Fork

A minimal Windows-only Chromium fork with profile management and privacy features for personal use.

## Features

- **Profile Manager**: JSON-driven profile configuration (`profiles.json`)
- **Isolated Profiles**: Separate user data directories per profile
- **Desktop & Mobile Profiles**: Desktop mode and mobile emulation (Variant A)
- **Privacy Defaults**: Per-profile privacy settings (3P cookie blocking, WebRTC control)
- **Default Search Engine**: Bing

## Quick Start

1. Build following `chromium_patches/BUILD_INSTRUCTIONS.md`
2. Copy `example_profiles.json` to `%LOCALAPPDATA%\BladeBlaid\profiles.json`
3. Launch browser and navigate to `chrome://profiles-lite`

## Profile Spec (v1)

See `chromium_patches/example_profiles.json` for complete example.

```json
{
  "version": 1,
  "profiles": [
    {
      "profile_id": "desktop_01",
      "type": "desktop",
      "label": "My Profile",
      "search_engine": "bing",
      "identity": {
        "uses_microsoft_account": true,
        "email_label": "user@outlook.com"
      },
      "fingerprint": { ... },
      "privacy": {
        "block_third_party_cookies": true,
        "block_trackers": true,
        "sanitize_urls": true,
        "webrtc_local_ip": false
      }
    }
  ]
}
```

## Project Structure

```
chromium_patches/
├── chrome/browser/bladeblaid/   # Core profile manager code
│   ├── bladeblaid_profile_types.h
│   ├── bladeblaid_profile_service.h
│   ├── bladeblaid_profile_service.cc
│   ├── profiles_lite_ui.h
│   ├── profiles_lite_ui.cc
│   └── BUILD.gn
├── patches/                      # Unified diffs for Chromium integration
├── example_profiles.json         # Example configuration
└── BUILD_INSTRUCTIONS.md         # Build guide
```

## Task A Implementation

This commit implements Task A: Profile Manager skeleton

- ✅ profiles.json loader from `%LOCALAPPDATA%\BladeBlaid\`
- ✅ JSON parsing with version validation (Chromium base/json)
- ✅ Internal API to access profiles by ID
- ✅ Profile directory management (create/delete)
- ✅ WebUI at `chrome://profiles-lite`
- ✅ Menu item "Profiles…" integration

## Hard Rules

- Windows-only
- No stored passwords (email_label is display only)
- User manually logs into Microsoft/Bing
- Minimal, isolated changes for easy rebasing

## License

BSD-style license (same as Chromium)
