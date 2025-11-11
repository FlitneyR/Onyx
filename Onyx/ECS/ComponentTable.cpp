#include "ComponentTable.h"

namespace onyx::ecs
{

IComponentTable::IIterator& IComponentTable::IIterator::operator ++()
{
	// we can't incremenet, we've walked off the table
	if ( m_page == m_table.End() )
		return *this;

	// get the next component in this page, if it exists
	m_index = m_page->GetNextOccupantIndex( m_index );
	if ( m_index < 16 )
		return *this;

	// if it doesn't, step to the next page, if this is off the table, return
	while ( ++m_page != m_table.End() && m_page->m_occupancy == 0 );
	if ( m_page == m_table.End() )
		return *this;

	// get the next component in this table
	m_index = m_page->GetNextOccupantIndex();
	return *this;
}

EntityID IComponentTable::IIterator::FindNextDirtyEntityID() const
{
	IPage* page = m_page;

	// we've reached the end of these components
	if ( page == m_table.End() )
		return NoEntity;

	// check if our current page has any dirty components after the current one
	if ( const u8 next_dirty = page->GetNextDirtyIndex( m_index ); next_dirty < 16 )
		return page->GetEntityID( next_dirty );

	// iterate through the pages until we reach the end, or one that has any dirty components
	while ( ++page != m_table.End() && page->m_dirty == 0 );

	// if we got to the end, there are no more dirty components
	if ( page == m_table.End() )
		return NoEntity;

	// Gentlemen, we got 'em
	return page->GetEntityID( page->GetNextDirtyIndex() );
}

EntityID IComponentTable::IIterator::GetNextEntityID() const
{
	if ( m_page == m_table.End() ) return NoEntity;
	
	if ( u8 next_index = m_page->GetNextOccupantIndex( m_index ); next_index < 16 )
		return m_page->GetEntityID( next_index );

	IPage* next_page = m_page + 1;
	while ( next_page != m_table.End() && next_page->m_occupancy == 0 )
		++next_page;

	if ( next_page == m_table.End() ) return NoEntity;

	return next_page->GetEntityID( next_page->GetNextOccupantIndex() );
}

void IComponentTable::IIterator::GoToNext()
{
	// don't risk dereferencing endPage
	if ( m_page == m_table.End() ) return;

	// go to the next occupant in this page, if one exists, then return
	if ( ( m_index = m_page->GetNextOccupantIndex( m_index ) ) < 16 ) return;

	// go to the next occupied page, if it is the end page, return
	while ( ++m_page != m_table.End() && m_page->m_occupancy == 0 );
	if ( m_page == m_table.End() ) return;

	// go to the next occupant in the new page
	m_index = m_page->GetNextOccupantIndex();
}

void IComponentTable::IIterator::GoTo( EntityID entity )
{
	// don't do anything if we've already reached the end of the array
	if ( m_page == m_table.End() )
		return;

	// extract the page id and index
	const u32 page_id = u32( entity ) & IPage::c_pageIdMask;
	const u8 idx = u32( entity ) & IPage::c_pageIndexMask;

	// walk through the page list to the matching page
	while ( m_page != m_table.End() && m_page->m_pageId < page_id )
		++m_page;

	// we reached the end and didn't find it
	if ( m_page == m_table.End() )
		return;

	// we walked as far as we could before over stepping, but didn't find the component
	// so go to the last component in this page
	if ( m_page->m_pageId != page_id )
	{
		m_index = m_page->GetNextOccupantIndex();
		return;
	}

	// we found the right page!
	m_index = idx;
}

}