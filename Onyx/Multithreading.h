#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>

namespace onyx
{

struct IJob
{
	virtual ~IJob() = default;
	virtual void Run() = 0;

	std::atomic_bool hasStarted = false;
};

struct JobQueue
{
	void Reserve( u32 num_jobs ) { m_jobs.reserve( num_jobs ); }

	template< typename Job, typename ... Args >
	void AddJob( Args ... args )
	{
		m_jobs.push_back( std::make_unique< Job >( args ... ) );
	}

	bool StartNextAvailableJob();
	void Reset() { m_jobs.clear(); }

private:
	std::vector< std::unique_ptr< IJob > > m_jobs;
};

struct WorkerPool
{
	WorkerPool( u32 num_workers = 0 );
	~WorkerPool();

	JobQueue& GetJobQueue();
	void Begin();
	void Wait();

private:
	std::vector< std::jthread > m_workers;
	JobQueue m_jobQueue;
	std::atomic_uint32_t m_activeWorkers = 0;
	bool m_workersCanStart = false;
	bool m_workersShouldStop = false;

	void Worker( u32 index );
};

}
