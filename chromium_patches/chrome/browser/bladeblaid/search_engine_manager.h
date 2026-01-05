// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_SEARCH_ENGINE_MANAGER_H_
#define CHROME_BROWSER_BLADEBLAID_SEARCH_ENGINE_MANAGER_H_

#include <string>

class Profile;

namespace bladeblaid {

// Search engine identifiers
enum class SearchEngineType {
  kBing,
  kGoogle,
  kDuckDuckGo,
  kYahoo,
  kEcosia,
  kStartpage,
  kCustom
};

// Manages default search engine for BladeBlaid profiles.
class SearchEngineManager {
 public:
  SearchEngineManager();
  ~SearchEngineManager();

  // Set the default search engine for a profile.
  // |engine_name| should be "bing", "google", "duckduckgo", etc.
  void SetDefaultSearchEngine(Profile* chrome_profile,
                              const std::string& engine_name);

  // Set Bing as the default search engine.
  void SetBingAsDefault(Profile* chrome_profile);

  // Get the current default search engine name.
  std::string GetDefaultSearchEngineName(Profile* chrome_profile) const;

  // Parse engine name string to enum.
  static SearchEngineType ParseEngineName(const std::string& name);

 private:
  // Get the prepopulated search engine ID for a search engine type.
  int GetPrepopulatedId(SearchEngineType type) const;
};

// Set default search engine from BladeBlaid profile config.
void ApplyBladeBlaidSearchEngine(Profile* chrome_profile,
                                  const std::string& engine_name);

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_SEARCH_ENGINE_MANAGER_H_
