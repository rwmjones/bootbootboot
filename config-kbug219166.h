/* Configure bootbootboot for reproducing
 * https://bugzilla.kernel.org/show_bug.cgi?id=219166
 */

#define NAME_OF_TEST \
  "https://bugzilla.kernel.org/show_bug.cgi?id=219166 using guestfish"

/* This test can sometimes run for longer than 60 seconds under load. */
#undef MAX_TIME
#define MAX_TIME 300

static void
set_up_environment (void)
{
  setenv ("LIBGUESTFS_BACKEND", "direct", 1);
  setenv ("LIBGUESTFS_BACKEND_SETTINGS", "force_tcg", 1);
}

static void
exec_test (void)
{
  execlp ("guestfish",
          "guestfish", "-a", "/dev/null", "-v",
          "run",
          NULL);
}

static enum test_status
check_test_status (const char *output, int test_hanged, int exit_status)
{
  char cmd[256];
  int r;

  /* We're looking for a hang, with the last line of output containing
   * "noop", but give it a few lines of slack.
   */
  if (test_hanged) {
    snprintf (cmd, sizeof cmd,
              "tail -5 %s | grep -s -q noop",
              output);
    r = system (cmd);
    if (r == 0)
      return FAIL;
  }

  /* If it ran to completion, test passed. */
  if (WIFEXITED (exit_status) && WEXITSTATUS (exit_status) == 0)
    return PASS;

  /* Anything else needs further analysis. */
  return UNCLASSIFIED;
}
