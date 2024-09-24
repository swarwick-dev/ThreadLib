/*
 * Thread.cpp
 *
 *  Created on: 15 Apr 2015
 *      Author: GBY18020
 */

#include "Thread.h"

#include <cerrno>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "Logger.h"
#include "constants.h"

int iStop = false;

Thread::Thread()
{
	this->eState = INACTIVE;
	this->iPid = 0;
	this->iThreadId = -1;
	this->iExitCode = 0;
	this->sInterface = "";
	this->sLogDir = "";
	this->sRunDtm = "";
	this->sLogFile = "";
	this->sStartDtm = "";
	this->sEndDtm = "";
	this->pSignalHandler = NULL;
	this->pThreadParams = NULL;
	this->runFunction = NULL;
}

Thread::~Thread()
{
	this->eState = INACTIVE;
	this->iPid = 0;
	this->iThreadId = -1;
	this->iExitCode = 0;
	this->sInterface = "";
	this->sLogDir = "";
	this->sRunDtm = "";
	this->sLogFile = "";
	this->sStartDtm = "";
	this->sEndDtm = "";
	this->pSignalHandler = NULL;
	this->pThreadParams = NULL;
}

void Thread::printThreadState()
{
	gl_trace( _BASIC, "process id <%d> thread id <%d> state <%d> exit code <%d> start dtm <%s> end dtm <%s>", this->iPid, this->iThreadId,
			this->eState, this->iExitCode, this->sStartDtm.c_str(), this->sEndDtm.c_str());
}

void Thread::initialise(int iThreadId, std::string sInterface, std::string sLogDir, std::string sRunDtm,
		Delegate<int, int, pid_t> *runFunction, pt2SignalFunction pSignalFunction, void *pThreadParams)
{
	char caFullLog[512 + 1] = { '\0' };

	this->eState = INACTIVE;
	this->iPid = 0;
	this->iThreadId = -1;
	this->iExitCode = 0;
	this->sInterface = sInterface;
	this->sLogDir = sLogDir;
	this->sRunDtm = sRunDtm;
	this->iThreadId = iThreadId;
	this->runFunction = runFunction;
	this->pSignalHandler = pSignalFunction;
	this->pThreadParams = pThreadParams;

	sprintf(caFullLog, "%s/%s_%s_%d.log", this->sLogDir.c_str(), this->sInterface.c_str(), this->sRunDtm.c_str(), this->iThreadId);
	this->sLogFile = caFullLog;
}

int Thread::run()
{
	int retVal = SUCCESS;
	char errorTxt[512 + 1] = { '\0' };
	char caCurrentDtm[DATETIME_LEN + 1] = { '\0' };
	time_t currentTime;
	const struct tm *nowTime;
	pid_t pid = 0;

	errno = 0;

	if ((pid = this->iPid = fork()) < 0) {
		strerror_r(errno, errorTxt, 512);
		gl_error("Fork of thread %d failed :%s", this->iThreadId, errorTxt);
		retVal = FAILURE;
	}
	else if (pid == 0) {
		// Child Thread
		signal(SIGUSR1, this->pSignalHandler);
		signal(SIGINT, this->pSignalHandler);
		signal(SIGTERM, this->pSignalHandler);
		signal(SIGQUIT, this->pSignalHandler);

		std::filebuf threadLog;

		retVal = SUCCESS;
		errno = 0;

		threadLog.open(this->sLogFile.c_str(), std::ios::out);

		if (!threadLog.is_open()) {
			strerror_r(errno, errorTxt, 512);
			gl_error("Error opening log file: %s", errorTxt);
			retVal = FAILURE;
		}
		else {
			std::ostream ostr(&threadLog);
			std::streambuf *coutbuf = std::cout.rdbuf();
			std::cout.rdbuf(ostr.rdbuf());

			gl_inform("%s Thread %d Started", this->sInterface.c_str(), this->iThreadId);

			// Call the main processing function for this interfaces thread (assumes a C++ class delegate)
			retVal = (*this->runFunction)(this->iThreadId, this->iPid);

			gl_inform("%s Thread %d Completed", this->sInterface.c_str(), this->iThreadId);

			std::cout.flush();
			std::cout.rdbuf(coutbuf);
			threadLog.close();
			threadLog.~basic_filebuf();
		}

		_exit(retVal);
	}
	else {
		// Main thread
		// Print thread log file information
		gl_inform("<TID : %d> %s PID Log File : %s", this->iThreadId, this->sInterface.c_str(), this->sLogFile.c_str());
		this->eState = ACTIVE;
		// get the current date and time and format it correctly
		(void) time(&currentTime);
		nowTime = localtime(&currentTime);
		;
		strftime(caCurrentDtm, DATETIME_LEN, "%Y%m%d%H%M%S", nowTime);
		this->sStartDtm = caCurrentDtm;
	}

	return retVal;
}

void Thread::setState(ThreadState i)
{
	this->eState = i;
}
void Thread::setPid(pid_t i)
{
	this->iPid = i;
}
void Thread::setThreadId(int i)
{
	this->iThreadId = i;
}
void Thread::setExitCode(int i)
{
	this->iExitCode = i;
}
int Thread::getState()
{
	return this->eState;
}
pid_t Thread::getPid()
{
	return this->iPid;
}
int Thread::getThreadId()
{
	return this->iThreadId;
}
int Thread::getExitCode()
{
	return this->iExitCode;
}
void Thread::setEndDtm(std::string sEndDtm)
{
	this->sEndDtm = sEndDtm;
}

void Thread::destroy()
{
	this->eState = INACTIVE;
	this->iPid = 0;
	this->iThreadId = -1;
	this->iExitCode = 0;
	this->sInterface = "";
	this->sLogDir = "";
	this->sRunDtm = "";
	this->sLogFile = "";
	this->sStartDtm = "";
	this->sEndDtm = "";
	this->pSignalHandler = NULL;
	this->pThreadParams = NULL;	
}

