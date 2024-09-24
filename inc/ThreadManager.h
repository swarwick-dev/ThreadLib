/*
 * ThreadManager.h
 *
 *  Created on: 15 Apr 2015
 *      Author: GBY18020
 */

#ifndef THREADMANAGER_H_
#define THREADMANAGER_H_

#include <string>
#include <sys/types.h>

#include "Thread.h"
#include "Delegate.h"

void handleSIGTERM(int sig);
void handleSIGCHLD(int sig);

#define MAX_THREADS 30

class ThreadManager {
	private:
		std::string sInterface;
		std::string sLogDir;
		std::string sRunDtm;
		Delegate<int, int, pid_t> *runFunction;
		void *pThreadParams;
		pt2SignalFunction pSignalFunction;
	public:
		Thread aThreads[MAX_THREADS];
		int iNumThreads;

		ThreadManager();
		virtual ~ThreadManager();
		int initialise(std::string sInterface, std::string sLogDir, int iNumThreads, std::string sRunDtm,
				Delegate<int, int, pid_t> *runFunction, pt2SignalFunction pSignalFunction, void *pThreadParams);
		void printThreadStates();
		int run();
		int shutdown();
		void destroy();
};

#endif /* THREADMANAGER_H_ */
