#ifndef POOL_MNGR_H
#define POOL_MNGR_H

#include "utils/SingleLock.h"
#include <list>
#include "utils/log.h"

#include "SDL/SDL.h"

static const unsigned int MAX_POOL_SIZE = 10;


template< class T > 
class CPoolMngr : public CCriticalSection
{
  typedef std::list< T* >   ObjList;
  typedef typename std::list< T* >::iterator ObjListIterator;

  unsigned int  m_nMaxPoolSize;
  ObjList 	m_oFree;
  SDL_sem*      m_pSemaphore;
public:
  CPoolMngr( unsigned int nMaxPoolSize = MAX_POOL_SIZE ) 
    : m_pSemaphore( SDL_CreateSemaphore( nMaxPoolSize ) )
  {
  }

  virtual ~CPoolMngr()
  {
    ResetPool();
    SDL_DestroySemaphore( m_pSemaphore );
  }

  void ResetPool()
  {
    CSingleLock guard( *this );
    ObjListIterator iter = m_oFree.begin();
    ObjListIterator endIter = m_oFree.end();

    while( iter != endIter )
    {
      delete *iter;
      m_oFree.erase( iter++ );
    }
    m_oFree.clear();
  }


  T* GetFromPool()
  {
    if( -1 == SDL_SemWait( m_pSemaphore ) )
    {
      return NULL;
    }

    CSingleLock guard( *this );

    T* pObj = FindObject();

    if( pObj ) 
    {
      return pObj;
    }

    return CreateObject();
  }

  void ReturnToPool( T* pObj )
  {
    CSingleLock guard( *this );

    m_oFree.push_back( pObj );
    SDL_SemPost( m_pSemaphore );
  }

protected:
  virtual void InitializeNewObject( T* pObject )
  {
  }

private:
  T* CreateObject()
  {
    T* pObj = new T();

    InitializeNewObject( pObj );
    if( !pObj || !pObj->Init() || !pObj->Validate())
    {
      CLog::Log ( LOGERROR, "Could not create Object" );
      if( pObj )
      {
        delete pObj;
      }
      return NULL;
    }

    return pObj;
  }

  T* FindObject()
  {
    while( !m_oFree.empty() ) {
      T* pObj = m_oFree.front();

      if( pObj && pObj->Validate() ) 
      {
        m_oFree.pop_front(); 

        return pObj;
      }
      else
      {
	// we do pop also here since the front contain invalid element,
        m_oFree.pop_front();
        // this object was allocated, so we must deallocate it
        delete pObj;
      }
    }

    return NULL;
  }
};

#endif 
