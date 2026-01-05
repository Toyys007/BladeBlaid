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
│   ├── bladeblaid_profile_service.h/.cc
│   ├── profiles_lite_ui.h/.cc
│   ├── privacy_settings_manager.h/.cc  # Task B
│   ├── search_engine_manager.h/.cc     # Task B
│   ├── tracker_blocker.h/.cc           # Task B
│   ├── url_sanitizer.h/.cc             # Task B
│   ├── mobile_emulation.h/.cc          # Task D
│   ├── mobile_emulation_tab_helper.h/.cc  # Task D
│   ├── mobile_ua_override.h/.cc        # Task D
│   └── BUILD.gn
├── test_pages/                   # Test pages for verification
│   └── mobile_emulation_test.html
├── patches/                      # Unified diffs for Chromium integration
├── example_profiles.json         # Example configuration
└── BUILD_INSTRUCTIONS.md         # Build guide
```

## Task A Implementation

Profile Manager skeleton:

- ✅ profiles.json loader from `%LOCALAPPDATA%\BladeBlaid\`
- ✅ JSON parsing with version validation (Chromium base/json)
- ✅ Internal API to access profiles by ID
- ✅ Profile directory management (create/delete)
- ✅ WebUI at `chrome://profiles-lite`
- ✅ Menu item "Profiles…" integration

## Task B Implementation

Per-profile privacy defaults:

- ✅ **Third-party cookie blocking** - Uses Chrome's content settings
- ✅ **Tracker blocking** - MVP domain list (Google Analytics, Facebook, etc.)
- ✅ **URL sanitization** - Strips utm_*, fbclid, gclid, msclkid, etc.
- ✅ **WebRTC IP leak prevention** - Public interface only mode
- ✅ **HTTPS-First mode** - Enabled by default
- ✅ **Default search engine** - Bing for all new profiles

### Privacy Features

| Feature | Implementation |
|---------|----------------|
| 3P Cookie Blocking | `prefs::kCookieControlsMode` |
| Tracker Blocking | Domain blocklist + beacon detection |
| URL Sanitization | Navigation throttle strips tracking params |
| WebRTC Protection | `default_public_interface_only` policy |
| HTTPS-First | `prefs::kHttpsOnlyModeEnabled` |
| DNT Header | `prefs::kEnableDoNotTrack` |

### Blocked Tracking Parameters

`utm_source`, `utm_medium`, `utm_campaign`, `utm_term`, `utm_content`,
`gclid`, `fbclid`, `msclkid`, `yclid`, `ttclid`, `ref`, `ref_src`, and more.

## Task D Implementation

Mobile emulation (Variant A - Chrome mobile emulation style):

- ✅ **Chrome Android UA** - Full mobile user agent with device model
- ✅ **Viewport from spec** - screen width/height + devicePixelRatio
- ✅ **Touch events** - maxTouchPoints = 5, ontouchstart enabled
- ✅ **Pointer: coarse** - CSS media query returns coarse pointer
- ✅ **Hover: none** - CSS media query returns no hover capability
- ✅ **Platform override** - navigator.platform returns "Linux armv8l"

### Mobile Emulation Details

| Property | Desktop Value | Mobile Emulated Value |
|----------|--------------|----------------------|
| navigator.userAgent | Windows Chrome | Android Chrome Mobile |
| navigator.platform | Win32 | Linux armv8l |
| navigator.maxTouchPoints | 0 | 5 |
| screen.width | 1920 | 412 (profile spec) |
| devicePixelRatio | 1.0 | 2.625 (profile spec) |
| pointer media | fine | coarse |
| hover media | hover | none |

### Launching Mobile Profile

```powershell
chrome.exe --bladeblaid-profile=mobile_01 --bladeblaid-mobile --user-data-dir="%LOCALAPPDATA%\BladeBlaid\Profiles\BladeBlaid_mobile_01"
```

## Hard Rules

- Windows-only
- No stored passwords (email_label is display only)
- User manually logs into Microsoft/Bing
- Minimal, isolated changes for easy rebasing

## License

BSD-style license (same as Chromium)
