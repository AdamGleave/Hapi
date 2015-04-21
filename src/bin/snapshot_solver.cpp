#include <iostream>
#include <sstream>
#include <string>

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

	int infd[2];
	pid_t child;

	std::stringstream buffer(std::ios::in|std::ios::out);
	std::string line;
	while (std::cin.good()) {
		std::getline(std::cin, line);
		while (std::cin.good()) {
			if (line == "c EOI") {
				// end of iteration
				break;
			} else {
				// input to solver, add to buffer
				buffer << line << std::endl;
			}

			std::getline(std::cin, line);
		}

		if (buffer.str().empty()) {
			// no data read. don't break, could just be an empty iteration.
			continue;
		}

		// Tell benchmark to start timing from here. This lets us exclude, in the
		// total time measurement, the overhead of the snapshot generator.
		//
		// We also exclude the time spent creating the process. This has the benefit
		// that it allows more direct comparison with an incremental solver. We
		// could write a full solver that would always stay active, after all.
		//
		// However, there is the disadvantage this excludes some of the time
		// spent parsing.
		//
		// N.B. Snapshot generator really slow, so the buffering is critical.
		fprintf(stderr, "STARTTIME\n");
		fflush(stderr);

		// Launch solver
		child = ExecCommandSync(&argv[1], infd);
		FILE *child_stdin = fdopen(infd[1], "w");

		// Send the graph across
		fprintf(child_stdin, "%s", buffer.str().c_str());
		// EOF needed to indicate graph is finished
		fclose(child_stdin);

		// Reset state
		buffer.str(""); // empty contents
		buffer.clear(); // reset flags

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
