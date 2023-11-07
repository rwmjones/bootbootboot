/* Configure bootbootboot for reproducing
 * https://bugzilla.redhat.com/show_bug.cgi?id=2216496
 * using only qemu commands
 */

#define NAME_OF_TEST \
  "https://bugzilla.redhat.com/show_bug.cgi?id=2216496 using qemu"

//#define QEMU "qemu-system-x86_64"
#define QEMU "/home/rjones/d/qemu/build/qemu-system-x86_64"
#define VMLINUX "/home/rjones/d/linux/arch/x86/boot/bzImage"

static void
set_up_environment (void)
{
  // nothing
}

static void
exec_test (void)
{
  execlp (QEMU, QEMU,
          "-no-user-config", "-nodefaults", "-display", "none",
          "-machine", "accel=tcg,graphics=off", "-cpu", "max,la57=off,serialize=off",
          "-smp", "4",
          "-m", "1280",
          "-no-reboot",
          "-rtc", "driftfix=slew", "-no-hpet",
          "-global", "kvm-pit.lost_tick_policy=discard",
          "-kernel", VMLINUX,
          "-serial", "file:/dev/stdout",
          "-append", "panic=1 console=ttyS0 edd=off udevtimeout=6000 udev.event-timeout=6000 no_timer_check printk.time=1 cgroup_disable=memory usbcore.nousb cryptomgr.notests tsc=reliable 8250.nr_uarts=1 selinux=0 TERM=xterm-256color",
          NULL);
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
            "grep -s -q -E "
            "'VFS: (Cannot open root device|Unable to mount root fs)' %s",
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
