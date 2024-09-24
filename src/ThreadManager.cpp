/*
 * ThreadManager.cpp
 *
 *  Created on: 15 Apr 2015
 *      Author: GBY18020
 */

#include "ThreadManager.h"
#include <signal.h>
#include <cerrno>
#include <sys/wait.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "constants.h"
#include "Logger.h"

extern int mainPid;
extern int iExitCode;
extern ThreadManager threads;

void handleSIGTERM(int sig)
{
	if (getpid() == mainPid) {
		gl_inform("Kill signal received attempting clean exit");
		threads.shutdown();
		gl_warn("Recommend execution of recovery phase prior to restarting interface");
		iExitCode = FAILURE;
	}
}

void handleSIGCHLD(int sig)
{
	int status;
	pid_t pid;
	char errorTxt[512 + 1] = { '\0' };
	char caCurrentDtm[DATETIME_LEN + 1] = { '\0' };
	time_t currentTime;
	const struct tm *nowTime;

	while ((pid = waitpid(-1, &status, 0)) > 0) {
		if (pid == -1) {
			strerror_r(errno, errorTxt, 512);
			gl_error("Error while waiting for child process to complete : %s", errorTxt);
		}
		else {
			for (int iter2 = 0; iter2 < MAX_THREADS; iter2++) {
				if (threads.aThreads[iter2].getPid() == pid) {
					threads.aThreads[iter2].setState(INACTIVE);
					/* get the current date and time and format it correctly */
					(void) time(&currentTime);
					nowTime = localtime(&currentTime);
					strftime(caCurrentDtm, DATETIME_LEN, "%Y%m%d%H%M%S", nowTime);
					threads.aThreads[iter2].setEndDtm(caCurrentDtm);

					if (WIFEXITED(status)) {
						gl_inform("Thread %d exited with status %d", threads.aThreads[iter2].getThreadId(), WEXITSTATUS(status));
						threads.aThreads[iter2].setExitCode(WEXITSTATUS(status));
					}

					if (WIFSIGNALED(status)) {
						gl_inform("Thread %d exited due to signal %d", threads.aThreads[iter2].getThreadId(), WTERMSIG(status));
						threads.aThreads[iter2].setExitCode(WTERMSIG(status));
						iExitCode = FAILURE;
					}

					if (status != 0) {
						gl_error("Check log file for thread %d to determine the error(s)", threads.aThreads[iter2].getThreadId());
						iExitCode = FAILURE;
					}
				}
			}
		}
	}
}

ThreadManager::ThreadManager()
{
	// TODO Auto-generated constructor stub
	this->iNumThreads = 0;
	this->sInterface = "";
	this->sLogDir = "";
	this->sRunDtm = "";
	this->pSignalFunction = NULL;
	this->pThreadParams = NULL;
	this->runFunction = NULL;
}

ThreadManager::~ThreadManager()
{
	// TODO Auto-generated destructor stub
}

void ThreadManager::printThreadStates()
{
	for (int i = 0; i < this->iNumThreads; i++) {
		this->aThreads[i].printThreadState();
	}
}

int ThreadManager::run()
{
	int retVal = SUCCESS;
	int iter = 0;
	char errorTxt[512 + 1] = { '\0' };
	pid_t pid;
	int status = 0;
	int errCount = 0;
	char caCurrentDtm[DATETIME_LEN + 1] = { '\0' };
	time_t currentTime;
	const struct tm *nowTime;

	gl_inform("About to fork instances, monitor thread logs for progress");

	for (iter = 0; iter < this->iNumThreads; iter++) {
		this->aThreads[iter].run();
		sleep(1); // pad out the thread starts
	}

	this->printThreadStates();

	while (iter > 0) {
		pid = waitpid(-1, &status, 0); /* Use -1 as the pid to wait for the next exitting child */
		--iter;

		if (pid == -1) {
			strerror_r(errno, errorTxt, 512);
			gl_error("Error while waiting for child process to complete : %s", errorTxt);
		}
		else {
			for (int iter2 = 0; iter2 < MAX_THREADS; iter2++) {
				if (threads.aThreads[iter2].getPid() == pid) {
					threads.aThreads[iter2].setState(INACTIVE);
					/* get the current date and time and format it correctly */
					(void) time(&currentTime);
					nowTime = localtime(&currentTime);
					strftime(caCurrentDtm, DATETIME_LEN, "%Y%m%d%H%M%S", nowTime);
					threads.aThreads[iter2].setEndDtm(caCurrentDtm);

					if (WIFEXITED(status)) {
						gl_inform("Thread %d exited with status %d", threads.aThreads[iter2].getThreadId(), WEXITSTATUS(status));
						threads.aThreads[iter2].setExitCode(WEXITSTATUS(status));
					}

					if (WIFSIGNALED(status)) {
						gl_inform("Thread %d exited due to signal %d", threads.aThreads[iter2].getThreadId(), WTERMSIG(status));
						threads.aThreads[iter2].setExitCode(WTERMSIG(status));
						retVal = FAILURE;
						iExitCode = FAILURE;
					}

					if (status != 0) {
						gl_error("Check log file for thread %d to determine the error(s)", threads.aThreads[iter2].getThreadId());
						retVal = FAILURE;
						iExitCode = FAILURE;
					}
				}
			}
		}
	}

	this->printThreadStates();

	gl_inform("All instances finished ");
	return retVal;
}

int ThreadManager::initialise(std::string sInterface, std::string sLogDir, int iNumThreads, std::string sRunDtm,
		Delegate<int, int, pid_t> *runFunction, pt2SignalFunction pSignalFunction, void *pThreadParams)
{
	int retVal = SUCCESS;

	this->sInterface = sInterface;
	this->sLogDir = sLogDir;
	this->sRunDtm = sRunDtm;
	this->iNumThreads = iNumThreads;
	this->runFunction = runFunction;
	this->pSignalFunction = pSignalFunction;
	this->pThreadParams = pThreadParams;

	for (int iter = 0; iter < this->iNumThreads; iter++) {
		this->aThreads[iter].initialise(iter + 1, this->sInterface, this->sLogDir, this->sRunDtm, this->runFunction, this->pSignalFunction,
				this->pThreadParams);
	}

	return retVal;
}

int ThreadManager::shutdown()
{
	int retVal = SUCCESS;
	int iter = 0;
	int status;
	int pid;
	char errorTxt[512 + 1] = { '\0' };

	// Handle stopping of Post Processing Threads
	for (iter = 0; iter < this->iNumThreads; iter++) {
		if (this->aThreads[iter].getState() == ACTIVE && this->aThreads[iter].getPid() != 0) {
			gl_inform("Attempting shutdown of pid %d", this->aThreads[iter].getPid());
			if (kill(this->aThreads[iter].getPid(), SIGUSR1) != 0) {
				strerror_r(errno, errorTxt, 512);
				gl_error("Error while killing thread %d : %s", this->aThreads[iter].getPid(), errorTxt);
				break;
			}
		}
	}

	return retVal;
}

void ThreadManager::destroy()
{
	int iter = 0;

	for (iter = 0; iter < this->iNumThreads; iter++) {
		this->aThreads[iter].destroy();
	}
}

