/* Configure bootbootboot for reproducing
 * https://bugzilla.redhat.com/show_bug.cgi?id=2216496
 */

#define NAME_OF_TEST \
  "https://bugzilla.redhat.com/show_bug.cgi?id=2216496 using guestfish"

//#define QEMU "qemu-system-x86_64"
#define QEMU "/home/rjones/d/qemu/build/qemu-system-x86_64"

static void
set_up_environment (void)
{
  setenv ("LIBGUESTFS_HV", QEMU, 1);
  setenv ("LIBGUESTFS_BACKEND_SETTINGS", "force_tcg", 1);
  setenv ("LIBGUESTFS_BACKEND", "direct", 1);
}

static void
exec_test (void)
{
  execlp ("guestfish",
          "guestfish", "-a", "/dev/null", "-v",
          "set-smp", "4", ":", "run",
          NULL);
}

static enum test_status
check_test_status (const char *output, int test_hanged, int exit_status)
{
  char cmd[256];
  int r;

  if (test_hanged)
    return FAIL;

  if (WIFEXITED (exit_status) && WEXITSTATUS (exit_status) == 0)
    return PASS;

  /* Check for the bug */
  snprintf (cmd, sizeof cmd,
            "grep -s -q 'asm_exc_int3' %s",
            output);
  r = system (cmd);
  if (r == 0)
    return FAIL;

  /* Check for https://gitlab.com/qemu-project/qemu/-/issues/1738 */
  snprintf (cmd, sizeof cmd,
            "grep -s -q 'appliance closed the connection' %s",
            output);
  r = system (cmd);
  if (r == 0)
    return SKIP;

  /* Anything else needs further analysis. */
  return UNCLASSIFIED;
}
