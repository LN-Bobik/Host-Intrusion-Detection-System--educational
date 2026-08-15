#include "hids.h"

volatile sig_atomic_t g_running       = 1;
volatile sig_atomic_t g_reload_config = 0;
volatile sig_atomic_t g_force_check   = 0;

static void handle_sigterm(int signo) { (void)signo; g_running = 0;       }
static void handle_sighup(int signo)  { (void)signo; g_reload_config = 1; }
static void handle_sigusr1(int signo) { (void)signo; g_force_check = 1;   }

void signals_init(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}