/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

 // Calin-Andrei Bucur 332CB

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "exec_parser.h"

static so_exec_t *exec; // Executable struct
static struct sigaction default_handler; // I save the default SEGV handler here
int page_size; // Size of a page
int exec_fd; // File descriptor of the exec file

// Custom handler for page faults
static void so_handler (int signum, siginfo_t *info, void *ucontext)
{
	// Go through the segments
	for (int i = 0; i < exec->segments_no; i++) {
		// Check if the page fault is inside the current segment
		if ((int)info->si_addr >= exec->segments[i].vaddr && (int)info->si_addr < exec->segments[i].vaddr + exec->segments[i].mem_size) {
			// Page where the fault occured
			int page_idx = ((int)info->si_addr - exec->segments[i].vaddr) / page_size;

			// Check if the page is already mapped
			// If it's mapped that means permissions are being trespassed and we run the default handler to get a segfault
			if (((unsigned int *)exec->segments[i].data)[page_idx] == 1)
				default_handler.sa_sigaction(signum, info, ucontext);
			// Check if the file size is the same as memory size
			if (exec->segments[i].mem_size == exec->segments[i].file_size) {
				int length;

				// Determine whether the page can be filled or not
				// And calculate how much of it can be filled
				if ((page_idx + 1) * page_size > exec->segments[i].file_size)
					length = exec->segments[i].file_size - page_idx * page_size;
				else
					length = page_size;
				void *rd;

				// Map the page and copy the contents of the exec
				rd = mmap((void *)exec->segments[i].vaddr + page_idx * page_size, length, exec->segments[i].perm, MAP_PRIVATE | MAP_FIXED, exec_fd, exec->segments[i].offset + page_idx * page_size);
				if ((int)rd == -1)
					exit(-1);
			} else { // If we need to have additional zero-ed memory
				// Check if we are on a page that needs to be fully filled
				if ((page_idx + 1) * page_size <= exec->segments[i].file_size) {
					void *rd;

					// Map the page and copy the contents of the exec
					rd = mmap((void *)exec->segments[i].vaddr + page_idx * page_size, page_size, exec->segments[i].perm, MAP_PRIVATE | MAP_FIXED, exec_fd, exec->segments[i].offset + page_idx * page_size);
					if ((int)rd == -1)
						exit(-1);
				} else if ((page_idx + 1) * page_size > exec->segments[i].file_size && page_idx * page_size < exec->segments[i].file_size) {
					// Check if the page needs to be partially filled and partially zero-ed
					void *rd;

					// Map the page and copy the contents of the exec file
					rd = mmap((void *)exec->segments[i].vaddr + page_idx * page_size, exec->segments[i].file_size - page_idx * page_size, exec->segments[i].perm, MAP_PRIVATE | MAP_FIXED, exec_fd, exec->segments[i].offset + page_idx * page_size);
					if ((int)rd == -1)
						exit(-1);
					// Fill the rest of the page with zeros
					memset((void *)exec->segments[i].vaddr + exec->segments[i].file_size, 0, (page_idx + 1) * page_size - exec->segments[i].file_size);
				} else { // If the whole page needs to be zero-ed
					void *rd;

					// Map the page and fill it with zeros
					rd = mmap((void *)exec->segments[i].vaddr + page_idx * page_size, exec->segments[i].mem_size, exec->segments[i].perm, MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
					if ((int)rd == -1)
						exit(-1);
				}
			}
			// Mark the page as mapped and return
			((unsigned int *)exec->segments[i].data)[page_idx] = 1;
			return;
		}
	}
	// If we got here it means the page fault isn't inside any segment
	// So we must segfault
	default_handler.sa_sigaction(signum, info, ucontext);
}

// Sets so_handler as the used handler
int so_init_loader(void)
{
	int rd;

	// Get the page size
	page_size = getpagesize();
	struct sigaction handler;

	memset(&handler, 0, sizeof(handler));
	// Initialize the signal mask
	rd = sigemptyset(&handler.sa_mask);
	if (rd < 0)
		return -1;
	rd = sigaddset(&handler.sa_mask, SIGSEGV);
	if (rd < 0)
		return -1;
	// Set the flags
	handler.sa_flags = SA_SIGINFO;
	handler.sa_sigaction = so_handler;
	// Set so_handler as the new handler but also save the default handler
	rd = sigaction(SIGSEGV, &handler, &default_handler);
	if (rd < 0)
		return -1;
	return 0;
}

// Executes
int so_execute(char *path, char *argv[])
{
	// Parses the exec
	exec = so_parse_exec(path);
	if (!exec)
		return -1;
	// Opens the file descriptor for the exec file
	exec_fd = open(path, O_RDONLY, 0);
	if (exec_fd < 0)
		return -1;
	// Initialize the data field of each segment
	// An int for every page in the segment
	// 0 = unmapped
	// 1 = mapped
	for (int i = 0; i < exec->segments_no; i++) {
		exec->segments[i].data = (void *)calloc(ceil((exec->segments[i].mem_size)/page_size), sizeof(unsigned int));
		if (!exec->segments[i].data)
			return -1;
	}
	// Execute
	so_start_exec(exec, argv);
	for (int i = 0; i < exec->segments_no; i++)
		free(exec->segments[i].data);
	close(exec_fd);
	return 0;
}
