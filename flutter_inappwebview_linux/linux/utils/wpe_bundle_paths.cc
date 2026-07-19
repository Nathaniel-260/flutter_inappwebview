#include "wpe_bundle_paths.h"

// dladdr is a GNU extension declared only under _GNU_SOURCE.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <glib.h>

#include <string>

#include "log.h"

namespace flutter_inappwebview_plugin {

namespace {

// Directory that contains this plugin's shared library. Resolved via dladdr so
// it is correct regardless of the working directory the app is launched from.
std::string PluginBundleDirectory() {
  Dl_info info;
  if (dladdr(reinterpret_cast<void*>(&PluginBundleDirectory), &info) == 0 ||
      info.dli_fname == nullptr) {
    return {};
  }
  gchar* dir = g_path_get_dirname(info.dli_fname);
  std::string result = dir ? dir : "";
  g_free(dir);
  return result;
}

bool FileExists(const std::string& path) {
  return g_file_test(path.c_str(), G_FILE_TEST_EXISTS);
}

// Prepend dir to LD_LIBRARY_PATH so spawned helper processes resolve the bundled
// libWPEWebKit; their own RPATH points at the (absent) system install dir.
void PrependLibraryPath(const std::string& dir) {
  const char* current = g_getenv("LD_LIBRARY_PATH");
  std::string value = current && *current ? dir + ":" + current : dir;
  g_setenv("LD_LIBRARY_PATH", value.c_str(), TRUE);
}

}  // namespace

void ConfigureBundledWebKitPaths() {
  const std::string bundleDir = PluginBundleDirectory();
  if (bundleDir.empty()) {
    return;
  }

  // Helper processes: both are required, so only redirect when both are present.
  const std::string webProcess = bundleDir + "/WPEWebProcess";
  const std::string networkProcess = bundleDir + "/WPENetworkProcess";
  if (FileExists(webProcess) && FileExists(networkProcess)) {
    // overwrite=FALSE: respect an explicit developer override.
    g_setenv("WEBKIT_EXEC_PATH", bundleDir.c_str(), FALSE);
    PrependLibraryPath(bundleDir);
    debugLog("Using bundled WPE helper processes from " + bundleDir);
  }

  // Injected bundle: WEBKIT_INJECTED_BUNDLE_PATH is a directory, honored by all builds.
  if (FileExists(bundleDir + "/libWPEInjectedBundle.so")) {
    g_setenv("WEBKIT_INJECTED_BUNDLE_PATH", bundleDir.c_str(), FALSE);
    debugLog("Using bundled WPE injected bundle from " + bundleDir);
  }
}

}  // namespace flutter_inappwebview_plugin
