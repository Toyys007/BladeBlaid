// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_URL_SANITIZER_H_
#define CHROME_BROWSER_BLADEBLAID_URL_SANITIZER_H_

#include <string>
#include <vector>

#include "url/gurl.h"

namespace bladeblaid {

// Sanitizes URLs by removing known tracking parameters.
// This helps prevent cross-site tracking via URL decoration.
class URLSanitizer {
 public:
  URLSanitizer();
  ~URLSanitizer();

  // Sanitize a URL by removing tracking parameters.
  // Returns the sanitized URL, or the original if no changes needed.
  GURL SanitizeURL(const GURL& url) const;

  // Check if a URL has tracking parameters that would be removed.
  bool HasTrackingParams(const GURL& url) const;

  // Get the list of tracking parameters being filtered.
  const std::vector<std::string>& GetTrackingParams() const;

  // Add a custom tracking parameter to filter.
  void AddTrackingParam(const std::string& param);

  // Enable/disable sanitization.
  void SetEnabled(bool enabled) { enabled_ = enabled; }
  bool IsEnabled() const { return enabled_; }

 private:
  // Check if a parameter name is a tracking parameter.
  bool IsTrackingParam(const std::string& param_name) const;

  // Default tracking parameters to remove.
  std::vector<std::string> tracking_params_;

  bool enabled_ = true;
};

// Get the global URL sanitizer instance.
URLSanitizer* GetURLSanitizer();

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_URL_SANITIZER_H_
