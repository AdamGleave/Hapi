#include <stdio.h>
#include <stdbool.h>

#include <glog/logging.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

// Attribution: ExecCommandSync & WaitForFinish borrowed from Firmament project,
// present in file src/misc/utils.cc.

// Pipe setup
// infd[0] == CHILD_READ
// infd[1] == PARENT_WRITE
int32_t ExecCommandSync(char *const args[], int infd[2]) {
  pid_t pid;
  if (pipe(infd) != 0) {
    LOG(ERROR) << "Failed to create pipe to task.";
  }
  /*if (pipe(outfd) != 0) {
    LOG(ERROR) << "Failed to create pipe from task.";
  }
  if (pipe(errfd) != 0) {
      LOG(ERROR) << "Failed to create pipe from task.";
	}*/

  pid = fork();
  switch (pid) {
    case -1:
      // Error
      PLOG(FATAL) << "Failed to fork child process.";
      break;
    case 0: {
      // Child
      // Close parent pipe descriptors
      close(infd[1]);
      /*close(outfd[0]);
      close(errfd[0]);*/
      // set up pipes
      CHECK(dup2(infd[0], STDIN_FILENO) == STDIN_FILENO);
      /*CHECK(dup2(outfd[1], STDOUT_FILENO) == STDOUT_FILENO);
      CHECK(dup2(errfd[1], STDERR_FILENO) == STDERR_FILENO);*/
      // close unnecessary pipe descriptors
      close(infd[0]);
      /*close(outfd[1]);
      close(errfd[1]);*/
      // Run the task binary
      execvp(args[0], &args[0]);
      // execl only returns if there was an error
      PLOG(FATAL) << "execvp failed";
      //ReportTaskExecutionFailure();
      _exit(1);
      break;
    }
    default:
      // Parent
      VLOG(1) << "Subprocess with PID " << pid << " created.";
      // close unused pipe ends
      close(infd[0]);
      /*close(outfd[1]);
      close(errfd[1]);*/
      return pid;
  }
  return -1;
}

bool WaitForFinish(pid_t pid) {
  // Wait for task to terminate
  int status;
  while (waitpid(pid, &status, 0) != pid) {
    VLOG(2) << "Waiting for child process " << pid << " to exit...";
  }
  if (WIFEXITED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " exited with status "
            << WEXITSTATUS(status);
    if (WEXITSTATUS(status) == 0) {
    	return true;
    }
  } else if (WIFSIGNALED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " exited due to uncaught "
            << "signal " << WTERMSIG(status);
  } else if (WIFSTOPPED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " is stopped due to "
            << "signal " << WSTOPSIG(status);
  } else {
    LOG(ERROR) << "Unexpected exit status: " << std::hex << status;
  }
  return false;
}

int main(int argc, char *argv[]) {
	FLAGS_logtostderr = true;
	google::InitGoogleLogging(argv[0]);

	if (argc < 2) {
		fprintf(stderr, "usage: <path to solver> [solver arguments]\n");
		return 1;
	}

	char line[1024];
	int infd[2];
	pid_t child;

	while (true) {
		if (fgets(line, sizeof(line), stdin) == NULL) {
			// EOF
			return 0;
		} // there's some data, launch a solver

		child = ExecCommandSync(&argv[1], infd);
		FILE *child_stdin = fdopen(infd[1], "w");
		fprintf(child_stdin, "%s", line);

		while (fgets(line, sizeof(line), stdin)) {
			if (strcmp(line, "c EOI\n") == 0) {
				break;
			} else {
				// write to pipe
				fprintf(child_stdin, "%s", line);
			}
		}
		// send EOF to child process
		fclose(child_stdin);
		// wait for results
		if (!WaitForFinish(child)) {
			LOG(FATAL) << "Process terminated abnormally.";
		}
		close(infd[1]);

		fprintf(stdout, "c EOI\n");
		fflush(stdout);
	}

	return 0;
}
