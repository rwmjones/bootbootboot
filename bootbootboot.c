/* bootbootboot
 * Copyright (C) 2023 Richard W.M. Jones
 * Copyright (C) 2023 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

/* The number of boot iterations that must pass before we declare success. */
#define NR_ITERATIONS 5000

/* The maximum number of threads we will ever use.  However we limit
 * the actual number of threads to the number of online processors
 * (like 'nproc').
 */
#define MAX_THREADS 16

/* The maximum time before we declare a boot has hung (seconds). */
#define MAX_TIME 60

/* Each test can pass or fail, or skip if the result is another
 * failure, or unclassified which will exit requiring user attention.
 */
enum test_status { PASS, FAIL, SKIP, UNCLASSIFIED };

/* Include one of these configuration files only! */
//#include "config-bz2216496-guestfish.h"
#include "config-bz2216496-qemu.h"

/*----------------------------------------------------------------------*/

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned iterations, skips;

static void *start_thread (void *);

int
main (int argc, char *argv[])
{
  long n, i;
  int r;
  pthread_t thread[MAX_THREADS];

  set_up_environment ();

  n = sysconf (_SC_NPROCESSORS_ONLN);
  if (n == -1) error (EXIT_FAILURE, errno, "sysconf");

  if (n > MAX_THREADS)
    n = MAX_THREADS;

  printf ("bootbootboot\n");
  printf ("Test: %s\n", NAME_OF_TEST);
  printf ("Number of threads: %d\n", (int) n);

  for (i = 0; i < n; ++i) {
    r = pthread_create (&thread[i], NULL, start_thread, NULL);
    if (r != 0) error (EXIT_FAILURE, r, "pthread_create");
  }
  for (i = 0; i < n; ++i) {
    r = pthread_join (thread[i], NULL);
    if (r != 0) error (EXIT_FAILURE, r, "pthread_join");
  }
  printf ("\n");
  printf ("\n");
  printf ("test ok\n");
  exit (EXIT_SUCCESS);
}

static void *
start_thread (void *vp)
{
  pid_t pid;
  char log_file[] = "/tmp/bbbout.XXXXXX";
  char cmd[16384];
  int fd, i, r, status, test_hanged;
  enum test_status test_status;

  if (mkstemp (log_file) == -1)
    error (EXIT_FAILURE, errno, "mkstemp: %s", log_file);

  create_command_line (cmd, sizeof cmd);

  /* This runs a loop starting the guest. */
  for (;;) {
    pthread_mutex_lock (&lock);
    if (iterations >= NR_ITERATIONS) {
      pthread_mutex_unlock (&lock);
      return NULL;
    }
    if (iterations <= MAX_THREADS) { // stagger the start times
      pthread_mutex_unlock (&lock);
      usleep (rand () % 3000000);
      pthread_mutex_lock (&lock);
    }
    printf ("iterations: %u  skipped: %u\r", iterations, skips);
    fflush (stdout);
    pthread_mutex_unlock (&lock);

    fd = open (log_file, O_WRONLY|O_TRUNC|O_CREAT|O_NOCTTY, 0600);
    if (fd == -1)
      error (EXIT_FAILURE, errno, "open: %s", log_file);

    pid = fork ();
    if (pid == -1)
      error (EXIT_FAILURE, errno, "fork");
    if (pid == 0) {
      /* Child process. */

      /* Capture stdout & stderr to the temporary file. */
      dup2 (fd, STDOUT_FILENO);
      dup2 (fd, STDERR_FILENO);
      close (fd);

      /* Run the command. */
      _exit (system (cmd) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    close (fd);

    /* In the parent wait up to MAX_TIME seconds before declaring a
     * hang.
     */
    status = -1;
    for (i = 0; i < MAX_TIME; ++i) {
      r = waitpid (pid, &status, WNOHANG);
      if (r == -1)
        error (EXIT_FAILURE, errno, "waitpid");
      if (r > 0) break;
      sleep (1);
    }

    test_hanged = i == MAX_TIME;
    if (test_hanged) kill (pid, 9);

    /* Check the test status of this run. */
    test_status = check_test_status (log_file, test_hanged, status);
    switch (test_status) {
    case PASS:
      pthread_mutex_lock (&lock);
      iterations++;
      pthread_mutex_unlock (&lock);
      break;
    case SKIP:
      pthread_mutex_lock (&lock);
      skips++;
      pthread_mutex_unlock (&lock);
      break;
    case FAIL:
      /* If it's a fail, then we've hit the bug so dump the log and
       * exit the program.
       */
      printf ("\n");
      printf ("\n");
      snprintf (cmd, sizeof cmd, "col -b < %s", log_file);
      system (cmd);
      fprintf (stderr,
               "*** TEST %s AFTER %u ITERATIONS ***\n",
               test_hanged ? "HUNG" : "FAILED", iterations);
      exit (EXIT_FAILURE);
    case UNCLASSIFIED:
      printf ("\n");
      printf ("\n");
      fprintf (stderr,
               "*** Unclassified problem ***\n"
               "This is not a test failure, it requires user attention.\n"
               "See the test log file: %s\n"
               "You may need to change config-*.h check_test_status()\n",
               log_file);
      exit (EXIT_FAILURE);
    }
  }

  unlink (log_file);
}
