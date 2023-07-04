/* Configure bootbootboot for reproducing
 * https://bugzilla.redhat.com/show_bug.cgi?id=2216496
 * using only qemu commands
 */

#define NAME_OF_TEST \
  "https://bugzilla.redhat.com/show_bug.cgi?id=2216496 using qemu"

#define QEMU "qemu-system-x86_64"
#define VMLINUX "/boot/vmlinuz-6.3.10-200.fc38.x86_64"

static void
set_up_environment (void)
{
  // nothing
}

static void
create_command_line (char *cmd, size_t len)
{
  snprintf (cmd, len,
            "%s "
            "-no-user-config -nodefaults -display none "
            "-machine accel=tcg,graphics=off -cpu max,la57=off -m 1280 "
            "-smp 4 "
            "-no-reboot "
            "-rtc driftfix=slew -no-hpet -global kvm-pit.lost_tick_policy=discard "
            "-kernel %s "
            "-serial file:/dev/stdout "
            "-append \"panic=1 console=ttyS0 edd=off udevtimeout=6000 udev.event-timeout=6000 no_timer_check printk.time=1 cgroup_disable=memory usbcore.nousb cryptomgr.notests tsc=reliable 8250.nr_uarts=1 selinux=0 TERM=xterm-256color\"",
            QEMU, VMLINUX
            );
}

static enum test_status
check_test_status (const char *output, int test_hanged, int exit_status)
{
  char cmd[256];
  int r;

  if (test_hanged)
    return FAIL;

  /* Check for expected output (no initramfs or root device). */
  snprintf (cmd, sizeof cmd,
            "grep -s -q 'VFS: Cannot open root device' %s",
            output);
  r = system (cmd);
  if (r == 0)
    return PASS;

  /* Check for the bug. */
  snprintf (cmd, sizeof cmd,
            "grep -s -q 'asm_exc_int3' %s",
            output);
  r = system (cmd);
  if (r == 0)
    return FAIL;

  /* Anything else needs further analysis. */
  return UNCLASSIFIED;
}
