#include <gst/gst.h>

#include "nsAutoPtr.h"
#include "prlog.h"
#include "nsCCNxGSTHandler.h"

#if defined(PR_LOGGING)
extern PRLogModuleInfo* gCCNxLog;
#endif
#define LOG(args) PR_LOG(gCCNxLog, PR_LOG_DEBUG, args)

void test_gst (int argc, char *argv[]) {
  const gchar *nano_str;
  guint major, minor, micro, nano;

  gst_init (&argc, &argv);

  gst_version (&major, &minor, &micro, &nano);

  if (nano == 1)
    nano_str = "(CVS)";
  else if (nano == 2)
    nano_str = "(Prerelease)";
  else
    nano_str = "";

  LOG (("This program is linked against GStreamer %d.%d.%d %s\n",
        major, minor, micro, nano_str));
}
