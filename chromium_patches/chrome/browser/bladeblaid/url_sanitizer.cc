// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/url_sanitizer.h"

#include <algorithm>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/url_util.h"

namespace bladeblaid {

namespace {

// Singleton instance
URLSanitizer* g_url_sanitizer = nullptr;

// Known tracking parameters to remove.
// These are commonly used for cross-site tracking via URL decoration.
const char* const kDefaultTrackingParams[] = {
    // Google Analytics / Ads
    "utm_source",
    "utm_medium",
    "utm_campaign",
    "utm_term",
    "utm_content",
    "utm_id",
    "utm_source_platform",
    "utm_creative_format",
    "utm_marketing_tactic",

    // Google Click ID
    "gclid",
    "gclsrc",

    // Facebook Click ID
    "fbclid",

    // Microsoft/Bing Click ID
    "msclkid",

    // Yandex Click ID
    "yclid",
    "_ym_uid",
    "_ym_visorc",

    // TikTok Click ID
    "ttclid",

    // Twitter Click ID
    "twclid",

    // Mailchimp
    "mc_cid",
    "mc_eid",

    // HubSpot
    "hsa_acc",
    "hsa_cam",
    "hsa_grp",
    "hsa_ad",
    "hsa_src",
    "hsa_tgt",
    "hsa_kw",
    "hsa_mt",
    "hsa_net",
    "hsa_ver",

    // Generic referral tracking
    "ref",
    "ref_src",
    "ref_url",
    "referrer",

    // Adobe Analytics
    "s_kwcid",
    "ef_id",

    // Drip
    "__s",

    // Vero
    "vero_id",
    "vero_conv",

    // Marketo
    "mkt_tok",

    // Outbrain
    "obOrigUrl",

    // Wicked Reports
    "wickedid",

    // Other common trackers
    "igshid",      // Instagram
    "si",          // Spotify
    "_hsenc",      // HubSpot encoding
    "_hsmi",       // HubSpot email ID
    "trk",         // LinkedIn
    "trkInfo",     // LinkedIn
    "clickid",     // Generic
    "affiliate_id",
    "aff_id",
};

}  // namespace

URLSanitizer::URLSanitizer() {
  // Initialize with default tracking parameters
  for (const char* param : kDefaultTrackingParams) {
    tracking_params_.push_back(param);
  }
}

URLSanitizer::~URLSanitizer() = default;

GURL URLSanitizer::SanitizeURL(const GURL& url) const {
  if (!enabled_ || !url.is_valid() || !url.has_query()) {
    return url;
  }

  // Parse query string
  std::string query = url.query();
  std::vector<std::pair<std::string, std::string>> params;

  // Split query string into key-value pairs
  base::StringPairs query_pairs;
  base::SplitStringIntoKeyValuePairs(query, '=', '&', &query_pairs);

  bool modified = false;
  std::string new_query;

  for (const auto& pair : query_pairs) {
    if (!IsTrackingParam(pair.first)) {
      // Keep this parameter
      if (!new_query.empty()) {
        new_query += '&';
      }
      new_query += pair.first;
      if (!pair.second.empty()) {
        new_query += '=';
        new_query += pair.second;
      }
    } else {
      modified = true;
    }
  }

  if (!modified) {
    return url;
  }

  // Rebuild URL with sanitized query
  GURL::Replacements replacements;
  if (new_query.empty()) {
    replacements.ClearQuery();
  } else {
    replacements.SetQueryStr(new_query);
  }

  return url.ReplaceComponents(replacements);
}

bool URLSanitizer::HasTrackingParams(const GURL& url) const {
  if (!url.is_valid() || !url.has_query()) {
    return false;
  }

  std::string query = url.query();
  base::StringPairs query_pairs;
  base::SplitStringIntoKeyValuePairs(query, '=', '&', &query_pairs);

  for (const auto& pair : query_pairs) {
    if (IsTrackingParam(pair.first)) {
      return true;
    }
  }

  return false;
}

const std::vector<std::string>& URLSanitizer::GetTrackingParams() const {
  return tracking_params_;
}

void URLSanitizer::AddTrackingParam(const std::string& param) {
  // Avoid duplicates
  if (std::find(tracking_params_.begin(), tracking_params_.end(), param) ==
      tracking_params_.end()) {
    tracking_params_.push_back(param);
  }
}

bool URLSanitizer::IsTrackingParam(const std::string& param_name) const {
  // Case-insensitive comparison
  std::string lower_param = base::ToLowerASCII(param_name);

  for (const auto& tracking_param : tracking_params_) {
    if (base::ToLowerASCII(tracking_param) == lower_param) {
      return true;
    }
  }

  return false;
}

URLSanitizer* GetURLSanitizer() {
  if (!g_url_sanitizer) {
    g_url_sanitizer = new URLSanitizer();
  }
  return g_url_sanitizer;
}

}  // namespace bladeblaid
