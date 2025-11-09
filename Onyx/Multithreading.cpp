#include "Multithreading.h"

#include "tracy/Tracy.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

namespace onyx
{

IJob* JobQueue::GetNextJob( bool& any_unstarted_jobs )
{
	any_unstarted_jobs = false;

	for ( auto& [id, _job] : m_jobs )
	{
		if ( _job->hasFinished )
			continue;

		if ( !_job->hasStarted.exchange( true ) )
		{
			any_unstarted_jobs = true;

			bool can_start = true;
			for ( const IJob* dependency : _job->dependencies )
			{
				if ( !dependency->hasFinished )
					can_start = false;

				break;
			}

			if ( !can_start )
			{
				_job->hasStarted = false;
				continue;
			}

			return _job.get();
		}
	}

	return nullptr;
}

bool JobQueue::StartNextAvailableJob()
{
	bool any_unstarted_jobs;

	if ( IJob* const job = GetNextJob( any_unstarted_jobs ) )
	{
		job->Run();
		job->hasFinished = true;
	}

	return any_unstarted_jobs;
}

WorkerPool::WorkerPool( u32 num_workers )
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

	num_workers = std::min( num_workers, std::thread::hardware_concurrency() );

	m_workers.reserve( num_workers );
	for ( u32 idx = 0; idx < num_workers; ++idx )
		m_workers.push_back( std::jthread( [this, idx, num_workers] { Worker( idx, num_workers ); } ) );
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

	m_workersCanStart = std::min< u32 >( ( m_jobQueue.Count() + 1 ) / 2, m_workers.size() );

#ifdef _WIN32
	// tell the important threads to ramp up
	for ( u32 idx = 0; idx < m_workersCanStart; ++idx )
		SetThreadPriority( m_workers[ idx ].native_handle(), THREAD_PRIORITY_HIGHEST );
#endif

	while ( m_activeWorkers < m_workersCanStart );
	m_workersCanStart = 0;
}

void WorkerPool::Wait()
{
	ZoneScoped;

	while ( m_activeWorkers > 0 );
}

void WorkerPool::Worker( u32 index, u32 count )
{
#ifdef _WIN32
	{
		TCHAR thread_name[ 32 ] = L"";
		wsprintf( thread_name, L"Worker %u", index );

		SetThreadDescription( GetCurrentThread(), thread_name );
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_LOWEST );
	}
#endif

	while ( !m_workersShouldStop )
	{
		if ( index >= m_workersCanStart )
		{
			std::this_thread::yield();
			continue;
		}
		
		TracyMessageL( "Woke up" );
		m_activeWorkers++;

		while ( m_jobQueue.StartNextAvailableJob() );

		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_LOWEST );

		TracyMessageL( "Ready to sleep" );
		while ( index < m_workersCanStart ) std::this_thread::yield();

		TracyMessageL( "Back to sleep" );
		m_activeWorkers--;
	}
}

}
