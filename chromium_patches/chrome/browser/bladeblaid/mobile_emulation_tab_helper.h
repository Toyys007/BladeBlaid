// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_TAB_HELPER_H_
#define CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_TAB_HELPER_H_

#include "chrome/browser/bladeblaid/bladeblaid_profile_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace bladeblaid {

// Tab helper that manages mobile emulation for a WebContents.
// Automatically applies mobile device emulation when the WebContents
// is created for a mobile BladeBlaid profile.
//
// This includes:
// - Device emulation params (viewport, DPR, screen size)
// - Touch event simulation
// - Pointer/hover media query overrides
// - User agent override
class MobileEmulationTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<MobileEmulationTabHelper> {
 public:
  ~MobileEmulationTabHelper() override;

  // Disable copy/move
  MobileEmulationTabHelper(const MobileEmulationTabHelper&) = delete;
  MobileEmulationTabHelper& operator=(const MobileEmulationTabHelper&) = delete;

  // Initialize mobile emulation for this tab.
  // Should be called after WebContents creation if mobile profile.
  void InitializeMobileEmulation(const BladeBlaidProfile& profile);

  // Check if mobile emulation is active.
  bool IsMobileEmulationActive() const { return emulation_active_; }

  // Get the profile being emulated.
  const BladeBlaidProfile* GetEmulatedProfile() const;

  // content::WebContentsObserver overrides:
  void RenderViewReady() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;

 private:
  friend class content::WebContentsUserData<MobileEmulationTabHelper>;

  explicit MobileEmulationTabHelper(content::WebContents* web_contents);

  // Apply device emulation to the render widget.
  void ApplyDeviceEmulation();

  // Update emulation after navigation (in case renderer changed).
  void UpdateEmulationAfterNavigation();

  // Profile being emulated (stored by ID, looked up when needed).
  std::string profile_id_;

  // Whether emulation is currently active.
  bool emulation_active_ = false;

  // Cached emulation parameters.
  std::unique_ptr<BladeBlaidProfile> cached_profile_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

// Create tab helper for mobile emulation if needed.
// Should be called when creating WebContents for a BladeBlaid profile.
void MaybeCreateMobileEmulationTabHelper(content::WebContents* web_contents,
                                          const std::string& profile_id);

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_MOBILE_EMULATION_TAB_HELPER_H_
