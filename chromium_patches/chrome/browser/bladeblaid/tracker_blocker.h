// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#ifndef CHROME_BROWSER_BLADEBLAID_TRACKER_BLOCKER_H_
#define CHROME_BROWSER_BLADEBLAID_TRACKER_BLOCKER_H_

#include <set>
#include <string>

#include "url/gurl.h"

namespace bladeblaid {

// Resource types that can be blocked
enum class ResourceType {
  kScript,
  kImage,
  kXhr,
  kSubFrame,
  kOther
};

// Simple tracker blocker based on known tracking domains.
// This is an MVP implementation - not a full content blocker like uBlock.
// Blocks:
// - Known tracking domains (analytics, ad networks, etc.)
// - Third-party tracking pixels/beacons
// - Known fingerprinting scripts
class TrackerBlocker {
 public:
  TrackerBlocker();
  ~TrackerBlocker();

  // Check if a request should be blocked.
  // |request_url| - The URL being requested
  // |page_url| - The top-level page URL (for first/third party determination)
  // |resource_type| - Type of resource being requested
  bool ShouldBlockRequest(const GURL& request_url,
                          const GURL& page_url,
                          ResourceType resource_type) const;

  // Check if a domain is a known tracker.
  bool IsTrackerDomain(const std::string& domain) const;

  // Check if URL is a tracking pixel/beacon.
  bool IsTrackingBeacon(const GURL& url, ResourceType resource_type) const;

  // Enable/disable blocking.
  void SetEnabled(bool enabled) { enabled_ = enabled; }
  bool IsEnabled() const { return enabled_; }

  // Allow first-party requests from tracker domains
  // (e.g., visiting google-analytics.com directly)
  void SetAllowFirstParty(bool allow) { allow_first_party_ = allow; }

 private:
  // Initialize the tracker domain list.
  void InitializeTrackerDomains();

  // Check if two URLs are same-party (same eTLD+1).
  bool IsSameParty(const GURL& url1, const GURL& url2) const;

  // Get eTLD+1 for a URL.
  std::string GetETLDPlusOne(const GURL& url) const;

  // Set of known tracker domains (stored as lowercase).
  std::set<std::string> tracker_domains_;

  // Set of known tracking beacon paths.
  std::set<std::string> beacon_paths_;

  bool enabled_ = true;
  bool allow_first_party_ = true;
};

// Get the global tracker blocker instance.
TrackerBlocker* GetTrackerBlocker();

}  // namespace bladeblaid

#endif  // CHROME_BROWSER_BLADEBLAID_TRACKER_BLOCKER_H_
