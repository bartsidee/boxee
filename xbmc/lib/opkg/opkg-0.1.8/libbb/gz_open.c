/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) many different people.  If you wrote this, please
 * acknowledge your work.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libbb.h"

FILE *
gz_open(FILE *compressed_file, int *pid)
{
	int unzip_pipe[2];

	if (pipe(unzip_pipe)!=0) {
		perror_msg("pipe");
		return(NULL);
	}

    /* If we don't flush, we end up with two copies of anything pending, 
       one from the parent, one from the child */
    fflush(stdout);
    fflush(stderr);

	if ((*pid = fork()) == -1) {
		perror_msg("fork");
		return(NULL);
	}
	if (*pid==0) {
		/* child process */
		close(unzip_pipe[0]);
		unzip(compressed_file, fdopen(unzip_pipe[1], "w"));
		fflush(NULL);
		fclose(compressed_file);
		close(unzip_pipe[1]);
		_exit(EXIT_SUCCESS);
	}
	close(unzip_pipe[1]);
	return(fdopen(unzip_pipe[0], "r"));
}

int
gz_close(int gunzip_pid)
{
	int status;
	int ret;

	if (waitpid(gunzip_pid, &status, 0) == -1) {
		perror_msg("waitpid");
		return -1;
	}
	
	if (WIFSIGNALED(status)) {
		error_msg("Unzip process killed by signal %d.\n",
			WTERMSIG(status));
		return -1;
	}

	if (!WIFEXITED(status)) {
		/* shouldn't happen */
		error_msg("Your system is broken: got status %d from waitpid.\n",
				status);
		return -1;
	}

	if ((ret = WEXITSTATUS(status))) {
		error_msg("Unzip process failed with return code %d.\n",
				ret);
		return -1;
	}

	return 0;
}
