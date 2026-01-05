// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_MOBILE_UA_OVERRIDE_H_
#define CHROME_BROWSER_BLADEBLAID_MOBILE_UA_OVERRIDE_H_

#include <string>

#include "chrome/browser/bladeblaid/bladeblaid_profile_types.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

namespace bladeblaid {

// Generates mobile-appropriate user agent strings and metadata.
// These are used to make the browser appear as Chrome on Android.
class MobileUserAgentGenerator {
 public:
  MobileUserAgentGenerator();
  ~MobileUserAgentGenerator();

  // Generate full UA string for mobile profile.
  std::string GenerateUserAgent(const BladeBlaidProfile& profile) const;

  // Generate UA metadata for Client Hints (Sec-CH-UA-*).
  blink::UserAgentMetadata GenerateUserAgentMetadata(
      const BladeBlaidProfile& profile) const;

  // Get just the Chrome version string.
  std::string GetChromeVersion() const;

  // Get device model string based on profile.
  std::string GetDeviceModel(const BladeBlaidProfile& profile) const;

  // Check if profile uses mobile UA.
  static bool UsesMobileUA(const BladeBlaidProfile& profile);

 private:
  // Chrome version components
  std::string major_version_;
  std::string full_version_;
};

// Get mobile user agent for current BladeBlaid profile (if mobile).
// Returns empty string if not in mobile mode.
std::string GetMobileUserAgentOverride();

// Get mobile user agent metadata for current BladeBlaid profile.
// Returns nullopt if not in mobile mode.
std::optional<blink::UserAgentMetadata> GetMobileUserAgentMetadataOverride();

// Check if current session should use mobile UA.
bool ShouldUseMobileUserAgent();

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_MOBILE_UA_OVERRIDE_H_
