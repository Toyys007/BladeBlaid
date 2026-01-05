// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_PRIVACY_SETTINGS_MANAGER_H_
#define CHROME_BROWSER_BLADEBLAID_PRIVACY_SETTINGS_MANAGER_H_

#include <string>

#include "chrome/browser/bladeblaid/bladeblaid_profile_types.h"

class PrefService;
class Profile;

namespace bladeblaid {

// Manages privacy settings for BladeBlaid profiles.
// Applies privacy preferences from the BladeBlaid profile spec
// to Chrome's standard preference system.
class PrivacySettingsManager {
 public:
  PrivacySettingsManager();
  ~PrivacySettingsManager();

  // Apply privacy settings from a BladeBlaid profile to a Chrome profile.
  // This modifies Chrome's preferences to match the privacy config.
  void ApplyPrivacySettings(Profile* chrome_profile,
                           const BladeBlaidProfile& bladeblaid_profile);

  // Apply only cookie settings.
  void ApplyCookieSettings(Profile* chrome_profile,
                          const PrivacyConfig& privacy_config);

  // Apply WebRTC settings.
  void ApplyWebRTCSettings(Profile* chrome_profile,
                          const PrivacyConfig& privacy_config);

  // Apply HTTPS-First mode.
  void ApplyHTTPSFirstMode(Profile* chrome_profile, bool enabled);

  // Apply Do Not Track header.
  void ApplyDoNotTrack(Profile* chrome_profile, bool enabled);

  // Enable/disable URL sanitization for this profile.
  void ConfigureURLSanitizer(bool enabled);

  // Enable/disable tracker blocking for this profile.
  void ConfigureTrackerBlocker(bool enabled);

  // Get current privacy state summary for debugging.
  std::string GetPrivacyStateSummary(Profile* chrome_profile) const;

 private:
  // Apply third-party cookie blocking.
  void ApplyThirdPartyCookieBlocking(PrefService* prefs, bool block);

  // Set content setting for cookies.
  void SetCookieContentSetting(Profile* chrome_profile, bool block_third_party);
};

// Apply privacy settings for the current BladeBlaid profile.
// Should be called after profile initialization.
void ApplyBladeBlaidPrivacySettings(Profile* chrome_profile,
                                    const std::string& bladeblaid_profile_id);

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_PRIVACY_SETTINGS_MANAGER_H_
