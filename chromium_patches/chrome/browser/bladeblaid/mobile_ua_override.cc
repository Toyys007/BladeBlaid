// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/mobile_ua_override.h"

#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"

namespace bladeblaid {

namespace {

// Chrome version info - should be updated with each Chromium update
constexpr char kChromeMajorVersion[] = "120";
constexpr char kChromeFullVersion[] = "120.0.6099.144";
constexpr char kAndroidVersion[] = "13";
constexpr char kWebKitVersion[] = "537.36";

// Brand strings for Client Hints
constexpr char kBrandChromium[] = "Chromium";
constexpr char kBrandChrome[] = "Google Chrome";
constexpr char kBrandNotA[] = "Not_A Brand";

}  // namespace

MobileUserAgentGenerator::MobileUserAgentGenerator()
    : major_version_(kChromeMajorVersion),
      full_version_(kChromeFullVersion) {}

MobileUserAgentGenerator::~MobileUserAgentGenerator() = default;

// static
bool MobileUserAgentGenerator::UsesMobileUA(const BladeBlaidProfile& profile) {
  return profile.type == ProfileType::kMobile &&
         profile.fingerprint.ua_profile == "chrome_android";
}

std::string MobileUserAgentGenerator::GetChromeVersion() const {
  return full_version_;
}

std::string MobileUserAgentGenerator::GetDeviceModel(
    const BladeBlaidProfile& profile) const {
  const auto& screen = profile.fingerprint.screen;

  // Determine device model based on screen characteristics
  // This provides realistic device identification

  if (screen.device_pixel_ratio >= 3.5 && screen.width >= 400) {
    return "Pixel 6 Pro";
  } else if (screen.device_pixel_ratio >= 2.5 && screen.width >= 400) {
    return "Pixel 6";
  } else if (screen.device_pixel_ratio >= 3.0 && screen.width <= 360) {
    return "SM-G991B";  // Samsung Galaxy S21
  } else if (screen.device_pixel_ratio >= 2.75) {
    return "SM-A536B";  // Samsung Galaxy A53
  } else {
    return "Pixel 4a";  // Generic mid-range Android
  }
}

std::string MobileUserAgentGenerator::GenerateUserAgent(
    const BladeBlaidProfile& profile) const {
  std::string device_model = GetDeviceModel(profile);

  // Standard Chrome Android UA format:
  // Mozilla/5.0 (Linux; Android VERSION; DEVICE) AppleWebKit/WEBKIT
  // (KHTML, like Gecko) Chrome/VERSION Mobile Safari/WEBKIT

  return base::StringPrintf(
      "Mozilla/5.0 (Linux; Android %s; %s) AppleWebKit/%s "
      "(KHTML, like Gecko) Chrome/%s Mobile Safari/%s",
      kAndroidVersion,
      device_model.c_str(),
      kWebKitVersion,
      full_version_.c_str(),
      kWebKitVersion);
}

blink::UserAgentMetadata MobileUserAgentGenerator::GenerateUserAgentMetadata(
    const BladeBlaidProfile& profile) const {
  blink::UserAgentMetadata metadata;

  // Brand list for Sec-CH-UA header
  // Format: "Brand";v="Version"
  blink::UserAgentBrandVersion chromium_brand;
  chromium_brand.brand = kBrandChromium;
  chromium_brand.version = major_version_;

  blink::UserAgentBrandVersion chrome_brand;
  chrome_brand.brand = kBrandChrome;
  chrome_brand.version = major_version_;

  blink::UserAgentBrandVersion not_a_brand;
  not_a_brand.brand = kBrandNotA;
  not_a_brand.version = "8";

  metadata.brand_version_list.push_back(chromium_brand);
  metadata.brand_version_list.push_back(chrome_brand);
  metadata.brand_version_list.push_back(not_a_brand);

  // Full version list (for Sec-CH-UA-Full-Version-List)
  blink::UserAgentBrandVersion chromium_full;
  chromium_full.brand = kBrandChromium;
  chromium_full.version = full_version_;

  blink::UserAgentBrandVersion chrome_full;
  chrome_full.brand = kBrandChrome;
  chrome_full.version = full_version_;

  metadata.brand_full_version_list.push_back(chromium_full);
  metadata.brand_full_version_list.push_back(chrome_full);
  metadata.brand_full_version_list.push_back(not_a_brand);

  // Full version
  metadata.full_version = full_version_;

  // Platform info
  metadata.platform = "Android";
  metadata.platform_version = kAndroidVersion;

  // Architecture (Android is typically ARM)
  metadata.architecture = "arm";
  metadata.bitness = "64";

  // Device model
  metadata.model = GetDeviceModel(profile);

  // Mobile indicator - CRITICAL for mobile detection
  metadata.mobile = true;

  // WoW64 (Windows on Windows 64) - not applicable for Android
  metadata.wow64 = false;

  return metadata;
}

std::string GetMobileUserAgentOverride() {
  const base::CommandLine& cmd = *base::CommandLine::ForCurrentProcess();

  if (!cmd.HasSwitch("bladeblaid-profile") ||
      !cmd.HasSwitch("bladeblaid-mobile")) {
    return std::string();
  }

  std::string profile_id = cmd.GetSwitchValueASCII("bladeblaid-profile");

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();
  if (!service || !service->IsInitialized()) {
    return std::string();
  }

  const BladeBlaidProfile* profile = service->GetProfileById(profile_id);
  if (!profile || profile->type != ProfileType::kMobile) {
    return std::string();
  }

  MobileUserAgentGenerator generator;
  return generator.GenerateUserAgent(*profile);
}

std::optional<blink::UserAgentMetadata> GetMobileUserAgentMetadataOverride() {
  const base::CommandLine& cmd = *base::CommandLine::ForCurrentProcess();

  if (!cmd.HasSwitch("bladeblaid-profile") ||
      !cmd.HasSwitch("bladeblaid-mobile")) {
    return std::nullopt;
  }

  std::string profile_id = cmd.GetSwitchValueASCII("bladeblaid-profile");

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();
  if (!service || !service->IsInitialized()) {
    return std::nullopt;
  }

  const BladeBlaidProfile* profile = service->GetProfileById(profile_id);
  if (!profile || profile->type != ProfileType::kMobile) {
    return std::nullopt;
  }

  MobileUserAgentGenerator generator;
  return generator.GenerateUserAgentMetadata(*profile);
}

bool ShouldUseMobileUserAgent() {
  const base::CommandLine& cmd = *base::CommandLine::ForCurrentProcess();
  return cmd.HasSwitch("bladeblaid-mobile");
}

}  // namespace bladeblaid
