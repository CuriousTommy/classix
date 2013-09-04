//
// DebugThreadManager.cpp
// Classix
//
// Copyright (C) 2012 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//

#ifndef __Classix__DebugThreadManager__
#define __Classix__DebugThreadManager__

#include <mutex>
#include <thread>
#include <memory>
#include <vector>
#include <condition_variable>

#include "ThreadManager.h"
#include "MachineState.h"
#include "Interpreter.h"
#include "StackPreparator.h"
#include "Structures.h"
#include "WaitQueue.h"

enum class ThreadState
{
	NotReady,
	Stopped,
	Executing,
	Completed
};

enum class StopReason
{
	Executing,
	InterruptTrap,
	AccessViolation,
	InvalidInstruction,
};

enum class RunCommand
{
	Kill = -1,
	None = 0,
	
	// Commands <= 0 cannot be passed to Perform
	SingleStep,
	StepOver,
	Continue,
};

class DebugThreadManager;

struct ThreadContext
{
	friend class DebugThreadManager;
	
	Common::AutoAllocation stack;
	PPCVM::MachineState machineState;
	uint32_t pc; // valid only when thread is stopped
	
	ThreadState GetThreadState() const;
	StopReason GetStopReason() const;
	void Perform(RunCommand command);
	
	void Interrupt();
	void Kill();
	
	std::thread::native_handle_type GetThreadId();
	
private:
	PPCVM::Execution::Interpreter interpreter;
	std::thread thread;
	ThreadState executionState;
	StopReason stopReason;
	
	std::mutex mShouldResume;
	std::condition_variable cvShouldResume;
	std::atomic<RunCommand> nextAction;
	
	ThreadContext(Common::Allocator& allocator, DebugThreadManager& manager, size_t stackSize);
	
	RunCommand GetNextAction();
	
	ThreadContext(const ThreadContext&) = delete;
	ThreadContext(ThreadContext&&) = delete;
	void operator=(const ThreadContext&) = delete;
	void operator=(ThreadContext&&) = delete;
};

struct ThreadUpdate
{
	ThreadContext& context;
	ThreadState state;
	
	explicit ThreadUpdate(ThreadContext& context);
};

class DebugThreadManager : public OSEnvironment::ThreadManager
{
	friend class ThreadContext;
	friend class Breakpoint;
	
	// needs to be a recursive mutex so EnterCriticalSection doesn't
	Common::Allocator& allocator;
	unsigned inCriticalSection;
	
	mutable std::recursive_mutex threadsLock;
	std::unordered_map<std::thread::native_handle_type, std::unique_ptr<ThreadContext>> threads;
	uint32_t lastExitCode;
	
	mutable std::mutex breakpointsLock;
	std::unordered_map<Common::UInt32*, std::pair<PPCVM::Instruction, unsigned>> breakpoints;
	
	// wait queues
	std::shared_ptr<WaitQueue<std::string>> sink;
	WaitQueue<ThreadUpdate> changingContexts;
	
	bool GetRealInstruction(Common::UInt32* location, PPCVM::Instruction& output);
	void DebugLoop(ThreadContext& context, bool autostart);
	
public:
	class Breakpoint
	{
		friend class DebugThreadManager;
		DebugThreadManager& threads;
		Common::UInt32* location;
		PPCVM::Instruction instruction;
		
		Breakpoint(DebugThreadManager& manager, Common::UInt32* location);

	public:
		Breakpoint(Breakpoint&& that);
		Breakpoint(const Breakpoint& that) = delete;
		
		const Common::UInt32* GetLocation() const;
		PPCVM::Instruction GetInstruction() const;
		~Breakpoint();
	};
	
	DebugThreadManager(Common::Allocator& allocator);
	
	virtual bool IsThreadExecuting() const override;
	virtual void MarkThreadAsExecuting() override;
	virtual void UnmarkThreadAsExecuting() override;
	
	virtual void EnterCriticalSection() noexcept override;
	virtual void ExitCriticalSection() noexcept override;
	
	void SetBreakpoint(Common::UInt32* location);
	bool RemoveBreakpoint(Common::UInt32* location);
	Breakpoint CreateBreakpoint(Common::UInt32* location);
	
	std::shared_ptr<WaitQueue<std::string>> GetCommandSink();
	void SetCommandSink(std::shared_ptr<WaitQueue<std::string>>& sink);
	
	void ConsumeThreadEvents(); // expected to run on a dedicated thread
	uint32_t GetLastExitCode() const; // exit code of the last thread to complete
	
	ThreadContext& StartThread(const Common::StackPreparator& stack, size_t stackSize, const PEF::TransitionVector& entryPoint, bool startNow = false);

	size_t ThreadCount() const;
	bool GetThread(std::thread::native_handle_type handle, ThreadContext*& context);
	
	template<typename TAction>
	void ForEachThread(TAction&& action)
	{
		std::lock_guard<std::recursive_mutex> guard(threadsLock);
		for (auto& pair : threads)
		{
			action(*pair.second.get());
		}
	}
};

#endif /* defined(__Classix__DebugThreadManager__) */