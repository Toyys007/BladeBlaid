// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_SERVICE_H_
#define CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_SERVICE_H_

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_types.h"

namespace bladeblaid {

// Observer interface for profile changes
class BladeBlaidProfileServiceObserver {
 public:
  virtual void OnProfilesLoaded() {}
  virtual void OnProfileCreated(const std::string& profile_id) {}
  virtual void OnProfileDeleted(const std::string& profile_id) {}
  virtual ~BladeBlaidProfileServiceObserver() = default;
};

// Service for managing BladeBlaid profiles
// Loads profiles from profiles.json and manages profile directories
class BladeBlaidProfileService {
 public:
  // Get singleton instance
  static BladeBlaidProfileService* GetInstance();

  BladeBlaidProfileService();
  ~BladeBlaidProfileService();

  // Disallow copy/move
  BladeBlaidProfileService(const BladeBlaidProfileService&) = delete;
  BladeBlaidProfileService& operator=(const BladeBlaidProfileService&) = delete;

  // Initialize service - loads profiles.json
  // Should be called early in browser startup
  bool Initialize();

  // Check if service is initialized
  bool IsInitialized() const { return initialized_; }

  // Get all loaded profiles
  const std::vector<BladeBlaidProfile>& GetProfiles() const;

  // Get profile by ID (returns nullptr if not found)
  const BladeBlaidProfile* GetProfileById(const std::string& profile_id) const;

  // Get currently active profile (if any)
  const BladeBlaidProfile* GetActiveProfile() const;

  // Set active profile by ID
  bool SetActiveProfile(const std::string& profile_id);

  // Profile directory management
  base::FilePath GetProfileDataDir(const std::string& profile_id) const;
  bool CreateProfileDirectory(const std::string& profile_id);
  bool DeleteProfileDirectory(const std::string& profile_id);
  bool ProfileDirectoryExists(const std::string& profile_id) const;

  // Create new profile from template and add to config
  bool CreateProfile(const std::string& profile_id,
                     ProfileType type,
                     const std::string& label);

  // Delete profile from config (optionally delete data dir)
  bool DeleteProfile(const std::string& profile_id, bool delete_data);

  // Save current config to profiles.json
  bool SaveConfig();

  // Get path to profiles.json
  base::FilePath GetConfigFilePath() const;

  // Get base user data directory for BladeBlaid profiles
  base::FilePath GetBladeBlaidUserDataDir() const;

  // Observer management
  void AddObserver(BladeBlaidProfileServiceObserver* observer);
  void RemoveObserver(BladeBlaidProfileServiceObserver* observer);

  // For testing: override config file path
  void SetConfigFilePathForTesting(const base::FilePath& path);

 private:
  // Parse profiles.json content
  bool ParseConfig(const std::string& json_content);

  // Parse individual profile from JSON
  std::optional<BladeBlaidProfile> ParseProfile(const base::Value::Dict& dict);

  // Parse nested config objects
  ScreenConfig ParseScreenConfig(const base::Value::Dict& dict);
  LocaleConfig ParseLocaleConfig(const base::Value::Dict& dict);
  HardwareConfig ParseHardwareConfig(const base::Value::Dict& dict);
  FingerprintConfig ParseFingerprintConfig(const base::Value::Dict& dict);
  PrivacyConfig ParsePrivacyConfig(const base::Value::Dict& dict);
  IdentityConfig ParseIdentityConfig(const base::Value::Dict& dict);

  // Convert profile to JSON for saving
  base::Value::Dict ProfileToDict(const BladeBlaidProfile& profile) const;

  // Create default profile template
  BladeBlaidProfile CreateDefaultProfile(const std::string& profile_id,
                                         ProfileType type,
                                         const std::string& label);

  // Notify observers
  void NotifyProfilesLoaded();
  void NotifyProfileCreated(const std::string& profile_id);
  void NotifyProfileDeleted(const std::string& profile_id);

  bool initialized_ = false;
  BladeBlaidConfig config_;
  std::string active_profile_id_;
  base::FilePath config_file_path_override_;

  base::ObserverList<BladeBlaidProfileServiceObserver> observers_;
};

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_BLADEBLAID_PROFILE_SERVICE_H_
