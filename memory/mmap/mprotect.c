#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ucontext.h>

// Global variables for demonstration
static void *protected_memory = NULL;
static size_t memory_size = 0;
static int segv_count = 0;

// SIGSEGV handler function
void sigsegv_handler(int sig, siginfo_t *info, void *context) {
    segv_count++;
    
    printf("\n=== SIGSEGV Handler Triggered ===\n");
    printf("Signal: %d (SIGSEGV)\n", sig);
    printf("Fault address: %p\n", info->si_addr);
    printf("Error code: %d\n", info->si_code);
    printf("Fault count: %d\n", segv_count);
    
    // Decode the error code
    switch(info->si_code) {
        case SEGV_MAPERR:
            printf("Error type: Address not mapped to object\n");
            break;
        case SEGV_ACCERR:
            printf("Error type: Invalid permissions for mapped object\n");
            break;
        default:
            printf("Error type: Unknown (%d)\n", info->si_code);
    }
    
    // Get context information (architecture-dependent)
    ucontext_t *uc = (ucontext_t *)context;
    printf("Instruction pointer: %p\n", (void*)uc->uc_mcontext.gregs[REG_RIP]);
    
    // Check if the fault is in our protected region
    if (info->si_addr >= protected_memory && 
        info->si_addr < (char*)protected_memory + memory_size) {
        printf("Fault is within protected memory region\n");
        
        // Option 1: Fix the protection and continue
        if (segv_count <= 3) {
            printf("Temporarily allowing access to continue execution...\n");
            if (mprotect(protected_memory, memory_size, PROT_READ | PROT_WRITE) == -1) {
                perror("mprotect (allow)");
                exit(1);
            }
            return; // Continue execution
        }
    }
    
    printf("Too many faults or fault outside protected region. Exiting.\n");
    exit(1);
}

// Function to set up SIGSEGV handler
void setup_signal_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO; // Use siginfo_t structure
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("SIGSEGV handler installed\n");
}

// Function to demonstrate memory protection
void demonstrate_mprotect() {
    printf("\n=== Memory Protection Demonstration ===\n");
    
    // Allocate memory page-aligned
    size_t page_size = getpagesize();
    memory_size = page_size;
    
    // Use mmap to allocate memory (easier to control protection)
    protected_memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (protected_memory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    
    printf("Allocated %zu bytes at %p (page size: %zu)\n", 
           memory_size, protected_memory, page_size);
    
    // Write some data to the memory
    strcpy((char*)protected_memory, "Hello, World!");
    printf("Wrote data: '%s'\n", (char*)protected_memory);
    
    // Make memory read-only
    printf("\nMaking memory read-only...\n");
    if (mprotect(protected_memory, memory_size, PROT_READ) == -1) {
        perror("mprotect (read-only)");
        exit(1);
    }
    
    printf("Reading from protected memory: '%s'\n", (char*)protected_memory);
    
    // This will trigger SIGSEGV
    printf("Attempting to write to protected memory...\n");
    strcpy((char*)protected_memory, "This should fail!");
    
    printf("Reading from protected memory: '%s'\n", (char*)protected_memory);

    printf("Write succeeded after handler intervention\n");
}


// Main demonstration function
int main() {
    printf("=== Access Violation & SIGSEGV Demo with mprotect ===\n");
    
    // Set up signal handler first
    setup_signal_handler();
    
    // Main demonstration
    demonstrate_mprotect();
    
    // Clean up
    if (protected_memory != NULL) {
        printf("\nCleaning up...\n");
        munmap(protected_memory, memory_size);
    }
    
    printf("Demo completed successfully!\n");
    return 0;
}

/*
=== Access Violation & SIGSEGV Demo with mprotect ===
SIGSEGV handler installed

=== Memory Protection Demonstration ===
Allocated 4096 bytes at 0x7ae2190d3000 (page size: 4096)
Wrote data: 'Hello, World!'

Making memory read-only...
Reading from protected memory: 'Hello, World!'
Attempting to write to protected memory...

=== SIGSEGV Handler Triggered ===
Signal: 11 (SIGSEGV)
Fault address: 0x7ae2190d3000
Error code: 2
Fault count: 1
Error type: Invalid permissions for mapped object
Instruction pointer: 0x58dd00a506c1
Fault is within protected memory region
Temporarily allowing access to continue execution...
Reading from protected memory: 'This should fail!'
Write succeeded after handler intervention

Cleaning up...
Demo completed successfully!
*/