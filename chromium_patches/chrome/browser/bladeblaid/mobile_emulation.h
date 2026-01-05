// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_H_
#define CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_H_

#include <string>

#include "chrome/browser/bladeblaid/bladeblaid_profile_types.h"
#include "third_party/blink/public/common/widget/device_emulation_params.h"
#include "third_party/blink/public/mojom/widget/device_emulation_params.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace bladeblaid {

// Mobile device presets matching common Android devices
struct MobileDevicePreset {
  const char* name;
  int width;
  int height;
  double device_pixel_ratio;
  const char* user_agent;
};

// User agent profiles for mobile emulation
enum class MobileUAProfile {
  kChromeAndroid,      // Standard Chrome on Android
  kChromeAndroidTablet // Chrome on Android tablet
};

// Manages mobile device emulation for BladeBlaid profiles.
// Uses Chromium's built-in device emulation infrastructure (same as DevTools).
class MobileEmulationManager {
 public:
  MobileEmulationManager();
  ~MobileEmulationManager();

  // Build device emulation params from a BladeBlaid profile.
  // Only applies to mobile type profiles.
  blink::mojom::DeviceEmulationParamsPtr BuildEmulationParams(
      const BladeBlaidProfile& profile) const;

  // Apply mobile emulation to a WebContents.
  // This enables touch events, sets viewport, and overrides UA.
  void ApplyMobileEmulation(content::WebContents* web_contents,
                            const BladeBlaidProfile& profile);

  // Remove mobile emulation from a WebContents.
  void RemoveMobileEmulation(content::WebContents* web_contents);

  // Check if a profile should use mobile emulation.
  static bool ShouldEmulate(const BladeBlaidProfile& profile);

  // Generate mobile user agent string.
  std::string GenerateMobileUserAgent(const BladeBlaidProfile& profile) const;

  // Generate mobile user agent metadata for Client Hints.
  void GetMobileUserAgentMetadata(const BladeBlaidProfile& profile,
                                  std::string* brand,
                                  std::string* version,
                                  std::string* platform,
                                  std::string* platform_version,
                                  bool* mobile) const;

  // Get common mobile device presets.
  static const MobileDevicePreset& GetPixel6Preset();
  static const MobileDevicePreset& GetPixel6ProPreset();
  static const MobileDevicePreset& GetSamsungS21Preset();
  static const MobileDevicePreset& GetGenericMobilePreset();

 private:
  // Build screen parameters for emulation.
  void BuildScreenParams(const BladeBlaidProfile& profile,
                         blink::mojom::DeviceEmulationParams* params) const;

  // Build viewport parameters.
  void BuildViewportParams(const BladeBlaidProfile& profile,
                           blink::mojom::DeviceEmulationParams* params) const;

  // Get Chrome version for UA string.
  std::string GetChromeVersion() const;

  // Get Android version for UA string.
  std::string GetAndroidVersion() const;
};

// Apply mobile emulation for a BladeBlaid profile if needed.
// Called during WebContents creation for mobile profiles.
void MaybeApplyMobileEmulation(content::WebContents* web_contents,
                               const std::string& bladeblaid_profile_id);

// Check if current process is running in mobile emulation mode.
bool IsRunningInMobileEmulationMode();

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_H_
