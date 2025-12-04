#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <ucontext.h>
#include <cstdio>
#include <cstdlib>

void sigusr1_handler(int, siginfo_t* info, void* ctx) {
    ucontext_t* uc = (ucontext_t*)ctx;

    pid_t sender_pid = info->si_pid;
    uid_t sender_uid = info->si_uid;

    struct passwd* pw = getpwuid(sender_uid);
    const char* username = pw ? pw->pw_name : "unknown";

    unsigned long eip=0, eax=0, ebx=0;
#if defined(__x86_64__)
    eip = uc->uc_mcontext.gregs[REG_RIP];
    eax = uc->uc_mcontext.gregs[REG_RAX];
    ebx = uc->uc_mcontext.gregs[REG_RBX];
#elif defined(__i386__)
    eip = uc->uc_mcontext.gregs[REG_EIP];
    eax = uc->uc_mcontext.gregs[REG_EAX];
    ebx = uc->uc_mcontext.gregs[REG_EBX];
#endif
  
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "Received a SIGUSR1 signal from process [%d] executed by [%d] (%s).\n"
        "State of the context: EIP = [%lx], EAX = [%lx], EBX = [%lx].\n",
        sender_pid, sender_uid, username, eip, eax, ebx);

    write(STDOUT_FILENO, buf, n);
}

int main() {
    char pidbuf[64];
    int n = snprintf(pidbuf, sizeof(pidbuf), "My PID: %d\n", getpid());
    write(STDOUT_FILENO, pidbuf, n);

    struct sigaction sa{};
    sa.sa_sigaction = sigusr1_handler;
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
        perror("sigaction failed");
        return 1;
    }

    while (1) sleep(10);

    return 0;
}
