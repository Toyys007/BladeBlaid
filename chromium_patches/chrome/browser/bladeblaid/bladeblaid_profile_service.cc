// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

#if BUILDFLAG(IS_WIN)
#include <shlobj.h>
#include "base/win/windows_types.h"
#endif

namespace bladeblaid {

namespace {

// Singleton instance
BladeBlaidProfileService* g_instance = nullptr;

// Config file name
constexpr char kConfigFileName[] = "profiles.json";
constexpr char kBladeBlaidDirName[] = "BladeBlaid";
constexpr int kExpectedConfigVersion = 1;

#if BUILDFLAG(IS_WIN)
base::FilePath GetLocalAppDataPath() {
  wchar_t path[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
    return base::FilePath(path);
  }
  // Fallback to executable directory
  base::FilePath exe_path;
  if (base::PathService::Get(base::DIR_EXE, &exe_path)) {
    return exe_path;
  }
  return base::FilePath();
}
#endif

}  // namespace

// static
BladeBlaidProfileService* BladeBlaidProfileService::GetInstance() {
  if (!g_instance) {
    g_instance = new BladeBlaidProfileService();
  }
  return g_instance;
}

BladeBlaidProfileService::BladeBlaidProfileService() = default;

BladeBlaidProfileService::~BladeBlaidProfileService() {
  if (g_instance == this) {
    g_instance = nullptr;
  }
}

bool BladeBlaidProfileService::Initialize() {
  if (initialized_) {
    return true;
  }

  base::FilePath config_path = GetConfigFilePath();
  if (config_path.empty()) {
    LOG(ERROR) << "BladeBlaid: Could not determine config file path";
    return false;
  }

  // Create BladeBlaid directory if it doesn't exist
  base::FilePath bladeblaid_dir = config_path.DirName();
  if (!base::DirectoryExists(bladeblaid_dir)) {
    if (!base::CreateDirectory(bladeblaid_dir)) {
      LOG(ERROR) << "BladeBlaid: Could not create directory: "
                 << bladeblaid_dir.value();
      return false;
    }
  }

  // Check if config file exists
  if (!base::PathExists(config_path)) {
    // Create default config with example profile
    LOG(INFO) << "BladeBlaid: Creating default profiles.json at: "
              << config_path.value();

    config_.version = kExpectedConfigVersion;
    config_.profiles.push_back(
        CreateDefaultProfile("default_desktop", ProfileType::kDesktop,
                            "Default Desktop Profile"));

    if (!SaveConfig()) {
      LOG(ERROR) << "BladeBlaid: Could not create default config";
      return false;
    }
  }

  // Read config file
  std::string json_content;
  if (!base::ReadFileToString(config_path, &json_content)) {
    LOG(ERROR) << "BladeBlaid: Could not read config file: "
               << config_path.value();
    return false;
  }

  // Parse config
  if (!ParseConfig(json_content)) {
    LOG(ERROR) << "BladeBlaid: Failed to parse profiles.json";
    return false;
  }

  initialized_ = true;
  LOG(INFO) << "BladeBlaid: Loaded " << config_.profiles.size() << " profiles";

  NotifyProfilesLoaded();
  return true;
}

const std::vector<BladeBlaidProfile>& BladeBlaidProfileService::GetProfiles() const {
  return config_.profiles;
}

const BladeBlaidProfile* BladeBlaidProfileService::GetProfileById(
    const std::string& profile_id) const {
  for (const auto& profile : config_.profiles) {
    if (profile.profile_id == profile_id) {
      return &profile;
    }
  }
  return nullptr;
}

const BladeBlaidProfile* BladeBlaidProfileService::GetActiveProfile() const {
  if (active_profile_id_.empty()) {
    return nullptr;
  }
  return GetProfileById(active_profile_id_);
}

bool BladeBlaidProfileService::SetActiveProfile(const std::string& profile_id) {
  if (GetProfileById(profile_id) == nullptr) {
    LOG(WARNING) << "BladeBlaid: Profile not found: " << profile_id;
    return false;
  }
  active_profile_id_ = profile_id;
  return true;
}

base::FilePath BladeBlaidProfileService::GetConfigFilePath() const {
  if (!config_file_path_override_.empty()) {
    return config_file_path_override_;
  }

#if BUILDFLAG(IS_WIN)
  base::FilePath local_app_data = GetLocalAppDataPath();
  if (local_app_data.empty()) {
    return base::FilePath();
  }
  return local_app_data.AppendASCII(kBladeBlaidDirName)
                       .AppendASCII(kConfigFileName);
#else
  // Non-Windows: not supported, but return empty path
  return base::FilePath();
#endif
}

base::FilePath BladeBlaidProfileService::GetBladeBlaidUserDataDir() const {
#if BUILDFLAG(IS_WIN)
  base::FilePath local_app_data = GetLocalAppDataPath();
  if (local_app_data.empty()) {
    return base::FilePath();
  }
  return local_app_data.AppendASCII(kBladeBlaidDirName).AppendASCII("Profiles");
#else
  return base::FilePath();
#endif
}

base::FilePath BladeBlaidProfileService::GetProfileDataDir(
    const std::string& profile_id) const {
  const BladeBlaidProfile* profile = GetProfileById(profile_id);
  if (!profile) {
    return base::FilePath();
  }

  base::FilePath base_dir = GetBladeBlaidUserDataDir();
  if (base_dir.empty()) {
    return base::FilePath();
  }

  return base_dir.AppendASCII(profile->GetDataDirName());
}

bool BladeBlaidProfileService::CreateProfileDirectory(
    const std::string& profile_id) {
  base::FilePath profile_dir = GetProfileDataDir(profile_id);
  if (profile_dir.empty()) {
    return false;
  }

  if (base::DirectoryExists(profile_dir)) {
    return true;  // Already exists
  }

  return base::CreateDirectory(profile_dir);
}

bool BladeBlaidProfileService::DeleteProfileDirectory(
    const std::string& profile_id) {
  base::FilePath profile_dir = GetProfileDataDir(profile_id);
  if (profile_dir.empty()) {
    return false;
  }

  if (!base::DirectoryExists(profile_dir)) {
    return true;  // Already gone
  }

  return base::DeletePathRecursively(profile_dir);
}

bool BladeBlaidProfileService::ProfileDirectoryExists(
    const std::string& profile_id) const {
  base::FilePath profile_dir = GetProfileDataDir(profile_id);
  if (profile_dir.empty()) {
    return false;
  }
  return base::DirectoryExists(profile_dir);
}

bool BladeBlaidProfileService::CreateProfile(
    const std::string& profile_id,
    ProfileType type,
    const std::string& label) {
  // Check if profile already exists
  if (GetProfileById(profile_id) != nullptr) {
    LOG(WARNING) << "BladeBlaid: Profile already exists: " << profile_id;
    return false;
  }

  // Validate profile_id (alphanumeric and underscores only)
  for (char c : profile_id) {
    if (!base::IsAsciiAlphaNumeric(c) && c != '_') {
      LOG(ERROR) << "BladeBlaid: Invalid profile_id: " << profile_id;
      return false;
    }
  }

  BladeBlaidProfile new_profile = CreateDefaultProfile(profile_id, type, label);
  config_.profiles.push_back(std::move(new_profile));

  if (!SaveConfig()) {
    config_.profiles.pop_back();
    return false;
  }

  // Create the data directory
  CreateProfileDirectory(profile_id);

  NotifyProfileCreated(profile_id);
  return true;
}

bool BladeBlaidProfileService::DeleteProfile(const std::string& profile_id,
                                              bool delete_data) {
  auto it = std::find_if(
      config_.profiles.begin(), config_.profiles.end(),
      [&profile_id](const BladeBlaidProfile& p) {
        return p.profile_id == profile_id;
      });

  if (it == config_.profiles.end()) {
    LOG(WARNING) << "BladeBlaid: Profile not found: " << profile_id;
    return false;
  }

  // Clear active profile if we're deleting it
  if (active_profile_id_ == profile_id) {
    active_profile_id_.clear();
  }

  config_.profiles.erase(it);

  if (!SaveConfig()) {
    LOG(ERROR) << "BladeBlaid: Failed to save config after deletion";
    // Continue anyway to delete data if requested
  }

  if (delete_data) {
    DeleteProfileDirectory(profile_id);
  }

  NotifyProfileDeleted(profile_id);
  return true;
}

bool BladeBlaidProfileService::SaveConfig() {
  base::Value::Dict root;
  root.Set("version", config_.version);

  base::Value::List profiles_list;
  for (const auto& profile : config_.profiles) {
    profiles_list.Append(ProfileToDict(profile));
  }
  root.Set("profiles", std::move(profiles_list));

  std::string json_output;
  if (!base::JSONWriter::WriteWithOptions(
          base::Value(std::move(root)),
          base::JSONWriter::OPTIONS_PRETTY_PRINT,
          &json_output)) {
    LOG(ERROR) << "BladeBlaid: Failed to serialize config to JSON";
    return false;
  }

  base::FilePath config_path = GetConfigFilePath();
  if (config_path.empty()) {
    return false;
  }

  if (!base::WriteFile(config_path, json_output)) {
    LOG(ERROR) << "BladeBlaid: Failed to write config file";
    return false;
  }

  return true;
}

bool BladeBlaidProfileService::ParseConfig(const std::string& json_content) {
  auto result = base::JSONReader::ReadAndReturnValueWithError(json_content);
  if (!result.has_value()) {
    LOG(ERROR) << "BladeBlaid: JSON parse error: " << result.error().message;
    return false;
  }

  if (!result->is_dict()) {
    LOG(ERROR) << "BladeBlaid: Config root must be an object";
    return false;
  }

  const base::Value::Dict& root = result->GetDict();

  // Validate version
  std::optional<int> version = root.FindInt("version");
  if (!version.has_value() || *version != kExpectedConfigVersion) {
    LOG(ERROR) << "BladeBlaid: Invalid or missing config version. Expected: "
               << kExpectedConfigVersion;
    return false;
  }

  config_.version = *version;
  config_.profiles.clear();

  // Parse profiles array
  const base::Value::List* profiles_list = root.FindList("profiles");
  if (!profiles_list) {
    LOG(ERROR) << "BladeBlaid: Missing 'profiles' array";
    return false;
  }

  for (const auto& profile_value : *profiles_list) {
    if (!profile_value.is_dict()) {
      LOG(WARNING) << "BladeBlaid: Skipping non-object profile entry";
      continue;
    }

    auto profile = ParseProfile(profile_value.GetDict());
    if (profile.has_value()) {
      config_.profiles.push_back(std::move(*profile));
    }
  }

  return true;
}

std::optional<BladeBlaidProfile> BladeBlaidProfileService::ParseProfile(
    const base::Value::Dict& dict) {
  BladeBlaidProfile profile;

  // Required: profile_id
  const std::string* profile_id = dict.FindString("profile_id");
  if (!profile_id || profile_id->empty()) {
    LOG(WARNING) << "BladeBlaid: Profile missing required 'profile_id'";
    return std::nullopt;
  }
  profile.profile_id = *profile_id;

  // Required: type
  const std::string* type_str = dict.FindString("type");
  if (!type_str) {
    LOG(WARNING) << "BladeBlaid: Profile missing 'type', defaulting to desktop";
    profile.type = ProfileType::kDesktop;
  } else if (*type_str == "mobile") {
    profile.type = ProfileType::kMobile;
  } else {
    profile.type = ProfileType::kDesktop;
  }

  // Optional: label
  const std::string* label = dict.FindString("label");
  profile.label = label ? *label : profile.profile_id;

  // Optional: search_engine (default: bing)
  const std::string* search_engine = dict.FindString("search_engine");
  profile.search_engine = search_engine ? *search_engine : "bing";

  // Optional: identity
  const base::Value::Dict* identity_dict = dict.FindDict("identity");
  if (identity_dict) {
    profile.identity = ParseIdentityConfig(*identity_dict);
  }

  // Optional: fingerprint
  const base::Value::Dict* fingerprint_dict = dict.FindDict("fingerprint");
  if (fingerprint_dict) {
    profile.fingerprint = ParseFingerprintConfig(*fingerprint_dict);
  }

  // Optional: privacy
  const base::Value::Dict* privacy_dict = dict.FindDict("privacy");
  if (privacy_dict) {
    profile.privacy = ParsePrivacyConfig(*privacy_dict);
  }

  // Optional: proxy (can be null or string)
  const base::Value* proxy_value = dict.Find("proxy");
  if (proxy_value && proxy_value->is_string()) {
    profile.proxy = proxy_value->GetString();
  }

  return profile;
}

ScreenConfig BladeBlaidProfileService::ParseScreenConfig(
    const base::Value::Dict& dict) {
  ScreenConfig config;

  config.width = dict.FindInt("width").value_or(config.width);
  config.height = dict.FindInt("height").value_or(config.height);
  config.device_pixel_ratio =
      dict.FindDouble("device_pixel_ratio").value_or(config.device_pixel_ratio);

  // Window variation (inner dimensions)
  const base::Value::Dict* variation = dict.FindDict("window_variation");
  if (variation) {
    config.inner_width = variation->FindInt("inner_width").value_or(config.width);
    config.inner_height = variation->FindInt("inner_height").value_or(config.height);
  } else {
    config.inner_width = config.width;
    config.inner_height = config.height;
  }

  return config;
}

LocaleConfig BladeBlaidProfileService::ParseLocaleConfig(
    const base::Value::Dict& dict) {
  LocaleConfig config;

  const std::string* language = dict.FindString("language");
  if (language) config.language = *language;

  const std::string* timezone = dict.FindString("timezone");
  if (timezone) config.timezone = *timezone;

  return config;
}

HardwareConfig BladeBlaidProfileService::ParseHardwareConfig(
    const base::Value::Dict& dict) {
  HardwareConfig config;

  config.cpu_cores = dict.FindInt("cpu_cores").value_or(config.cpu_cores);
  config.memory_gb = dict.FindInt("memory_gb").value_or(config.memory_gb);

  const std::string* gpu_vendor = dict.FindString("gpu_vendor");
  if (gpu_vendor) config.gpu_vendor = *gpu_vendor;

  return config;
}

FingerprintConfig BladeBlaidProfileService::ParseFingerprintConfig(
    const base::Value::Dict& dict) {
  FingerprintConfig config;

  const std::string* platform = dict.FindString("platform");
  if (platform) config.platform = *platform;

  const std::string* ua_profile = dict.FindString("ua_profile");
  if (ua_profile) config.ua_profile = *ua_profile;

  const std::string* noise_seed = dict.FindString("noise_seed");
  if (noise_seed) config.noise_seed = *noise_seed;

  const base::Value::Dict* screen_dict = dict.FindDict("screen");
  if (screen_dict) {
    config.screen = ParseScreenConfig(*screen_dict);
  }

  const base::Value::Dict* locale_dict = dict.FindDict("locale");
  if (locale_dict) {
    config.locale = ParseLocaleConfig(*locale_dict);
  }

  const base::Value::Dict* hardware_dict = dict.FindDict("hardware");
  if (hardware_dict) {
    config.hardware = ParseHardwareConfig(*hardware_dict);
  }

  return config;
}

PrivacyConfig BladeBlaidProfileService::ParsePrivacyConfig(
    const base::Value::Dict& dict) {
  PrivacyConfig config;

  config.block_third_party_cookies =
      dict.FindBool("block_third_party_cookies").value_or(config.block_third_party_cookies);
  config.block_trackers =
      dict.FindBool("block_trackers").value_or(config.block_trackers);
  config.sanitize_urls =
      dict.FindBool("sanitize_urls").value_or(config.sanitize_urls);
  config.webrtc_local_ip =
      dict.FindBool("webrtc_local_ip").value_or(config.webrtc_local_ip);

  return config;
}

IdentityConfig BladeBlaidProfileService::ParseIdentityConfig(
    const base::Value::Dict& dict) {
  IdentityConfig config;

  config.uses_microsoft_account =
      dict.FindBool("uses_microsoft_account").value_or(config.uses_microsoft_account);

  const std::string* email_label = dict.FindString("email_label");
  if (email_label) config.email_label = *email_label;

  return config;
}

base::Value::Dict BladeBlaidProfileService::ProfileToDict(
    const BladeBlaidProfile& profile) const {
  base::Value::Dict dict;

  dict.Set("profile_id", profile.profile_id);
  dict.Set("type", profile.type == ProfileType::kMobile ? "mobile" : "desktop");
  dict.Set("label", profile.label);
  dict.Set("search_engine", profile.search_engine);

  // Identity
  base::Value::Dict identity;
  identity.Set("uses_microsoft_account", profile.identity.uses_microsoft_account);
  identity.Set("email_label", profile.identity.email_label);
  dict.Set("identity", std::move(identity));

  // Fingerprint
  base::Value::Dict fingerprint;
  fingerprint.Set("platform", profile.fingerprint.platform);
  fingerprint.Set("ua_profile", profile.fingerprint.ua_profile);
  fingerprint.Set("noise_seed", profile.fingerprint.noise_seed);

  // Screen
  base::Value::Dict screen;
  screen.Set("width", profile.fingerprint.screen.width);
  screen.Set("height", profile.fingerprint.screen.height);
  screen.Set("device_pixel_ratio", profile.fingerprint.screen.device_pixel_ratio);

  base::Value::Dict window_variation;
  window_variation.Set("inner_width", profile.fingerprint.screen.inner_width);
  window_variation.Set("inner_height", profile.fingerprint.screen.inner_height);
  screen.Set("window_variation", std::move(window_variation));

  fingerprint.Set("screen", std::move(screen));

  // Locale
  base::Value::Dict locale;
  locale.Set("language", profile.fingerprint.locale.language);
  locale.Set("timezone", profile.fingerprint.locale.timezone);
  fingerprint.Set("locale", std::move(locale));

  // Hardware
  base::Value::Dict hardware;
  hardware.Set("cpu_cores", profile.fingerprint.hardware.cpu_cores);
  hardware.Set("memory_gb", profile.fingerprint.hardware.memory_gb);
  hardware.Set("gpu_vendor", profile.fingerprint.hardware.gpu_vendor);
  fingerprint.Set("hardware", std::move(hardware));

  dict.Set("fingerprint", std::move(fingerprint));

  // Privacy
  base::Value::Dict privacy;
  privacy.Set("block_third_party_cookies", profile.privacy.block_third_party_cookies);
  privacy.Set("block_trackers", profile.privacy.block_trackers);
  privacy.Set("sanitize_urls", profile.privacy.sanitize_urls);
  privacy.Set("webrtc_local_ip", profile.privacy.webrtc_local_ip);
  dict.Set("privacy", std::move(privacy));

  // Proxy
  if (profile.proxy.has_value()) {
    dict.Set("proxy", *profile.proxy);
  } else {
    dict.Set("proxy", base::Value());  // null
  }

  return dict;
}

BladeBlaidProfile BladeBlaidProfileService::CreateDefaultProfile(
    const std::string& profile_id,
    ProfileType type,
    const std::string& label) {
  BladeBlaidProfile profile;
  profile.profile_id = profile_id;
  profile.type = type;
  profile.label = label;
  profile.search_engine = "bing";

  // Default identity (empty label)
  profile.identity.uses_microsoft_account = false;
  profile.identity.email_label = "";

  // Default fingerprint based on type
  if (type == ProfileType::kMobile) {
    profile.fingerprint.platform = "android";
    profile.fingerprint.ua_profile = "chrome_android";
    profile.fingerprint.screen.width = 412;
    profile.fingerprint.screen.height = 915;
    profile.fingerprint.screen.device_pixel_ratio = 2.625;
    profile.fingerprint.screen.inner_width = 412;
    profile.fingerprint.screen.inner_height = 823;
    profile.fingerprint.hardware.cpu_cores = 8;
    profile.fingerprint.hardware.memory_gb = 8;
    profile.fingerprint.hardware.gpu_vendor = "qualcomm";
  } else {
    profile.fingerprint.platform = "windows";
    profile.fingerprint.ua_profile = "chrome_windows";
    profile.fingerprint.screen.width = 1920;
    profile.fingerprint.screen.height = 1080;
    profile.fingerprint.screen.device_pixel_ratio = 1.0;
    profile.fingerprint.screen.inner_width = 1920;
    profile.fingerprint.screen.inner_height = 969;
    profile.fingerprint.hardware.cpu_cores = 8;
    profile.fingerprint.hardware.memory_gb = 16;
    profile.fingerprint.hardware.gpu_vendor = "intel";
  }

  profile.fingerprint.locale.language = "en-US";
  profile.fingerprint.locale.timezone = "America/New_York";

  // Generate a default noise seed from profile_id
  // Simple hash-like seed generation
  uint32_t seed = 0;
  for (char c : profile_id) {
    seed = seed * 31 + static_cast<uint32_t>(c);
  }
  char seed_hex[9];
  snprintf(seed_hex, sizeof(seed_hex), "%08x", seed);
  profile.fingerprint.noise_seed = seed_hex;

  // Default privacy settings (privacy-focused)
  profile.privacy.block_third_party_cookies = true;
  profile.privacy.block_trackers = true;
  profile.privacy.sanitize_urls = true;
  profile.privacy.webrtc_local_ip = false;

  return profile;
}

void BladeBlaidProfileService::AddObserver(
    BladeBlaidProfileServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void BladeBlaidProfileService::RemoveObserver(
    BladeBlaidProfileServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void BladeBlaidProfileService::SetConfigFilePathForTesting(
    const base::FilePath& path) {
  config_file_path_override_ = path;
}

void BladeBlaidProfileService::NotifyProfilesLoaded() {
  for (auto& observer : observers_) {
    observer.OnProfilesLoaded();
  }
}

void BladeBlaidProfileService::NotifyProfileCreated(
    const std::string& profile_id) {
  for (auto& observer : observers_) {
    observer.OnProfileCreated(profile_id);
  }
}

void BladeBlaidProfileService::NotifyProfileDeleted(
    const std::string& profile_id) {
  for (auto& observer : observers_) {
    observer.OnProfileDeleted(profile_id);
  }
}

}  // namespace bladeblaid
