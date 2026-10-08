#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <term.h>

// The exit sequence, saved before it is disabled. endwin must not send it,
// or the terminal switches back to the pre-session screen and the text vanishes.
static char g_exitCaMode[64];

void DisableAlternateScreen()
{
  g_exitCaMode[0] = '\0';
  if (cur_term == nullptr)
    return;

  if ((exit_ca_mode != nullptr) && (exit_ca_mode != (char *)-1)) {
    strncpy(g_exitCaMode, exit_ca_mode, sizeof(g_exitCaMode) - 1);
    g_exitCaMode[sizeof(g_exitCaMode) - 1] = '\0';
    exit_ca_mode[0] = '\0';
  }
  if ((enter_ca_mode != nullptr) && (enter_ca_mode != (char *)-1))
    enter_ca_mode[0] = '\0';
}

void LeaveAlternateScreen()
{
  if (g_exitCaMode[0] == '\0')
    return;
  putp(g_exitCaMode);
  fflush(stdout);
}
