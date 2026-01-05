// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_PROFILES_LITE_UI_H_
#define CHROME_BROWSER_BLADEBLAID_PROFILES_LITE_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace bladeblaid {

// WebUI controller for chrome://profiles-lite
class ProfilesLiteUI : public content::WebUIController {
 public:
  explicit ProfilesLiteUI(content::WebUI* web_ui);
  ~ProfilesLiteUI() override;

  ProfilesLiteUI(const ProfilesLiteUI&) = delete;
  ProfilesLiteUI& operator=(const ProfilesLiteUI&) = delete;
};

// Message handler for profile operations
class ProfilesLiteHandler : public content::WebUIMessageHandler {
 public:
  ProfilesLiteHandler();
  ~ProfilesLiteHandler() override;

  ProfilesLiteHandler(const ProfilesLiteHandler&) = delete;
  ProfilesLiteHandler& operator=(const ProfilesLiteHandler&) = delete;

  // WebUIMessageHandler implementation
  void RegisterMessages() override;

 private:
  // Handler for getProfiles request
  void HandleGetProfiles(const base::Value::List& args);

  // Handler for launchProfile request
  void HandleLaunchProfile(const base::Value::List& args);

  // Handler for createProfile request
  void HandleCreateProfile(const base::Value::List& args);

  // Handler for deleteProfile request
  void HandleDeleteProfile(const base::Value::List& args);

  // Handler for getProfileStatus request (checks if data dir exists)
  void HandleGetProfileStatus(const base::Value::List& args);
};

// Returns the HTML content for the profiles-lite page
std::string GetProfilesLiteHTML();

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_PROFILES_LITE_UI_H_
