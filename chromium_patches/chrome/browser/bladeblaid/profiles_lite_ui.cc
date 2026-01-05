// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/profiles_lite_ui.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/process/launch.h"
#include "base/values.h"
#include "chrome/browser/bladeblaid/bladeblaid_profile_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

#if BUILDFLAG(IS_WIN)
#include "base/path_service.h"
#include "base/win/windows_types.h"
#endif

namespace bladeblaid {

namespace {

// HTML content for the profiles-lite page (embedded for simplicity)
const char kProfilesLiteHTML[] = R"html(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>BladeBlaid Profiles</title>
  <style>
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    body {
      font-family: 'Segoe UI', system-ui, sans-serif;
      background: #1a1a2e;
      color: #eee;
      padding: 24px;
      min-height: 100vh;
    }
    h1 {
      font-size: 24px;
      margin-bottom: 24px;
      color: #fff;
      font-weight: 500;
    }
    .toolbar {
      margin-bottom: 20px;
      display: flex;
      gap: 12px;
    }
    .btn {
      background: #4a4a6a;
      border: none;
      color: #fff;
      padding: 10px 20px;
      border-radius: 6px;
      cursor: pointer;
      font-size: 14px;
      transition: background 0.2s;
    }
    .btn:hover {
      background: #5a5a7a;
    }
    .btn-primary {
      background: #3b82f6;
    }
    .btn-primary:hover {
      background: #2563eb;
    }
    .btn-danger {
      background: #dc2626;
    }
    .btn-danger:hover {
      background: #b91c1c;
    }
    .btn-success {
      background: #16a34a;
    }
    .btn-success:hover {
      background: #15803d;
    }
    .profiles-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
      gap: 16px;
    }
    .profile-card {
      background: #252542;
      border-radius: 12px;
      padding: 20px;
      border: 1px solid #3a3a5a;
    }
    .profile-card.active {
      border-color: #3b82f6;
      box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.3);
    }
    .profile-header {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      margin-bottom: 12px;
    }
    .profile-label {
      font-size: 18px;
      font-weight: 500;
      color: #fff;
    }
    .profile-type {
      font-size: 12px;
      padding: 4px 8px;
      border-radius: 4px;
      background: #3a3a5a;
      color: #aaa;
      text-transform: uppercase;
    }
    .profile-type.mobile {
      background: #7c3aed;
      color: #fff;
    }
    .profile-id {
      font-size: 13px;
      color: #888;
      margin-bottom: 8px;
      font-family: monospace;
    }
    .profile-info {
      font-size: 13px;
      color: #aaa;
      margin-bottom: 4px;
    }
    .profile-info strong {
      color: #ccc;
    }
    .profile-status {
      margin-top: 12px;
      padding-top: 12px;
      border-top: 1px solid #3a3a5a;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #888;
    }
    .status-dot.exists {
      background: #22c55e;
    }
    .profile-actions {
      margin-top: 16px;
      display: flex;
      gap: 8px;
    }
    .profile-actions .btn {
      flex: 1;
      padding: 8px 12px;
      font-size: 13px;
    }
    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(0,0,0,0.7);
      align-items: center;
      justify-content: center;
      z-index: 1000;
    }
    .modal.show {
      display: flex;
    }
    .modal-content {
      background: #252542;
      border-radius: 12px;
      padding: 24px;
      min-width: 400px;
      max-width: 500px;
    }
    .modal-title {
      font-size: 20px;
      margin-bottom: 20px;
      color: #fff;
    }
    .form-group {
      margin-bottom: 16px;
    }
    .form-group label {
      display: block;
      margin-bottom: 6px;
      font-size: 14px;
      color: #ccc;
    }
    .form-group input, .form-group select {
      width: 100%;
      padding: 10px 12px;
      border: 1px solid #3a3a5a;
      border-radius: 6px;
      background: #1a1a2e;
      color: #fff;
      font-size: 14px;
    }
    .form-group input:focus, .form-group select:focus {
      outline: none;
      border-color: #3b82f6;
    }
    .modal-actions {
      display: flex;
      gap: 12px;
      justify-content: flex-end;
      margin-top: 20px;
    }
    .error-message {
      background: #7f1d1d;
      border: 1px solid #dc2626;
      border-radius: 6px;
      padding: 12px;
      margin-bottom: 16px;
      color: #fca5a5;
    }
    .success-message {
      background: #14532d;
      border: 1px solid #16a34a;
      border-radius: 6px;
      padding: 12px;
      margin-bottom: 16px;
      color: #86efac;
    }
    .empty-state {
      text-align: center;
      padding: 60px 20px;
      color: #888;
    }
    .empty-state h2 {
      font-size: 20px;
      margin-bottom: 12px;
      color: #aaa;
    }
    .config-path {
      font-size: 12px;
      color: #666;
      margin-top: 24px;
      padding-top: 16px;
      border-top: 1px solid #3a3a5a;
    }
    .config-path code {
      background: #1a1a2e;
      padding: 2px 6px;
      border-radius: 4px;
      font-family: 'Consolas', monospace;
    }
  </style>
</head>
<body>
  <h1>BladeBlaid Profiles</h1>

  <div class="toolbar">
    <button class="btn btn-primary" onclick="showCreateModal('desktop')">
      + Create Desktop Profile
    </button>
    <button class="btn btn-primary" onclick="showCreateModal('mobile')">
      + Create Mobile Profile
    </button>
    <button class="btn" onclick="refreshProfiles()">
      Refresh
    </button>
  </div>

  <div id="message-container"></div>

  <div id="profiles-container" class="profiles-grid"></div>

  <div class="config-path">
    Config file: <code id="config-path">%LOCALAPPDATA%\BladeBlaid\profiles.json</code>
  </div>

  <!-- Create Profile Modal -->
  <div id="create-modal" class="modal">
    <div class="modal-content">
      <h2 class="modal-title">Create New Profile</h2>
      <div class="form-group">
        <label for="profile-id">Profile ID (alphanumeric, underscores only)</label>
        <input type="text" id="profile-id" placeholder="e.g., work_profile_01" pattern="[a-zA-Z0-9_]+">
      </div>
      <div class="form-group">
        <label for="profile-label">Display Label</label>
        <input type="text" id="profile-label" placeholder="e.g., Work Profile">
      </div>
      <div class="form-group">
        <label for="profile-type">Profile Type</label>
        <select id="profile-type">
          <option value="desktop">Desktop (Chrome Windows)</option>
          <option value="mobile">Mobile (Chrome Android Emulation)</option>
        </select>
      </div>
      <div class="modal-actions">
        <button class="btn" onclick="hideCreateModal()">Cancel</button>
        <button class="btn btn-success" onclick="createProfile()">Create</button>
      </div>
    </div>
  </div>

  <!-- Delete Confirmation Modal -->
  <div id="delete-modal" class="modal">
    <div class="modal-content">
      <h2 class="modal-title">Delete Profile</h2>
      <p style="margin-bottom: 16px; color: #ccc;">
        Are you sure you want to delete profile "<span id="delete-profile-label"></span>"?
      </p>
      <div class="form-group">
        <label style="display: flex; align-items: center; gap: 8px; cursor: pointer;">
          <input type="checkbox" id="delete-data-checkbox" checked style="width: auto;">
          Also delete profile data directory
        </label>
      </div>
      <div class="modal-actions">
        <button class="btn" onclick="hideDeleteModal()">Cancel</button>
        <button class="btn btn-danger" onclick="confirmDelete()">Delete</button>
      </div>
    </div>
  </div>

  <script>
    let profiles = [];
    let profileStatuses = {};
    let deleteProfileId = null;

    // Initialize
    document.addEventListener('DOMContentLoaded', () => {
      refreshProfiles();
    });

    function refreshProfiles() {
      chrome.send('getProfiles');
    }

    // Called from C++ with profiles data
    function onProfilesReceived(data) {
      profiles = data.profiles || [];
      if (data.config_path) {
        document.getElementById('config-path').textContent = data.config_path;
      }
      renderProfiles();

      // Fetch status for each profile
      profiles.forEach(p => {
        chrome.send('getProfileStatus', [p.profile_id]);
      });
    }

    // Called from C++ with profile status
    function onProfileStatus(profileId, exists) {
      profileStatuses[profileId] = exists;
      renderProfiles();
    }

    // Called from C++ on operation result
    function onOperationResult(success, message) {
      showMessage(message, success);
      refreshProfiles();
    }

    function showMessage(message, isSuccess) {
      const container = document.getElementById('message-container');
      const className = isSuccess ? 'success-message' : 'error-message';
      container.innerHTML = '<div class="' + className + '">' + escapeHtml(message) + '</div>';
      setTimeout(() => { container.innerHTML = ''; }, 5000);
    }

    function escapeHtml(text) {
      const div = document.createElement('div');
      div.textContent = text;
      return div.innerHTML;
    }

    function renderProfiles() {
      const container = document.getElementById('profiles-container');

      if (profiles.length === 0) {
        container.innerHTML =
          '<div class="empty-state">' +
            '<h2>No Profiles</h2>' +
            '<p>Create a new profile to get started.</p>' +
          '</div>';
        return;
      }

      let html = '';
      for (const profile of profiles) {
        const exists = profileStatuses[profile.profile_id] || false;
        const isMobile = profile.type === 'mobile';

        html +=
          '<div class="profile-card">' +
            '<div class="profile-header">' +
              '<span class="profile-label">' + escapeHtml(profile.label) + '</span>' +
              '<span class="profile-type ' + (isMobile ? 'mobile' : '') + '">' + profile.type + '</span>' +
            '</div>' +
            '<div class="profile-id">' + escapeHtml(profile.profile_id) + '</div>' +
            '<div class="profile-info">' +
              '<strong>Search:</strong> ' + escapeHtml(profile.search_engine || 'bing') +
            '</div>';

        if (profile.identity && profile.identity.email_label) {
          html += '<div class="profile-info">' +
              '<strong>Label:</strong> ' + escapeHtml(profile.identity.email_label) +
            '</div>';
        }

        const screenWidth = profile.fingerprint?.screen?.width || 1920;
        const screenHeight = profile.fingerprint?.screen?.height || 1080;
        const language = profile.fingerprint?.locale?.language || 'en-US';

        html +=
            '<div class="profile-info">' +
              '<strong>Screen:</strong> ' + screenWidth + 'x' + screenHeight +
            '</div>' +
            '<div class="profile-info">' +
              '<strong>Locale:</strong> ' + escapeHtml(language) +
            '</div>' +
            '<div class="profile-status">' +
              '<span class="status-dot ' + (exists ? 'exists' : '') + '"></span>' +
              '<span>' + (exists ? 'Data directory exists' : 'No data directory') + '</span>' +
            '</div>' +
            '<div class="profile-actions">' +
              '<button class="btn btn-success" onclick="launchProfile(\'' + escapeHtml(profile.profile_id) + '\')">' +
                'Launch' +
              '</button>' +
              '<button class="btn btn-danger" onclick="showDeleteModal(\'' + escapeHtml(profile.profile_id) + '\', \'' + escapeHtml(profile.label) + '\')">' +
                'Delete' +
              '</button>' +
            '</div>' +
          '</div>';
      }

      container.innerHTML = html;
    }

    function showCreateModal(type) {
      document.getElementById('profile-type').value = type;
      document.getElementById('profile-id').value = '';
      document.getElementById('profile-label').value = '';
      document.getElementById('create-modal').classList.add('show');
      document.getElementById('profile-id').focus();
    }

    function hideCreateModal() {
      document.getElementById('create-modal').classList.remove('show');
    }

    function createProfile() {
      const profileId = document.getElementById('profile-id').value.trim();
      const label = document.getElementById('profile-label').value.trim();
      const type = document.getElementById('profile-type').value;

      if (!profileId) {
        showMessage('Profile ID is required', false);
        return;
      }

      if (!/^[a-zA-Z0-9_]+$/.test(profileId)) {
        showMessage('Profile ID must contain only letters, numbers, and underscores', false);
        return;
      }

      chrome.send('createProfile', [profileId, type, label || profileId]);
      hideCreateModal();
    }

    function launchProfile(profileId) {
      chrome.send('launchProfile', [profileId]);
    }

    function showDeleteModal(profileId, label) {
      deleteProfileId = profileId;
      document.getElementById('delete-profile-label').textContent = label;
      document.getElementById('delete-data-checkbox').checked = true;
      document.getElementById('delete-modal').classList.add('show');
    }

    function hideDeleteModal() {
      document.getElementById('delete-modal').classList.remove('show');
      deleteProfileId = null;
    }

    function confirmDelete() {
      if (!deleteProfileId) return;

      const deleteData = document.getElementById('delete-data-checkbox').checked;
      chrome.send('deleteProfile', [deleteProfileId, deleteData]);
      hideDeleteModal();
    }

    // Handle Enter key in create modal
    document.getElementById('profile-id').addEventListener('keyup', function(e) {
      if (e.key === 'Enter') createProfile();
    });
    document.getElementById('profile-label').addEventListener('keyup', function(e) {
      if (e.key === 'Enter') createProfile();
    });

    // Close modals on escape
    document.addEventListener('keyup', function(e) {
      if (e.key === 'Escape') {
        hideCreateModal();
        hideDeleteModal();
      }
    });

    // Close modals on backdrop click
    document.querySelectorAll('.modal').forEach(function(modal) {
      modal.addEventListener('click', function(e) {
        if (e.target === modal) {
          hideCreateModal();
          hideDeleteModal();
        }
      });
    });
  </script>
</body>
</html>
)html";

// Custom URL data source for serving embedded HTML
class ProfilesLiteDataSource : public content::URLDataSource {
 public:
  ProfilesLiteDataSource() = default;
  ~ProfilesLiteDataSource() override = default;

  // content::URLDataSource implementation:
  std::string GetSource() override { return "profiles-lite"; }

  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override {
    std::string html(kProfilesLiteHTML);
    std::move(callback).Run(
        base::MakeRefCounted<base::RefCountedString>(std::move(html)));
  }

  std::string GetMimeType(const GURL& url) override {
    return "text/html";
  }

  bool ShouldServeMimeTypeAsContentTypeHeader() override { return true; }

  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) override {
    if (directive == network::mojom::CSPDirectiveName::ScriptSrc) {
      return "script-src 'self' 'unsafe-inline';";
    }
    return content::URLDataSource::GetContentSecurityPolicy(directive);
  }
};

}  // namespace

std::string GetProfilesLiteHTML() {
  return kProfilesLiteHTML;
}

// ProfilesLiteUI implementation

ProfilesLiteUI::ProfilesLiteUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Create and add the message handler
  web_ui->AddMessageHandler(std::make_unique<ProfilesLiteHandler>());

  // Add URL data source for serving embedded HTML
  Profile* profile = Profile::FromWebUI(web_ui);
  content::URLDataSource::Add(profile,
                              std::make_unique<ProfilesLiteDataSource>());
}

ProfilesLiteUI::~ProfilesLiteUI() = default;

// ProfilesLiteHandler implementation

ProfilesLiteHandler::ProfilesLiteHandler() = default;
ProfilesLiteHandler::~ProfilesLiteHandler() = default;

void ProfilesLiteHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getProfiles",
      base::BindRepeating(&ProfilesLiteHandler::HandleGetProfiles,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "launchProfile",
      base::BindRepeating(&ProfilesLiteHandler::HandleLaunchProfile,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "createProfile",
      base::BindRepeating(&ProfilesLiteHandler::HandleCreateProfile,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "deleteProfile",
      base::BindRepeating(&ProfilesLiteHandler::HandleDeleteProfile,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getProfileStatus",
      base::BindRepeating(&ProfilesLiteHandler::HandleGetProfileStatus,
                          base::Unretained(this)));
}

void ProfilesLiteHandler::HandleGetProfiles(const base::Value::List& args) {
  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();

  base::Value::Dict result;
  base::Value::List profiles_list;

  for (const auto& profile : service->GetProfiles()) {
    base::Value::Dict profile_dict;
    profile_dict.Set("profile_id", profile.profile_id);
    profile_dict.Set("type", profile.type == ProfileType::kMobile ? "mobile" : "desktop");
    profile_dict.Set("label", profile.label);
    profile_dict.Set("search_engine", profile.search_engine);

    // Identity (label only, no credentials)
    base::Value::Dict identity;
    identity.Set("uses_microsoft_account", profile.identity.uses_microsoft_account);
    identity.Set("email_label", profile.identity.email_label);
    profile_dict.Set("identity", std::move(identity));

    // Fingerprint summary
    base::Value::Dict fingerprint;
    base::Value::Dict screen;
    screen.Set("width", profile.fingerprint.screen.width);
    screen.Set("height", profile.fingerprint.screen.height);
    fingerprint.Set("screen", std::move(screen));

    base::Value::Dict locale;
    locale.Set("language", profile.fingerprint.locale.language);
    locale.Set("timezone", profile.fingerprint.locale.timezone);
    fingerprint.Set("locale", std::move(locale));

    profile_dict.Set("fingerprint", std::move(fingerprint));

    profiles_list.Append(std::move(profile_dict));
  }

  result.Set("profiles", std::move(profiles_list));
  result.Set("config_path", service->GetConfigFilePath().AsUTF8Unsafe());

  // Send to JavaScript
  web_ui()->CallJavascriptFunctionUnsafe("onProfilesReceived",
                                          base::Value(std::move(result)));
}

void ProfilesLiteHandler::HandleLaunchProfile(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string()) {
    return;
  }

  std::string profile_id = args[0].GetString();
  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();

  const BladeBlaidProfile* profile = service->GetProfileById(profile_id);
  if (!profile) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Profile not found: " + profile_id));
    return;
  }

  // Ensure profile directory exists
  service->CreateProfileDirectory(profile_id);

  // Get profile data directory
  base::FilePath profile_dir = service->GetProfileDataDir(profile_id);
  if (profile_dir.empty()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Could not determine profile directory"));
    return;
  }

  // Launch new browser instance with this profile
#if BUILDFLAG(IS_WIN)
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path)) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Could not get executable path"));
    return;
  }

  base::CommandLine cmd(exe_path);
  cmd.AppendSwitchPath("user-data-dir", profile_dir);
  cmd.AppendSwitchASCII("bladeblaid-profile", profile_id);

  // Pass profile type for mobile emulation
  if (profile->type == ProfileType::kMobile) {
    cmd.AppendSwitch("bladeblaid-mobile");
  }

  base::LaunchOptions options;
  options.start_hidden = false;

  base::LaunchProcess(cmd, options);

  web_ui()->CallJavascriptFunctionUnsafe(
      "onOperationResult",
      base::Value(true),
      base::Value("Launched profile: " + profile->label));
#else
  web_ui()->CallJavascriptFunctionUnsafe(
      "onOperationResult",
      base::Value(false),
      base::Value("Profile launch only supported on Windows"));
#endif
}

void ProfilesLiteHandler::HandleCreateProfile(const base::Value::List& args) {
  if (args.size() < 3 || !args[0].is_string() || !args[1].is_string() ||
      !args[2].is_string()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Invalid arguments"));
    return;
  }

  std::string profile_id = args[0].GetString();
  std::string type_str = args[1].GetString();
  std::string label = args[2].GetString();

  ProfileType type = (type_str == "mobile") ? ProfileType::kMobile
                                            : ProfileType::kDesktop;

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();

  if (service->CreateProfile(profile_id, type, label)) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(true),
        base::Value("Profile created: " + label));
  } else {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Failed to create profile. ID may already exist or be invalid."));
  }
}

void ProfilesLiteHandler::HandleDeleteProfile(const base::Value::List& args) {
  if (args.size() < 2 || !args[0].is_string() || !args[1].is_bool()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Invalid arguments"));
    return;
  }

  std::string profile_id = args[0].GetString();
  bool delete_data = args[1].GetBool();

  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();

  if (service->DeleteProfile(profile_id, delete_data)) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(true),
        base::Value("Profile deleted"));
  } else {
    web_ui()->CallJavascriptFunctionUnsafe(
        "onOperationResult",
        base::Value(false),
        base::Value("Failed to delete profile"));
  }
}

void ProfilesLiteHandler::HandleGetProfileStatus(const base::Value::List& args) {
  if (args.empty() || !args[0].is_string()) {
    return;
  }

  std::string profile_id = args[0].GetString();
  BladeBlaidProfileService* service = BladeBlaidProfileService::GetInstance();

  bool exists = service->ProfileDirectoryExists(profile_id);

  web_ui()->CallJavascriptFunctionUnsafe(
      "onProfileStatus",
      base::Value(profile_id),
      base::Value(exists));
}

}  // namespace bladeblaid
