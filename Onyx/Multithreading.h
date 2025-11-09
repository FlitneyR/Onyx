#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>

namespace onyx
{

struct IJob
{
	virtual ~IJob() = default;

	virtual bool TryLock() { return true; }
	virtual void Run() = 0;

	void AddDependency( const IJob* other_job ) { dependencies.push_back( other_job ); }

	std::vector< const IJob* > dependencies;
	std::atomic_bool hasStarted = false;
	bool hasFinished = false;
};

struct JobQueue
{
	void Reserve( u32 num_jobs ) { m_jobs.reserve( num_jobs ); }

	template< typename Job, typename ... Args >
	void AddJob( u64 id, Args ... args )
	{
		m_jobs.insert( { id, std::make_unique< Job >( args ... ) } );
	}

	IJob* GetNextJob( bool& any_unstarted_jobs );
	bool StartNextAvailableJob();
	void Reset() { m_jobs.clear(); }
	u32 Count() const { return m_jobs.size(); }

	inline IJob* GetJob( u64 id )
	{
		auto iter = m_jobs.find( id );
		return iter == m_jobs.end() ? nullptr : iter->second.get();
	}

private:
	std::unordered_map< u64, std::unique_ptr< IJob > > m_jobs;
};

struct WorkerPool
{
	WorkerPool( u32 num_workers = UINT32_MAX );
	~WorkerPool();

	JobQueue& GetJobQueue();
	void Begin();
	void Wait();

private:
	std::vector< std::jthread > m_workers;
	JobQueue m_jobQueue;
	std::atomic_uint32_t m_activeWorkers = 0;
	u32 m_workersCanStart = 0;
	bool m_workersShouldStop = false;

	void Worker( u32 index, u32 count );
};

}
