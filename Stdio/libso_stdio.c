// Tema 2 SO - Calin Bucur - 332CB

#include "so_stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFLEN 4096

struct _so_file;

// Structure holding the file info
typedef struct _so_file {
	int fd; // The file descriptor
	int r; // Reading permission (1 = True, 0 = False)
	int w; // Writing permission (1 = True, 0 = False)
	int off; // Current reading position from the buffer
	int bytes; // Current position writing into the buffer
	int eof; // Did we reach EoF? (1 = True, 0 = False)
	int err; // Did we encounter an error? (1 = True, 0 = False) 
	int pid; // PID of the process asociated with the pipe (only on popen)
	char type; // The type of the file (r/w/a)
	char prev; // The previous operation executed on the file
	char *buf; // The buffer
} SO_FILE;

// Opens a file and returns the coresponding structure
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd;
	int w = 0;
	int r = 0;
	char type;

	// Set the permissions and the type of the file depending on the mode arg
	if (strcmp(mode, "r") == 0) {
		r = 1;
		fd = open(pathname, O_RDONLY);
		type = 'r';
	} else if (strcmp(mode, "r+") == 0) {
		r = 1;
		w = 1;
		fd = open(pathname, O_RDWR);
		type = 'r';
	} else if (strcmp(mode, "w") == 0) {
		w = 1;
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC);
		type = 'w';
	} else if (strcmp(mode, "w+") == 0) {
		r = 1;
		w = 1;
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC);
		type = 'w';
	} else if (strcmp(mode, "a") == 0) {
		w = 1;
		fd = open(pathname, O_WRONLY | O_CREAT);
		type = 'a';
	} else if (strcmp(mode, "a+") == 0) {
		r = 1;
		w = 1;
		fd = open(pathname, O_RDWR | O_CREAT);
		type = 'a';
	} else
		return NULL;
	// Check if the opening failed
	if (fd < 0)
		return NULL;

	// For append files set the cursor at the end of file
	if (strncmp(mode, "a", 1) == 0) {
		int rd = lseek(fd, 0, SEEK_END);

		if (rd < 0)
			return NULL;
	}
	// Allocate the structure and check it
	SO_FILE *stream = calloc(1, sizeof(SO_FILE));

	if (!stream)
		return NULL;
	// Initialize the fields
	stream->fd = fd;
	stream->w = w;
	stream->r = r;
	stream->off = 0;
	stream->bytes = 0;
	stream->eof = 0;
	stream->prev = 'r';
	stream->type = type;
	stream->err = 0;
	// Allocate and check the buffer
	stream->buf = calloc(BUFLEN, sizeof(char));
	if (!stream->buf) {
		free(stream);
		return NULL;
	}
	return stream;
}

// Reads a character from the file and returns it
int so_fgetc(SO_FILE *stream)
{
	// Check for permission
	if (stream->r == 0) {
		stream->err = 1;
		return SO_EOF;
	}
	// If the buffer needs to be filled
	if (stream->off >= stream->bytes) {
		// Rest the reading position and the buffer
		stream->off = 0;
		memset(stream->buf, 0, BUFLEN);
		// read the buffer
		stream->bytes = read(stream->fd, stream->buf, BUFLEN);
		// Check the buffer and set EoF flag if necessary
		if (stream->bytes <= 0) {
			if (stream->bytes == 0)
				stream->eof = 1;
			return SO_EOF;
		}
	}
	// Get the char from the buffer
	unsigned char c = stream->buf[stream->off];

	// Move the reading position
	stream->off++;
	stream->prev = 'r';
	return (int)c;
}

// Writes a character
int so_fputc(int c, SO_FILE *stream)
{
	// Check for permission
	if (stream->w == 0) {
		stream->err = 1;
		return SO_EOF;
	}
	// If the buffer is full
	if (stream->bytes >= BUFLEN) {
		// Write it in the file
		int rd = write(stream->fd, stream->buf, BUFLEN);
		int written = rd;

		if (rd < 0) {
			stream->err = 1;
			return SO_EOF;
		}
		// Write until either the whole buffer is written or an error is met
		while (written < stream->bytes) {
			stream->err = 1;
			rd = write(stream->fd, stream->buf + written, stream->bytes - written);
			if (rd < 0) {
				stream->err = 1;
				return SO_EOF;
			}
			written += rd;
		}
		// Reset EoF flag, writing position and clear the buffer
		stream->eof = 0;
		memset(stream->buf, 0, BUFLEN);
		stream->bytes = 0;
	}
	// Put the char in the buffer and move the writing position
	stream->buf[stream->bytes] = (char)c;
	stream->bytes++;
	stream->prev = 'w';
	return c;
}

// Flushes the buffer
int so_fflush(SO_FILE *stream)
{
	// Writes the buffer just like so_fputc
	if (stream->prev == 'w') {
		int rd = write(stream->fd, stream->buf, stream->bytes);
		int written = rd;

		if (rd < 0) {
			stream->err = 1;
			return SO_EOF;
		}
		while (written < stream->bytes) {
			rd = write(stream->fd, stream->buf + written, stream->bytes - written);
			if (rd < 0) {
				stream->err = 1;
				return SO_EOF;
			}
			written += rd;
		}
		stream->eof = 0;
		stream->bytes = 0;
		memset(stream->buf, 0, BUFLEN);
	}
	return 0;
}

// Moves the file cursor
int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rd;

	// If previous op was a write flush the buffer
	if (stream->prev == 'w') {
		rd = so_fflush(stream);
		if (rd < 0) {
			stream->err = 1;
			return SO_EOF;
		}
	}
	// If previous op was a read clear the buffer
	else if (stream->prev == 'r') {
		memset(stream->buf, 0, BUFLEN);
		stream->off = 0;
		stream->bytes = 0;
	}
	// Move the cursor and check the result
	rd = lseek(stream->fd, offset, whence);
	if (rd < 0) {
		stream->err = 1;
		return SO_EOF;
	}
	return 0;
}


// Returns the current position of the cursor
long so_ftell(SO_FILE *stream)
{
	// If there is anything in the buffer
	// The actual cursor and our cursor differ
	// So it calculates and returns our cursor without altering the real one
	if (stream->off > 0) {
		int ret = lseek(stream->fd, -BUFLEN + stream->off, SEEK_CUR);

		lseek(stream->fd, BUFLEN - stream->off, SEEK_CUR);
		return ret;
	} else if (stream->bytes > 0) {
		int ret = lseek(stream->fd, stream->bytes, SEEK_CUR);

		lseek(stream->fd, -stream->bytes, SEEK_CUR);
		return ret;
	}
	return lseek(stream->fd, 0, SEEK_CUR);
}

// Reads nmemb elements of the given size into the given memory address
size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i = 0;

	// Simply read char by char using so_getc
	// And place the char at the given memory address
	// Until either we have read the right amount or we have reached EoF
	for (i = 0; i < nmemb; i++) {
		for (int j = 0; j < size; j++) {
			int rd = so_fgetc(stream);

			if (rd == SO_EOF && so_feof(stream) == 0) {
				stream->err = 1;
				return 0;
			}
			if (rd == SO_EOF) {
				stream->err = 1;
				return i;
			}
			((unsigned char *)ptr)[i * size + j] = (unsigned char)rd;
		}
		if (so_feof(stream) != 0)
			return i;
	}
	return i;
}

// Writes nmemb elements of the given size from the given memory address
size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int i;

	// If we are in append mode move the cursor to the end of file
	if (stream->type == 'a')
		lseek(stream->fd, 0, SEEK_END);
	// Write char by char using so_fputc calls
	for (i = 0; i < nmemb; i++) {
		for (int j = 0; j < size; j++) {
			int rd = so_fputc(((unsigned char *)ptr)[i * size + j], stream);

			if (rd == SO_EOF) {
				stream->err = 1;
				return 0;
			}
		}
	}
	return i;
}

int so_fclose(SO_FILE *stream)
{
	int rd, rr;

	// Flush the buffer
	rr = so_fflush(stream);
	// Close the file descriptor
	rd = close(stream->fd);
	// Free the file structure
	free(stream->buf);
	free(stream);
	// Check if everything went good
	if (rd < 0 || rr < 0)
		return SO_EOF;
	return 0;
}

// Returns the EoF flag
int so_feof(SO_FILE *stream)
{
	return stream->eof;
}

// Returns the error flag
int so_ferror(SO_FILE *stream)
{
	return stream->err;
}

// Returns the file descriptor asociated to the file
int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

// Creates a child process and opens a pipe for reading/writing
SO_FILE *so_popen(const char *command, const char *type)
{
	int rd;
	// Create and check the pipe
	int pipe_fd[2];

	rd = pipe(pipe_fd);
	if (rd < 0)
		return NULL;
	// Create and check the child process
	int cpid;

	cpid = fork();
	if (cpid == -1)
		return NULL;
	// The child
	if (cpid == 0) {
		// Depending on the type
		// Redirect stdin/stdout and close the unused pipe fds
		if (strcmp(type, "r") == 0)
			dup2(pipe_fd[1], 1);
		else if (strcmp(type, "w") == 0)
			dup2(pipe_fd[0], 0);
		close(pipe_fd[0]);
		close(pipe_fd[1]);

		// Run the command
		execlp("bash", "bash", "-c", command, (char *) NULL);
	} else { // The parent
		// Depending on the type close the unused pipe fd
		if (strcmp(type, "r") == 0)
			close(pipe_fd[1]);
		else if (strcmp(type, "w") == 0)
			close(pipe_fd[0]);
		// Create a file structure just like so_fopen
		SO_FILE *stream = calloc(1, sizeof(SO_FILE));

		if (stream == NULL)
			return NULL;
		if (strcmp(type, "r") == 0) {
			stream->fd = pipe_fd[0];
			stream->w = 0;
			stream->r = 1;
		} else if (strcmp(type, "w") == 0) {
			stream->fd = pipe_fd[1];
			stream->w = 1;
			stream->r = 0;
		}
		stream->off = 0;
		stream->bytes = 0;
		stream->eof = 0;
		stream->prev = 'r';
		stream->type = type[0];
		stream->err = 0;
		stream->pid = cpid;
		stream->buf = calloc(BUFLEN, sizeof(char));
		if (!stream->buf) {
			free(stream);
			return NULL;
		}
		return stream;
	}
}
int so_pclose(SO_FILE *stream)
{
	int status;
	int rr;

	// Flush the buffer
	rr = so_fflush(stream);
	int rd;

	// Close the file descriptor
	rd = close(stream->fd);
	// Wait for the child process to end
	int ret = waitpid(stream->pid, &status, 0);

	// Free the file structure
	free(stream->buf);
	free(stream);
	// Check if everything went good
	if (rd < 0 || rr < 0 || ret < 0)
		return SO_EOF;
	return 0;
}
