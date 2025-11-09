#include "Multithreading.h"

#include "tracy/Tracy.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

namespace onyx
{

bool JobQueue::StartNextAvailableJob()
{
	IJob* job = nullptr;
	for ( auto& iter : m_jobs )
	{
		if ( !iter->hasStarted.exchange( true ) )
		{
			job = iter.get();
			break;
		}
	}

	if ( !job )
		return false;

	job->Run();
	return true;
}

WorkerPool::WorkerPool( u32 num_workers )
{
	if ( num_workers == 0 )
		num_workers = std::thread::hardware_concurrency();

	m_workers.reserve( num_workers );
	for ( u32 idx = 0; idx < num_workers; ++idx )
		m_workers.push_back( std::jthread( [this, idx] { Worker( idx ); } ) );
}

WorkerPool::~WorkerPool()
{
	m_workersShouldStop = true;
	Wait();

	for ( auto& thread : m_workers )
		thread.request_stop();

	m_workers.clear();
}

JobQueue& WorkerPool::GetJobQueue()
{
	Wait();
	m_jobQueue.Reset();
	return m_jobQueue;
}

void WorkerPool::Begin()
{
	ZoneScoped;

	m_workersCanStart = true;
	while ( m_activeWorkers < m_workers.size() );
	m_workersCanStart = false;
}

void WorkerPool::Wait()
{
	ZoneScoped;

	while ( m_activeWorkers > 0 );
}

void WorkerPool::Worker( u32 index )
{
#ifdef _WIN32
	{
		TCHAR thread_name[ 32 ] = L"";
		wsprintf( thread_name, L"Worker %u", index );

		SetThreadDescription( GetCurrentThread(), thread_name );
	}
#endif

	u32 last_work_batch = 0;

	while ( !m_workersShouldStop )
	{
		if ( !m_workersCanStart )
		{
			std::this_thread::yield();
			continue;
		}
		
		m_activeWorkers++;
		while ( m_jobQueue.StartNextAvailableJob() );
		while ( m_workersCanStart ) std::this_thread::yield();
		m_activeWorkers--;
	}
}

}
