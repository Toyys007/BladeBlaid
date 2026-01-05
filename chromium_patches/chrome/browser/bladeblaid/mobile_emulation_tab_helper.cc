// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/mobile_emulation_tab_helper.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"
#include "chrome/browser/bladeblaid/mobile_emulation.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/widget/device_emulation_params.h"
#include "third_party/blink/public/mojom/widget/device_emulation_params.mojom.h"

namespace bladeblaid {

WEB_CONTENTS_USER_DATA_KEY_IMPL(MobileEmulationTabHelper);

MobileEmulationTabHelper::MobileEmulationTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<MobileEmulationTabHelper>(*web_contents) {}

MobileEmulationTabHelper::~MobileEmulationTabHelper() {
  // Disable emulation when tab helper is destroyed
  if (emulation_active_ && web_contents()) {
    MobileEmulationManager manager;
    manager.RemoveMobileEmulation(web_contents());
  }
}

void MobileEmulationTabHelper::InitializeMobileEmulation(
    const BladeBlaidProfile& profile) {
  if (profile.type != ProfileType::kMobile) {
    LOG(INFO) << "BladeBlaid: Profile is not mobile type, skipping emulation";
    return;
  }

  profile_id_ = profile.profile_id;

  // Cache the profile for later use
  cached_profile_ = std::make_unique<BladeBlaidProfile>(profile);

  // Mark emulation as active
  emulation_active_ = true;

  // Apply emulation immediately if render view is ready
  if (web_contents()->GetRenderWidgetHostView()) {
    ApplyDeviceEmulation();
  }

  LOG(INFO) << "BladeBlaid: Mobile emulation initialized for profile: "
            << profile_id_;
}

const BladeBlaidProfile* MobileEmulationTabHelper::GetEmulatedProfile() const {
  return cached_profile_.get();
}

void MobileEmulationTabHelper::RenderViewReady() {
  // Apply emulation when render view becomes ready
  if (emulation_active_) {
    ApplyDeviceEmulation();
  }
}

void MobileEmulationTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Nothing special needed for navigation start
}

void MobileEmulationTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Re-apply emulation after navigation in case renderer changed
  if (emulation_active_ && navigation_handle->IsInMainFrame() &&
      navigation_handle->HasCommitted()) {
    UpdateEmulationAfterNavigation();
  }
}

void MobileEmulationTabHelper::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  // Apply emulation to new frames
  if (emulation_active_ && render_frame_host->IsInPrimaryMainFrame()) {
    ApplyDeviceEmulation();
  }
}

void MobileEmulationTabHelper::ApplyDeviceEmulation() {
  if (!cached_profile_) {
    LOG(WARNING) << "BladeBlaid: No cached profile for mobile emulation";
    return;
  }

  content::RenderWidgetHostView* rwhv =
      web_contents()->GetRenderWidgetHostView();
  if (!rwhv) {
    LOG(WARNING) << "BladeBlaid: No RenderWidgetHostView available";
    return;
  }

  content::RenderWidgetHost* rwh = rwhv->GetRenderWidgetHost();
  if (!rwh) {
    LOG(WARNING) << "BladeBlaid: No RenderWidgetHost available";
    return;
  }

  // Build emulation parameters
  MobileEmulationManager manager;
  auto params = manager.BuildEmulationParams(*cached_profile_);

  // Apply emulation via the widget input handler
  rwh->GetWidgetInputHandler()->EnableDeviceEmulation(*params);

  LOG(INFO) << "BladeBlaid: Device emulation applied - "
            << "viewport: " << params->view_size.width() << "x"
            << params->view_size.height()
            << ", DPR: " << params->device_scale_factor;
}

void MobileEmulationTabHelper::UpdateEmulationAfterNavigation() {
  // Small delay might be needed for the new renderer to be fully ready
  // For now, apply immediately
  ApplyDeviceEmulation();
}

void MaybeCreateMobileEmulationTabHelper(content::WebContents* web_contents,
                                          const std::string& profile_id) {
  if (profile_id.empty() || !web_contents) {
    return;
  }

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();
  if (!service || !service->IsInitialized()) {
    return;
  }

  const BladeBlaidProfile* profile = service->GetProfileById(profile_id);
  if (!profile || profile->type != ProfileType::kMobile) {
    return;
  }

  // Create the tab helper
  MobileEmulationTabHelper::CreateForWebContents(web_contents);

  // Initialize emulation
  MobileEmulationTabHelper* helper =
      MobileEmulationTabHelper::FromWebContents(web_contents);
  if (helper) {
    helper->InitializeMobileEmulation(*profile);
  }
}

}  // namespace bladeblaid
