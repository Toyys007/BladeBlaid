// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/privacy_settings_manager.h"

#include "base/logging.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"
#include "chrome/browser/bladeblaid/tracker_blocker.h"
#include "chrome/browser/bladeblaid/url_sanitizer.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace bladeblaid {

PrivacySettingsManager::PrivacySettingsManager() = default;
PrivacySettingsManager::~PrivacySettingsManager() = default;

void PrivacySettingsManager::ApplyPrivacySettings(
    Profile* chrome_profile,
    const BladeBlaidProfile& bladeblaid_profile) {
  if (!chrome_profile) {
    LOG(ERROR) << "BladeBlaid: Cannot apply privacy settings - null profile";
    return;
  }

  const PrivacyConfig& privacy = bladeblaid_profile.privacy;

  LOG(INFO) << "BladeBlaid: Applying privacy settings for profile: "
            << bladeblaid_profile.profile_id;

  // Apply cookie settings
  ApplyCookieSettings(chrome_profile, privacy);

  // Apply WebRTC settings
  ApplyWebRTCSettings(chrome_profile, privacy);

  // Apply HTTPS-First mode (enabled by default for privacy)
  ApplyHTTPSFirstMode(chrome_profile, true);

  // Apply Do Not Track header
  ApplyDoNotTrack(chrome_profile, true);

  // Configure URL sanitizer
  ConfigureURLSanitizer(privacy.sanitize_urls);

  // Configure tracker blocker
  ConfigureTrackerBlocker(privacy.block_trackers);

  LOG(INFO) << "BladeBlaid: Privacy settings applied - "
            << "3P cookies: " << (privacy.block_third_party_cookies ? "blocked" : "allowed")
            << ", Trackers: " << (privacy.block_trackers ? "blocked" : "allowed")
            << ", URL sanitization: " << (privacy.sanitize_urls ? "on" : "off")
            << ", WebRTC local IP: " << (privacy.webrtc_local_ip ? "allowed" : "blocked");
}

void PrivacySettingsManager::ApplyCookieSettings(
    Profile* chrome_profile,
    const PrivacyConfig& privacy_config) {
  PrefService* prefs = chrome_profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // Block third-party cookies if configured
  ApplyThirdPartyCookieBlocking(prefs, privacy_config.block_third_party_cookies);
  SetCookieContentSetting(chrome_profile, privacy_config.block_third_party_cookies);
}

void PrivacySettingsManager::ApplyThirdPartyCookieBlocking(PrefService* prefs,
                                                           bool block) {
  // Set the cookie behavior preference
  // Values: 0 = Allow all, 1 = Block third-party, 2 = Block all
  if (block) {
    prefs->SetInteger(prefs::kCookieControlsMode, 1);  // Block third-party in Incognito + regular
  }

  // Also set the block_third_party_cookies pref if available
  // This uses Chromium's standard third-party cookie blocking mechanism
}

void PrivacySettingsManager::SetCookieContentSetting(Profile* chrome_profile,
                                                      bool block_third_party) {
  HostContentSettingsMap* settings_map =
      HostContentSettingsMapFactory::GetForProfile(chrome_profile);

  if (!settings_map) {
    return;
  }

  if (block_third_party) {
    // Set default to block third-party cookies
    // This uses Chrome's content settings infrastructure
    settings_map->SetDefaultContentSetting(
        ContentSettingsType::COOKIES,
        CONTENT_SETTING_ALLOW);  // First party allowed

    // Note: Chrome's third-party cookie blocking is controlled separately
    // via prefs::kCookieControlsMode which we set above
  }
}

void PrivacySettingsManager::ApplyWebRTCSettings(
    Profile* chrome_profile,
    const PrivacyConfig& privacy_config) {
  PrefService* prefs = chrome_profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // WebRTC IP handling policy:
  // - "default" = Use default behavior (may leak local IP)
  // - "default_public_and_private_interfaces" = Use all interfaces
  // - "default_public_interface_only" = Only public interface (hides local IP)
  // - "disable_non_proxied_udp" = Most restrictive (may break WebRTC)

  if (!privacy_config.webrtc_local_ip) {
    // Prevent local IP leak - use public interface only
    prefs->SetString(prefs::kWebRTCIPHandlingPolicy,
                     "default_public_interface_only");

    // Also disable multiple routes
    prefs->SetBoolean(prefs::kWebRTCMultipleRoutesEnabled, false);

    // Disable non-proxied UDP if we want to be extra restrictive
    // prefs->SetBoolean(prefs::kWebRTCNonProxiedUdpEnabled, false);

    LOG(INFO) << "BladeBlaid: WebRTC local IP leak prevention enabled";
  } else {
    // Allow default WebRTC behavior
    prefs->SetString(prefs::kWebRTCIPHandlingPolicy, "default");
    prefs->SetBoolean(prefs::kWebRTCMultipleRoutesEnabled, true);
  }
}

void PrivacySettingsManager::ApplyHTTPSFirstMode(Profile* chrome_profile,
                                                  bool enabled) {
  PrefService* prefs = chrome_profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // Enable HTTPS-First Mode
  // This attempts to upgrade all navigations to HTTPS first
  prefs->SetBoolean(prefs::kHttpsOnlyModeEnabled, enabled);

  if (enabled) {
    LOG(INFO) << "BladeBlaid: HTTPS-First mode enabled";
  }
}

void PrivacySettingsManager::ApplyDoNotTrack(Profile* chrome_profile,
                                              bool enabled) {
  PrefService* prefs = chrome_profile->GetPrefs();
  if (!prefs) {
    return;
  }

  // Enable Do Not Track header
  prefs->SetBoolean(prefs::kEnableDoNotTrack, enabled);

  if (enabled) {
    LOG(INFO) << "BladeBlaid: Do Not Track header enabled";
  }
}

void PrivacySettingsManager::ConfigureURLSanitizer(bool enabled) {
  URLSanitizer* sanitizer = GetURLSanitizer();
  if (sanitizer) {
    sanitizer->SetEnabled(enabled);
    LOG(INFO) << "BladeBlaid: URL sanitization "
              << (enabled ? "enabled" : "disabled");
  }
}

void PrivacySettingsManager::ConfigureTrackerBlocker(bool enabled) {
  TrackerBlocker* blocker = GetTrackerBlocker();
  if (blocker) {
    blocker->SetEnabled(enabled);
    LOG(INFO) << "BladeBlaid: Tracker blocking "
              << (enabled ? "enabled" : "disabled");
  }
}

std::string PrivacySettingsManager::GetPrivacyStateSummary(
    Profile* chrome_profile) const {
  if (!chrome_profile) {
    return "Profile is null";
  }

  PrefService* prefs = chrome_profile->GetPrefs();
  if (!prefs) {
    return "PrefService is null";
  }

  std::string summary = "Privacy State:\n";

  // Cookie settings
  int cookie_mode = prefs->GetInteger(prefs::kCookieControlsMode);
  summary += "- Cookie Control Mode: " + std::to_string(cookie_mode) + "\n";

  // WebRTC
  std::string webrtc_policy = prefs->GetString(prefs::kWebRTCIPHandlingPolicy);
  summary += "- WebRTC IP Policy: " + webrtc_policy + "\n";

  // HTTPS-First
  bool https_first = prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled);
  summary += "- HTTPS-First: " + std::string(https_first ? "on" : "off") + "\n";

  // DNT
  bool dnt = prefs->GetBoolean(prefs::kEnableDoNotTrack);
  summary += "- Do Not Track: " + std::string(dnt ? "on" : "off") + "\n";

  // URL Sanitizer
  URLSanitizer* sanitizer = GetURLSanitizer();
  summary += "- URL Sanitizer: " +
             std::string(sanitizer && sanitizer->IsEnabled() ? "on" : "off") + "\n";

  // Tracker Blocker
  TrackerBlocker* blocker = GetTrackerBlocker();
  summary += "- Tracker Blocker: " +
             std::string(blocker && blocker->IsEnabled() ? "on" : "off") + "\n";

  return summary;
}

void ApplyBladeBlaidPrivacySettings(Profile* chrome_profile,
                                    const std::string& bladeblaid_profile_id) {
  if (bladeblaid_profile_id.empty()) {
    LOG(INFO) << "BladeBlaid: No profile ID specified, skipping privacy settings";
    return;
  }

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();
  if (!service || !service->IsInitialized()) {
    LOG(WARNING) << "BladeBlaid: Profile service not initialized";
    return;
  }

  const BladeBlaidProfile* profile = service->GetProfileById(bladeblaid_profile_id);
  if (!profile) {
    LOG(WARNING) << "BladeBlaid: Profile not found: " << bladeblaid_profile_id;
    return;
  }

  PrivacySettingsManager manager;
  manager.ApplyPrivacySettings(chrome_profile, *profile);
}

}  // namespace bladeblaid
