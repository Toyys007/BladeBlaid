// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/mobile_emulation.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/widget/device_emulation_params.h"

namespace bladeblaid {

namespace {

// Mobile device presets
const MobileDevicePreset kPixel6Preset = {
    "Pixel 6",
    412,   // width
    915,   // height
    2.625, // devicePixelRatio
    "Mozilla/5.0 (Linux; Android 13; Pixel 6) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
};

const MobileDevicePreset kPixel6ProPreset = {
    "Pixel 6 Pro",
    412,   // width
    892,   // height
    3.5,   // devicePixelRatio
    "Mozilla/5.0 (Linux; Android 13; Pixel 6 Pro) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
};

const MobileDevicePreset kSamsungS21Preset = {
    "Samsung Galaxy S21",
    360,   // width
    800,   // height
    3.0,   // devicePixelRatio
    "Mozilla/5.0 (Linux; Android 12; SM-G991B) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
};

const MobileDevicePreset kGenericMobilePreset = {
    "Generic Mobile",
    360,   // width
    640,   // height
    2.0,   // devicePixelRatio
    "Mozilla/5.0 (Linux; Android 12; Mobile) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36"
};

// Current Chrome version (should match actual build)
constexpr char kChromeVersion[] = "120.0.0.0";
constexpr char kAndroidVersion[] = "13";

}  // namespace

MobileEmulationManager::MobileEmulationManager() = default;
MobileEmulationManager::~MobileEmulationManager() = default;

// static
bool MobileEmulationManager::ShouldEmulate(const BladeBlaidProfile& profile) {
  return profile.type == ProfileType::kMobile;
}

// static
const MobileDevicePreset& MobileEmulationManager::GetPixel6Preset() {
  return kPixel6Preset;
}

// static
const MobileDevicePreset& MobileEmulationManager::GetPixel6ProPreset() {
  return kPixel6ProPreset;
}

// static
const MobileDevicePreset& MobileEmulationManager::GetSamsungS21Preset() {
  return kSamsungS21Preset;
}

// static
const MobileDevicePreset& MobileEmulationManager::GetGenericMobilePreset() {
  return kGenericMobilePreset;
}

blink::mojom::DeviceEmulationParamsPtr
MobileEmulationManager::BuildEmulationParams(
    const BladeBlaidProfile& profile) const {
  auto params = blink::mojom::DeviceEmulationParams::New();

  // Screen type: Mobile
  params->screen_type = blink::mojom::EmulatedScreenType::kMobile;

  // Build screen and viewport parameters
  BuildScreenParams(profile, params.get());
  BuildViewportParams(profile, params.get());

  // Device scale factor (devicePixelRatio)
  params->device_scale_factor = profile.fingerprint.screen.device_pixel_ratio;

  // Viewport settings
  params->viewport_offset = gfx::PointF(0, 0);
  params->viewport_scale = 1.0f;

  // Screen orientation: Portrait (most common for mobile)
  params->screen_orientation_type =
      display::mojom::ScreenOrientation::kPortraitPrimary;
  params->screen_orientation_angle = 0;

  LOG(INFO) << "BladeBlaid: Built mobile emulation params - "
            << "Screen: " << profile.fingerprint.screen.width << "x"
            << profile.fingerprint.screen.height
            << ", DPR: " << profile.fingerprint.screen.device_pixel_ratio;

  return params;
}

void MobileEmulationManager::BuildScreenParams(
    const BladeBlaidProfile& profile,
    blink::mojom::DeviceEmulationParams* params) const {
  const auto& screen = profile.fingerprint.screen;

  // Screen size (physical screen dimensions in CSS pixels)
  params->screen_size = gfx::Size(screen.width, screen.height);

  // View size (viewport size)
  int view_width = screen.inner_width > 0 ? screen.inner_width : screen.width;
  int view_height = screen.inner_height > 0 ? screen.inner_height : screen.height;
  params->view_size = gfx::Size(view_width, view_height);

  // View position (usually 0,0 for mobile)
  params->view_position = gfx::Point(0, 0);
}

void MobileEmulationManager::BuildViewportParams(
    const BladeBlaidProfile& profile,
    blink::mojom::DeviceEmulationParams* params) const {
  // For mobile, we want the viewport to match the screen size
  // (no browser chrome offset like on desktop)
  params->viewport_offset = gfx::PointF(0, 0);
  params->viewport_scale = 1.0f;
}

std::string MobileEmulationManager::GenerateMobileUserAgent(
    const BladeBlaidProfile& profile) const {
  // Build Chrome Android user agent string
  // Format: Mozilla/5.0 (Linux; Android VERSION; DEVICE) AppleWebKit/537.36
  //         (KHTML, like Gecko) Chrome/VERSION Mobile Safari/537.36

  std::string device_model = "Pixel 6";  // Default device

  // Determine device based on screen dimensions
  const auto& screen = profile.fingerprint.screen;
  if (screen.width >= 400 && screen.device_pixel_ratio >= 3.0) {
    device_model = "Pixel 6 Pro";
  } else if (screen.width <= 360) {
    device_model = "SM-G991B";  // Samsung S21
  }

  std::string ua = base::StringPrintf(
      "Mozilla/5.0 (Linux; Android %s; %s) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/%s Mobile Safari/537.36",
      GetAndroidVersion().c_str(),
      device_model.c_str(),
      GetChromeVersion().c_str());

  return ua;
}

void MobileEmulationManager::GetMobileUserAgentMetadata(
    const BladeBlaidProfile& profile,
    std::string* brand,
    std::string* version,
    std::string* platform,
    std::string* platform_version,
    bool* mobile) const {
  // Client Hints metadata for mobile
  *brand = "Chromium";
  *version = GetChromeVersion();
  *platform = "Android";
  *platform_version = GetAndroidVersion();
  *mobile = true;
}

std::string MobileEmulationManager::GetChromeVersion() const {
  return kChromeVersion;
}

std::string MobileEmulationManager::GetAndroidVersion() const {
  return kAndroidVersion;
}

void MobileEmulationManager::ApplyMobileEmulation(
    content::WebContents* web_contents,
    const BladeBlaidProfile& profile) {
  if (!web_contents) {
    LOG(WARNING) << "BladeBlaid: Cannot apply mobile emulation - null WebContents";
    return;
  }

  if (!ShouldEmulate(profile)) {
    return;
  }

  auto params = BuildEmulationParams(profile);

  // Get the RenderWidgetHostView to apply emulation
  content::RenderWidgetHostView* rwhv =
      web_contents->GetRenderWidgetHostView();
  if (!rwhv) {
    LOG(WARNING) << "BladeBlaid: No RenderWidgetHostView for mobile emulation";
    return;
  }

  content::RenderWidgetHost* rwh = rwhv->GetRenderWidgetHost();
  if (!rwh) {
    LOG(WARNING) << "BladeBlaid: No RenderWidgetHost for mobile emulation";
    return;
  }

  // Enable device emulation
  rwh->GetWidgetInputHandler()->EnableDeviceEmulation(*params);

  LOG(INFO) << "BladeBlaid: Mobile emulation applied for profile: "
            << profile.profile_id;
}

void MobileEmulationManager::RemoveMobileEmulation(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return;
  }

  content::RenderWidgetHostView* rwhv =
      web_contents->GetRenderWidgetHostView();
  if (!rwhv) {
    return;
  }

  content::RenderWidgetHost* rwh = rwhv->GetRenderWidgetHost();
  if (!rwh) {
    return;
  }

  rwh->GetWidgetInputHandler()->DisableDeviceEmulation();

  LOG(INFO) << "BladeBlaid: Mobile emulation removed";
}

void MaybeApplyMobileEmulation(content::WebContents* web_contents,
                               const std::string& bladeblaid_profile_id) {
  if (bladeblaid_profile_id.empty()) {
    return;
  }

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();
  if (!service || !service->IsInitialized()) {
    return;
  }

  const BladeBlaidProfile* profile =
      service->GetProfileById(bladeblaid_profile_id);
  if (!profile) {
    return;
  }

  if (MobileEmulationManager::ShouldEmulate(*profile)) {
    MobileEmulationManager manager;
    manager.ApplyMobileEmulation(web_contents, *profile);
  }
}

bool IsRunningInMobileEmulationMode() {
  const base::CommandLine& cmd = *base::CommandLine::ForCurrentProcess();
  return cmd.HasSwitch("bladeblaid-mobile");
}

}  // namespace bladeblaid
