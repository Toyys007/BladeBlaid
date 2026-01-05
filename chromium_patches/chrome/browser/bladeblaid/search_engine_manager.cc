// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/search_engine_manager.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"

namespace bladeblaid {

namespace {

// Prepopulated search engine IDs from Chromium's prepopulate data.
// These are defined in components/search_engines/prepopulated_engines.json
// Note: IDs may vary by region; these are common ones.
constexpr int kBingPrepopulatedId = 3;      // Bing
constexpr int kGooglePrepopulatedId = 1;    // Google
constexpr int kYahooPrepopulatedId = 2;     // Yahoo
constexpr int kDuckDuckGoPrepopulatedId = 92; // DuckDuckGo
constexpr int kEcosiaPrepopulatedId = 101;  // Ecosia

}  // namespace

SearchEngineManager::SearchEngineManager() = default;
SearchEngineManager::~SearchEngineManager() = default;

void SearchEngineManager::SetDefaultSearchEngine(Profile* chrome_profile,
                                                  const std::string& engine_name) {
  if (!chrome_profile) {
    LOG(ERROR) << "BladeBlaid: Cannot set search engine - null profile";
    return;
  }

  SearchEngineType type = ParseEngineName(engine_name);

  if (type == SearchEngineType::kBing) {
    SetBingAsDefault(chrome_profile);
    return;
  }

  TemplateURLService* service =
      TemplateURLServiceFactory::GetForProfile(chrome_profile);

  if (!service) {
    LOG(WARNING) << "BladeBlaid: TemplateURLService not available";
    return;
  }

  // Wait for service to be loaded
  if (!service->loaded()) {
    // Service not yet loaded - it will be configured on first use
    LOG(INFO) << "BladeBlaid: TemplateURLService not loaded yet, "
              << "search engine will be set when service loads";
    return;
  }

  int prepopulated_id = GetPrepopulatedId(type);
  if (prepopulated_id == 0) {
    LOG(WARNING) << "BladeBlaid: Unknown search engine: " << engine_name;
    return;
  }

  // Find the template URL by prepopulated ID
  TemplateURL* template_url = nullptr;
  TemplateURLService::TemplateURLVector urls = service->GetTemplateURLs();

  for (TemplateURL* url : urls) {
    if (url->prepopulate_id() == prepopulated_id) {
      template_url = url;
      break;
    }
  }

  if (template_url) {
    service->SetUserSelectedDefaultSearchProvider(template_url);
    LOG(INFO) << "BladeBlaid: Set default search engine to: "
              << template_url->short_name();
  } else {
    LOG(WARNING) << "BladeBlaid: Search engine not found with ID: "
                 << prepopulated_id;
  }
}

void SearchEngineManager::SetBingAsDefault(Profile* chrome_profile) {
  if (!chrome_profile) {
    return;
  }

  TemplateURLService* service =
      TemplateURLServiceFactory::GetForProfile(chrome_profile);

  if (!service) {
    LOG(WARNING) << "BladeBlaid: TemplateURLService not available for Bing setup";
    return;
  }

  if (!service->loaded()) {
    LOG(INFO) << "BladeBlaid: Waiting for TemplateURLService to load for Bing setup";
    return;
  }

  // Find Bing in prepopulated engines
  TemplateURL* bing_url = nullptr;
  TemplateURLService::TemplateURLVector urls = service->GetTemplateURLs();

  for (TemplateURL* url : urls) {
    if (url->prepopulate_id() == kBingPrepopulatedId) {
      bing_url = url;
      break;
    }

    // Also check by keyword/name as backup
    std::string keyword = base::ToLowerASCII(base::UTF16ToUTF8(url->keyword()));
    std::string short_name = base::ToLowerASCII(base::UTF16ToUTF8(url->short_name()));

    if (keyword.find("bing") != std::string::npos ||
        short_name.find("bing") != std::string::npos) {
      bing_url = url;
      break;
    }
  }

  if (bing_url) {
    service->SetUserSelectedDefaultSearchProvider(bing_url);
    LOG(INFO) << "BladeBlaid: Bing set as default search engine";
  } else {
    // Bing not found in prepopulated list - create custom entry
    LOG(INFO) << "BladeBlaid: Bing not in prepopulated list, creating custom entry";

    TemplateURLData bing_data;
    bing_data.SetShortName(u"Bing");
    bing_data.SetKeyword(u"bing.com");
    bing_data.SetURL("https://www.bing.com/search?q={searchTerms}");
    bing_data.suggestions_url = "https://www.bing.com/osjson.aspx?query={searchTerms}";
    bing_data.image_url = "https://www.bing.com/images/search?q={searchTerms}";
    bing_data.favicon_url = GURL("https://www.bing.com/favicon.ico");
    bing_data.prepopulate_id = kBingPrepopulatedId;
    bing_data.safe_for_autoreplace = false;

    TemplateURL* new_bing = service->Add(std::make_unique<TemplateURL>(bing_data));
    if (new_bing) {
      service->SetUserSelectedDefaultSearchProvider(new_bing);
      LOG(INFO) << "BladeBlaid: Created and set Bing as default search engine";
    }
  }
}

std::string SearchEngineManager::GetDefaultSearchEngineName(
    Profile* chrome_profile) const {
  if (!chrome_profile) {
    return "unknown";
  }

  TemplateURLService* service =
      TemplateURLServiceFactory::GetForProfile(chrome_profile);

  if (!service || !service->loaded()) {
    return "unknown";
  }

  const TemplateURL* default_url = service->GetDefaultSearchProvider();
  if (!default_url) {
    return "none";
  }

  return base::UTF16ToUTF8(default_url->short_name());
}

SearchEngineType SearchEngineManager::ParseEngineName(const std::string& name) {
  std::string lower_name = base::ToLowerASCII(name);

  if (lower_name == "bing" || lower_name == "microsoft bing") {
    return SearchEngineType::kBing;
  }
  if (lower_name == "google") {
    return SearchEngineType::kGoogle;
  }
  if (lower_name == "duckduckgo" || lower_name == "ddg") {
    return SearchEngineType::kDuckDuckGo;
  }
  if (lower_name == "yahoo") {
    return SearchEngineType::kYahoo;
  }
  if (lower_name == "ecosia") {
    return SearchEngineType::kEcosia;
  }
  if (lower_name == "startpage") {
    return SearchEngineType::kStartpage;
  }

  return SearchEngineType::kCustom;
}

int SearchEngineManager::GetPrepopulatedId(SearchEngineType type) const {
  switch (type) {
    case SearchEngineType::kBing:
      return kBingPrepopulatedId;
    case SearchEngineType::kGoogle:
      return kGooglePrepopulatedId;
    case SearchEngineType::kYahoo:
      return kYahooPrepopulatedId;
    case SearchEngineType::kDuckDuckGo:
      return kDuckDuckGoPrepopulatedId;
    case SearchEngineType::kEcosia:
      return kEcosiaPrepopulatedId;
    case SearchEngineType::kStartpage:
    case SearchEngineType::kCustom:
    default:
      return 0;
  }
}

void ApplyBladeBlaidSearchEngine(Profile* chrome_profile,
                                  const std::string& engine_name) {
  if (engine_name.empty()) {
    // Default to Bing if not specified
    SearchEngineManager manager;
    manager.SetBingAsDefault(chrome_profile);
    return;
  }

  SearchEngineManager manager;
  manager.SetDefaultSearchEngine(chrome_profile, engine_name);
}

}  // namespace bladeblaid
