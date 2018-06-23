#include "realtime_srv/common/RealtimeSrvShared.h"





std::unique_ptr< World >	World::sInstance;

void World::StaticInit()
{
	sInstance.reset( new World() );
}


#ifdef IS_LINUX

World::World()
	: mGameObjects( new GameObjs )
{}

GameObjectsPtr World::GetGameObjects()
{
	MutexLockGuard lock( mutex_ );
	return mGameObjects;
}

void World::GameObjectsCOW()
{
	if ( !mGameObjects.unique() )
	{
		mGameObjects.reset( new GameObjs( *mGameObjects ) );
	}
	assert( mGameObjects.unique() );
}

void World::AddGameObject( GameObjPtr inGameObject )
{
	MutexLockGuard lock( mutex_ );
	GameObjectsCOW();
	mGameObjects->insert( inGameObject );
}

void World::RemoveGameObject( GameObjPtr inGameObject )
{
	MutexLockGuard lock( mutex_ );
	GameObjectsCOW();
	mGameObjects->erase( inGameObject );
}

void World::Update()
{
	vector< GameObjPtr > GameObjsToRem;
	GameObjectsPtr  tempGameObjects = GetGameObjects();

	for ( GameObjs::iterator go = tempGameObjects->begin();
		go != tempGameObjects->end(); ++go )
		//for (  auto& go : *tempGameObjects )
	{
		if ( !( *go )->DoesWantToDie() )
		{
			( *go )->Update();
		}
		else
		{
			GameObjsToRem.push_back( ( *go ) );
			( *go )->HandleDying();
		}
	}

	if ( GameObjsToRem.size() > 0 )
	{
		MutexLockGuard lock( mutex_ );
		GameObjectsCOW();
		for ( auto g : GameObjsToRem )
		{
			mGameObjects->erase( g );
		}
	}
}

#else //IS_LINUX

World::World()
{}

void World::AddGameObject( GameObjPtr inGameObject )
{
	mGameObjects.push_back( inGameObject );
	inGameObject->SetIndexInWorld( mGameObjects.size() - 1 );
}


void World::RemoveGameObject( GameObjPtr inGameObject )
{
	int index = inGameObject->GetIndexInWorld();

	int lastIndex = mGameObjects.size() - 1;
	if ( index != lastIndex )
	{
		mGameObjects[index] = mGameObjects[lastIndex];
		mGameObjects[index]->SetIndexInWorld( index );
	}

	inGameObject->SetIndexInWorld( -1 );

	mGameObjects.pop_back();
}


void World::Update()
{
	for ( int i = 0, c = mGameObjects.size(); i < c; ++i )
	{
		GameObjPtr go = mGameObjects[i];
		if ( !go->DoesWantToDie() )
		{
			go->Update();
		}
		if ( go->DoesWantToDie() )
		{
			RemoveGameObject( go );
			go->HandleDying();
			--i;
			--c;
		}
	}
}
#endif //IS_LINUX