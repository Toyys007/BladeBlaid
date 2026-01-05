// Copyright 2024 The BladeBlaid Authors
// Use of this source code is governed by a BSD-style license.

#include "chrome/browser/bladeblaid/tracker_blocker.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace bladeblaid {

namespace {

// Singleton instance
TrackerBlocker* g_tracker_blocker = nullptr;

// Known tracker domains - MVP list of major trackers.
// This is intentionally conservative to avoid breaking sites.
// Full tracker lists (like EasyList) would be too aggressive for MVP.
const char* const kTrackerDomains[] = {
    // Google Analytics & Ads
    "google-analytics.com",
    "googleadservices.com",
    "googlesyndication.com",
    "googletagmanager.com",
    "googletagservices.com",
    "doubleclick.net",
    "2mdn.net",
    "admob.com",
    "app-measurement.com",

    // Facebook
    "facebook.net",
    "fbcdn.net",
    "connect.facebook.com",
    "pixel.facebook.com",

    // Twitter
    "analytics.twitter.com",
    "ads-twitter.com",
    "static.ads-twitter.com",

    // Microsoft (tracking only, not Bing search)
    "bat.bing.com",
    "clarity.ms",

    // Amazon
    "amazon-adsystem.com",
    "assoc-amazon.com",

    // Adobe
    "demdex.net",
    "omtrdc.net",
    "2o7.net",
    "everesttech.net",

    // Criteo
    "criteo.com",
    "criteo.net",

    // Other ad networks
    "adnxs.com",
    "adsrvr.org",
    "adroll.com",
    "bidswitch.net",
    "casalemedia.com",
    "contextweb.com",
    "dotomi.com",
    "lijit.com",
    "mathtag.com",
    "mediamath.com",
    "moatads.com",
    "openx.net",
    "outbrain.com",
    "pubmatic.com",
    "quantserve.com",
    "rfihub.com",
    "rlcdn.com",
    "rubiconproject.com",
    "scorecardresearch.com",
    "sharethrough.com",
    "simpli.fi",
    "spotxchange.com",
    "taboola.com",
    "tapad.com",
    "teads.tv",
    "tremorhub.com",
    "tribalfusion.com",
    "turn.com",
    "yieldmo.com",

    // Analytics platforms
    "hotjar.com",
    "newrelic.com",
    "nr-data.net",
    "segment.com",
    "segment.io",
    "mixpanel.com",
    "amplitude.com",
    "heapanalytics.com",
    "fullstory.com",
    "mouseflow.com",
    "crazyegg.com",
    "clicktale.net",
    "luckyorange.com",
    "inspectlet.com",

    // Fingerprinting / bot detection (blocking may cause issues on some sites)
    // "fingerprintjs.com",  // Excluded - may break legitimate use
    // "perimeterx.net",     // Excluded - may break legitimate use

    // Marketing automation
    "marketo.net",
    "marketo.com",
    "pardot.com",
    "eloqua.com",
    "hubspot.com",  // Note: may break embedded forms
    "hs-analytics.net",
    "hsforms.net",

    // Social widgets (tracking aspect)
    "addthis.com",
    "addtoany.com",
    "sharethis.com",

    // Other trackers
    "bluekai.com",
    "bkrtx.com",
    "exelator.com",
    "eyeota.net",
    "krxd.net",
    "liveramp.com",
    "liadm.com",
    "intentiq.com",
    "intentmedia.net",
};

// Known tracking beacon/pixel paths
const char* const kBeaconPaths[] = {
    "/pixel",
    "/pixel.gif",
    "/pixel.png",
    "/beacon",
    "/beacon.gif",
    "/track",
    "/track.gif",
    "/1x1.gif",
    "/clear.gif",
    "/spacer.gif",
    "/transparent.gif",
    "/blank.gif",
    "/analytics",
    "/collect",
    "/__utm.gif",
    "/r/collect",
    "/j/collect",
    "/pagead/",
    "/pcs/view",
    "/ads/",
};

}  // namespace

TrackerBlocker::TrackerBlocker() {
  InitializeTrackerDomains();
}

TrackerBlocker::~TrackerBlocker() = default;

void TrackerBlocker::InitializeTrackerDomains() {
  for (const char* domain : kTrackerDomains) {
    tracker_domains_.insert(base::ToLowerASCII(domain));
  }

  for (const char* path : kBeaconPaths) {
    beacon_paths_.insert(base::ToLowerASCII(path));
  }
}

bool TrackerBlocker::ShouldBlockRequest(const GURL& request_url,
                                        const GURL& page_url,
                                        ResourceType resource_type) const {
  if (!enabled_ || !request_url.is_valid()) {
    return false;
  }

  // Allow first-party requests if configured
  if (allow_first_party_ && IsSameParty(request_url, page_url)) {
    return false;
  }

  // Check if domain is a known tracker
  if (IsTrackerDomain(request_url.host())) {
    VLOG(1) << "BladeBlaid: Blocking tracker domain: " << request_url.host();
    return true;
  }

  // Check for tracking beacons (third-party small images/scripts)
  if (IsTrackingBeacon(request_url, resource_type)) {
    // Only block third-party beacons
    if (!IsSameParty(request_url, page_url)) {
      VLOG(1) << "BladeBlaid: Blocking tracking beacon: " << request_url.spec();
      return true;
    }
  }

  return false;
}

bool TrackerBlocker::IsTrackerDomain(const std::string& domain) const {
  std::string lower_domain = base::ToLowerASCII(domain);

  // Direct match
  if (tracker_domains_.count(lower_domain) > 0) {
    return true;
  }

  // Check if it's a subdomain of a tracker
  size_t dot_pos = lower_domain.find('.');
  while (dot_pos != std::string::npos) {
    std::string parent = lower_domain.substr(dot_pos + 1);
    if (tracker_domains_.count(parent) > 0) {
      return true;
    }
    dot_pos = lower_domain.find('.', dot_pos + 1);
  }

  return false;
}

bool TrackerBlocker::IsTrackingBeacon(const GURL& url,
                                       ResourceType resource_type) const {
  // Only check images and XHR for beacon patterns
  if (resource_type != ResourceType::kImage &&
      resource_type != ResourceType::kXhr) {
    return false;
  }

  std::string path = base::ToLowerASCII(url.path());

  // Check exact path matches
  if (beacon_paths_.count(path) > 0) {
    return true;
  }

  // Check path prefixes/suffixes
  for (const auto& beacon_path : beacon_paths_) {
    if (path.find(beacon_path) != std::string::npos) {
      return true;
    }
  }

  // Check for 1x1 images (common tracking pixels)
  if (resource_type == ResourceType::kImage) {
    // Check for common pixel naming patterns
    if (path.find("1x1") != std::string::npos ||
        path.find("pixel") != std::string::npos ||
        path.find("beacon") != std::string::npos ||
        path.find("tracking") != std::string::npos) {
      return true;
    }
  }

  return false;
}

bool TrackerBlocker::IsSameParty(const GURL& url1, const GURL& url2) const {
  if (!url1.is_valid() || !url2.is_valid()) {
    return false;
  }

  return GetETLDPlusOne(url1) == GetETLDPlusOne(url2);
}

std::string TrackerBlocker::GetETLDPlusOne(const GURL& url) const {
  if (!url.is_valid()) {
    return std::string();
  }

  return net::registry_controlled_domains::GetDomainAndRegistry(
      url,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

TrackerBlocker* GetTrackerBlocker() {
  if (!g_tracker_blocker) {
    g_tracker_blocker = new TrackerBlocker();
  }
  return g_tracker_blocker;
}

}  // namespace bladeblaid
