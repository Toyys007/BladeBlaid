// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_TYPES_H_
#define CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_TYPES_H_

#include <string>
#include <optional>

namespace bladeblaid {

// Screen configuration for fingerprint spoofing
struct ScreenConfig {
  int width = 1920;
  int height = 1080;
  double device_pixel_ratio = 1.0;
  int inner_width = 1920;
  int inner_height = 1080;
};

// Locale configuration
struct LocaleConfig {
  std::string language = "en-US";
  std::string timezone = "America/New_York";
};

// Hardware configuration for fingerprint
struct HardwareConfig {
  int cpu_cores = 4;
  int memory_gb = 8;
  std::string gpu_vendor = "intel";
};

// Complete fingerprint configuration
struct FingerprintConfig {
  std::string platform = "windows";
  std::string ua_profile = "chrome_windows";
  ScreenConfig screen;
  LocaleConfig locale;
  HardwareConfig hardware;
  std::string noise_seed;
};

// Privacy settings
struct PrivacyConfig {
  bool block_third_party_cookies = true;
  bool block_trackers = true;
  bool sanitize_urls = true;
  bool webrtc_local_ip = false;
};

// Identity label (NOT credentials - label only)
struct IdentityConfig {
  bool uses_microsoft_account = false;
  std::string email_label;  // Display label only, not used for auth
};

// Profile type enumeration
enum class ProfileType {
  kDesktop,
  kMobile
};

// Main profile definition
struct BladeBlaidProfile {
  std::string profile_id;
  ProfileType type = ProfileType::kDesktop;
  std::string label;
  std::string search_engine = "bing";
  IdentityConfig identity;
  FingerprintConfig fingerprint;
  PrivacyConfig privacy;
  std::optional<std::string> proxy;  // null = no proxy

  // Returns the user data directory subfolder name for this profile
  std::string GetDataDirName() const {
    return "BladeBlaid_" + profile_id;
  }
};

// Configuration file structure
struct BladeBlaidConfig {
  int version = 1;
  std::vector<BladeBlaidProfile> profiles;
};

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_TYPES_H_
