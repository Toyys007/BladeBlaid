# BladeBlaid - Windows Chromium Fork Build Instructions

## Prerequisites

1. **Windows 10/11 (64-bit)** - Required for building and running
2. **Visual Studio 2022** - With C++ desktop development workload
3. **Windows SDK 10.0.22621.0** or newer
4. **At least 100GB free disk space**
5. **16GB+ RAM recommended**
6. **depot_tools** - Chromium build tools

## Step 1: Set Up Build Environment

```powershell
# Install depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git C:\depot_tools

# Add to PATH (run as Administrator or add permanently via System Properties)
$env:PATH = "C:\depot_tools;$env:PATH"
[Environment]::SetEnvironmentVariable("PATH", "C:\depot_tools;$env:PATH", "User")

# Disable auto-update (optional, for consistent builds)
$env:DEPOT_TOOLS_UPDATE = "0"
```

## Step 2: Fetch Chromium Source

```powershell
# Create working directory
mkdir C:\chromium
cd C:\chromium

# Create .gclient file
@"
solutions = [
  {
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {},
  },
]
target_os = ["win"]
"@ | Out-File -FilePath .gclient -Encoding ASCII

# Fetch source (this takes several hours)
gclient sync --nohooks --no-history

# Run hooks
gclient runhooks
```

## Step 3: Apply BladeBlaid Patches

```powershell
# Copy BladeBlaid source files to Chromium tree
cd C:\chromium\src

# Create bladeblaid directory
mkdir chrome\browser\bladeblaid

# Copy all bladeblaid source files
Copy-Item -Path "path\to\BladeBlaid\chromium_patches\chrome\browser\bladeblaid\*" `
          -Destination "chrome\browser\bladeblaid\" -Recurse

# Apply patches to existing Chromium files
# Each patch file shows the modifications needed
# Apply manually or use git apply if formatted as git patches
```

### Files to Add (New)
```
chrome/browser/bladeblaid/
├── BUILD.gn
├── bladeblaid_profile_types.h
├── bladeblaid_profile_service.h
├── bladeblaid_profile_service.cc
├── profiles_lite_ui.h
└── profiles_lite_ui.cc
```

### Files to Modify (Patches)
See `patches/` directory for unified diffs:

1. **chrome/browser/BUILD.gn** - Add bladeblaid dependency
2. **chrome/browser/chrome_browser_main.cc** - Initialize profile service
3. **chrome/browser/ui/webui/chrome_web_ui_controller_factory.cc** - Register WebUI
4. **chrome/app/chrome_command_ids.h** - Add menu command ID
5. **chrome/browser/ui/toolbar/app_menu_model.cc** - Add menu item
6. **chrome/browser/ui/browser_commands.cc/h** - Add command handler
7. **chrome/browser/ui/browser_command_controller.cc** - Execute command

## Step 4: Configure GN Build

```powershell
cd C:\chromium\src

# Generate build configuration
gn gen out\BladeBlaid --args='
is_debug = false
is_component_build = false
target_cpu = "x64"
target_os = "win"

# Windows-specific
is_clang = true
use_lld = true

# Disable unwanted features
enable_nacl = false
enable_remoting = false
enable_reporting = false
enable_service_discovery = false

# Disable telemetry
safe_browsing_mode = 0
enable_hangout_services_extension = false

# Build optimization
symbol_level = 0
blink_symbol_level = 0

# Optional: faster builds
use_thin_lto = false
is_official_build = false

# Chrome branding (optional - use Chromium branding for personal use)
is_chrome_branded = false
chrome_pgo_phase = 0
'
```

## Step 5: Build

```powershell
cd C:\chromium\src

# Build Chrome target (takes 2-6 hours depending on hardware)
autoninja -C out\BladeBlaid chrome

# Or build specific targets for faster iteration:
autoninja -C out\BladeBlaid chrome/browser/bladeblaid:bladeblaid
autoninja -C out\BladeBlaid chrome/browser/bladeblaid:webui
```

## Step 6: Set Up profiles.json

```powershell
# Create BladeBlaid data directory
$bladeblaidDir = "$env:LOCALAPPDATA\BladeBlaid"
New-Item -ItemType Directory -Force -Path $bladeblaidDir

# Copy example profiles.json
Copy-Item "path\to\BladeBlaid\chromium_patches\example_profiles.json" `
          "$bladeblaidDir\profiles.json"
```

## Step 7: Run BladeBlaid

```powershell
# Run the built browser
C:\chromium\src\out\BladeBlaid\chrome.exe

# Or with a specific profile
C:\chromium\src\out\BladeBlaid\chrome.exe --bladeblaid-profile=desktop_01 --user-data-dir="$env:LOCALAPPDATA\BladeBlaid\Profiles\BladeBlaid_desktop_01"
```

## Verification Steps

### 1. Verify Profile Service Initialization
Open Chrome DevTools console on any page and check for startup log:
```
BladeBlaid: Loaded X profiles
```

### 2. Verify WebUI Page
Navigate to: `chrome://profiles-lite`

You should see:
- List of profiles from profiles.json
- Create/Delete/Launch buttons working
- Profile status indicators

### 3. Verify Profile Isolation
1. Launch two different profiles
2. Verify they have separate data directories
3. Check cookies/localStorage are isolated

### 4. Verify Menu Integration
1. Click the three-dot menu (⋮)
2. Look for "Profiles…" menu item
3. Click to open chrome://profiles-lite

## Troubleshooting

### Build Errors

**Missing dependencies:**
```powershell
gclient sync
gclient runhooks
```

**Compiler errors in bladeblaid:**
Check that all includes are correct and BUILD.gn dependencies are complete.

### Runtime Errors

**Profile service not initializing:**
- Check `%LOCALAPPDATA%\BladeBlaid\profiles.json` exists
- Verify JSON is valid (use jsonlint)
- Check Windows Event Viewer for crashes

**WebUI not loading:**
- Verify WebUI registration in chrome_web_ui_controller_factory.cc
- Check browser console for JavaScript errors

## Project Structure

```
%LOCALAPPDATA%\BladeBlaid\
├── profiles.json              # Profile definitions
└── Profiles\                  # Profile data directories
    ├── BladeBlaid_desktop_01\ # User data for desktop_01
    ├── BladeBlaid_mobile_01\  # User data for mobile_01
    └── ...
```

## Command-Line Switches

| Switch | Description |
|--------|-------------|
| `--bladeblaid-profile=ID` | Load specific profile by ID |
| `--bladeblaid-mobile` | Enable mobile emulation mode |
| `--user-data-dir=PATH` | Override user data directory |

## Notes

- This is a **Windows-only** fork for **personal use**
- No passwords are stored - identity.email_label is display only
- User logs into Microsoft/Bing manually per profile
- Fingerprint settings are applied in later tasks (Task B+)
