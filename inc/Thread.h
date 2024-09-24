/*
 * Thread.h
 *
 *  Created on: 15 Apr 2015
 *      Author: GBY18020
 */

#ifndef THREAD_H_
#define THREAD_H_

#include <signal.h>
#include <string>

#include "Delegate.h"

typedef void (*pt2SignalFunction)(int iSigNum);

typedef enum _ThreadState {
	INACTIVE, ACTIVE
} ThreadState;

class Thread {
	private:
		ThreadState eState;
		pid_t iPid;
		int iThreadId;
		int iExitCode;
		std::string sInterface;
		std::string sLogDir;
		std::string sRunDtm;
		std::string sLogFile;
		std::string sStartDtm;
		std::string sEndDtm;
		pt2SignalFunction pSignalHandler;
		void *pThreadParams;
		Delegate<int, int, pid_t> *runFunction;
	public:
		Thread();
		virtual ~Thread();
		void destroy();
		void printThreadState();
		int run();
		void setState(ThreadState i);
		void setPid(pid_t i);
		void setThreadId(int i);
		void setExitCode(int i);
		void setEndDtm(std::string sEndDtm);
		int getState();
		pid_t getPid();
		int getThreadId();
		int getExitCode();
		void initialise(int iThreadId, std::string sInterface, std::string sLogDir, std::string sRunDtm,
				Delegate<int, int, pid_t> *runFunction, pt2SignalFunction pSignalFunction, void *pThreadParams);
};

#endif /* THREAD_H_ */
