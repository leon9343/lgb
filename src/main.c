#include <App/app.h>
#include <Emulator/cpu/cpu.h>
#include <llog.h>
#include "util.h"
#include <stdlib.h>

int main() {
  // App initialization
  ResultApp ra = app_create();
  if (result_App_is_err(&ra)) {
    LOG_ERROR("failed to create app: %s (%s)", ra.message, error_string(ra.error_code));
    return EXIT_FAILURE;
  }
  App app = result_App_get_data(&ra);

  // Running
  app_run(&app);

  // Cleanup
  app_destroy(&app);
  exit(EXIT_SUCCESS);
}
