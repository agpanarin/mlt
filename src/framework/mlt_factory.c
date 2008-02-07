/*
 * mlt_factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mlt.h"
#include "mlt_repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREFIX_LIB LIBDIR "/mlt"
#define PREFIX_DATA PREFIX "/share/mlt"

/** Singleton repositories
*/

static char *mlt_directory = NULL;
static mlt_properties global_properties = NULL;
static mlt_repository repository = NULL;
static mlt_properties event_object = NULL;
static int unique_id = 0;

/** Event transmitters.
*/

static void mlt_factory_create_request( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( char * )args[ 0 ], ( char * )args[ 1 ], ( mlt_service * )args[ 2 ] );
}

static void mlt_factory_create_done( mlt_listener listener, mlt_properties owner, mlt_service this, void **args )
{
	if ( listener != NULL )
		listener( owner, this, ( char * )args[ 0 ], ( char * )args[ 1 ], ( mlt_service )args[ 2 ] );
}

/** Construct the factories.
*/

mlt_repository mlt_factory_init( const char *directory )
{
	// Only initialise once
	if ( mlt_directory == NULL )
	{
		// Allow user over rides
		if ( directory == NULL || !strcmp( directory, "" ) )
			directory = getenv( "MLT_REPOSITORY" );

		// If no directory is specified, default to install directory
		if ( directory == NULL )
			directory = PREFIX_LIB;

		// Store the prefix for later retrieval
		mlt_directory = strdup( directory );

		// Initialise the pool
		mlt_pool_init( );

		// Create and set up the events object
		event_object = mlt_properties_new( );
		mlt_events_init( event_object );
		mlt_events_register( event_object, "producer-create-request", ( mlt_transmitter )mlt_factory_create_request );
		mlt_events_register( event_object, "producer-create-done", ( mlt_transmitter )mlt_factory_create_done );
		mlt_events_register( event_object, "filter-create-request", ( mlt_transmitter )mlt_factory_create_request );
		mlt_events_register( event_object, "filter-create-done", ( mlt_transmitter )mlt_factory_create_done );
		mlt_events_register( event_object, "transition-create-request", ( mlt_transmitter )mlt_factory_create_request );
		mlt_events_register( event_object, "transition-create-done", ( mlt_transmitter )mlt_factory_create_done );
		mlt_events_register( event_object, "consumer-create-request", ( mlt_transmitter )mlt_factory_create_request );
		mlt_events_register( event_object, "consumer-create-done", ( mlt_transmitter )mlt_factory_create_done );

		// Create the global properties
		global_properties = mlt_properties_new( );

		// Create the repository of services
		repository = mlt_repository_init( directory );

		// Force a clean up when app closes
		atexit( mlt_factory_close );
	}

	// Allow property refresh on a subsequent initialisation
	if ( global_properties != NULL )
	{
		mlt_properties_set_or_default( global_properties, "MLT_NORMALISATION", getenv( "MLT_NORMALISATION" ), "PAL" );
		mlt_properties_set_or_default( global_properties, "MLT_PRODUCER", getenv( "MLT_PRODUCER" ), "fezzik" );
		mlt_properties_set_or_default( global_properties, "MLT_CONSUMER", getenv( "MLT_CONSUMER" ), "sdl" );
		mlt_properties_set( global_properties, "MLT_TEST_CARD", getenv( "MLT_TEST_CARD" ) );
		mlt_properties_set_or_default( global_properties, "MLT_PROFILE", getenv( "MLT_PROFILE" ), "dv_pal" );
		mlt_properties_set_or_default( global_properties, "MLT_DATA", getenv( "MLT_DATA" ), PREFIX_DATA );
	}


	return repository;
}

/** Fetch the events object.
*/

mlt_properties mlt_factory_event_object( )
{
	return event_object;
}

/** Fetch the module directory used in this instance.
*/

const char *mlt_factory_directory( )
{
	return mlt_directory;
}

/** Get a value from the environment.
*/

char *mlt_environment( const char *name )
{
	if ( global_properties )
		return mlt_properties_get( global_properties, name );
	else
		return NULL;
}

/** Set a value in the environment.
*/

int mlt_environment_set( const char *name, const char *value )
{
	if ( global_properties )
		return mlt_properties_set( global_properties, name, value );
	else
		return -1;
}

static void set_common_properties( mlt_properties properties, mlt_profile profile, const char *type, const char *service )
{
	mlt_properties_set_int( properties, "_unique_id", ++ unique_id );
	mlt_properties_set( properties, "mlt_type", type );
	if ( mlt_properties_get_int( properties, "_mlt_service_hidden" ) == 0 )
		mlt_properties_set( properties, "mlt_service", service );
	if ( profile != NULL )
		mlt_properties_set_data( properties, "_profile", profile, 0, NULL, NULL );
}

/** Fetch a producer from the repository.
*/

mlt_producer mlt_factory_producer( mlt_profile profile, const char *service, void *input )
{
	mlt_producer obj = NULL;

	// Pick up the default normalising producer if necessary
	if ( service == NULL )
		service = mlt_environment( "MLT_PRODUCER" );

	// Offer the application the chance to 'create'
	mlt_events_fire( event_object, "producer-create-request", service, input, &obj, NULL );

	// Try to instantiate via the specified service
	if ( obj == NULL )
	{
		obj = mlt_repository_create( repository, profile, producer_type, service, input );
		mlt_events_fire( event_object, "producer-create-done", service, input, obj, NULL );
		if ( obj != NULL )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( obj );
			set_common_properties( properties, profile, "producer", service );
		}
	}
	return obj;
}

/** Fetch a filter from the repository.
*/

mlt_filter mlt_factory_filter( mlt_profile profile, const char *service, void *input )
{
	mlt_filter obj = NULL;

	// Offer the application the chance to 'create'
	mlt_events_fire( event_object, "filter-create-request", service, input, &obj, NULL );

	if ( obj == NULL )
	{
   		obj = mlt_repository_create( repository, profile, filter_type, service, input );
		mlt_events_fire( event_object, "filter-create-done", service, input, obj, NULL );
	}

	if ( obj != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( obj );
		set_common_properties( properties, profile, "filter", service );
	}
	return obj;
}

/** Fetch a transition from the repository.
*/

mlt_transition mlt_factory_transition( mlt_profile profile, const char *service, void *input )
{
	mlt_transition obj = NULL;

	// Offer the application the chance to 'create'
	mlt_events_fire( event_object, "transition-create-request", service, input, &obj, NULL );

	if ( obj == NULL )
	{
   		obj = mlt_repository_create( repository, profile, transition_type, service, input );
		mlt_events_fire( event_object, "transition-create-done", service, input, obj, NULL );
	}

	if ( obj != NULL )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( obj );
		set_common_properties( properties, profile, "transition", service );
	}
	return obj;
}

/** Fetch a consumer from the repository
*/

mlt_consumer mlt_factory_consumer( mlt_profile profile, const char *service, void *input )
{
	mlt_consumer obj = NULL;

	if ( service == NULL )
		service = mlt_environment( "MLT_CONSUMER" );

	// Offer the application the chance to 'create'
	mlt_events_fire( event_object, "consumer-create-request", service, input, &obj, NULL );

	if ( obj == NULL )
	{
		obj = mlt_repository_create( repository, profile, consumer_type, service, input );
		mlt_events_fire( event_object, "consumer-create-done", service, input, obj, NULL );
	}

	if ( obj != NULL )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( obj );
		set_common_properties( properties, profile, "consumer", service );
	}
	return obj;
}

/** Register an object for clean up.
*/

void mlt_factory_register_for_clean_up( void *ptr, mlt_destructor destructor )
{
	char unique[ 256 ];
	sprintf( unique, "%08d", mlt_properties_count( global_properties ) );
	mlt_properties_set_data( global_properties, unique, ptr, 0, destructor, NULL );
}

/** Close the factory.
*/

void mlt_factory_close( )
{
	if ( mlt_directory != NULL )
	{
		mlt_properties_close( event_object );
		mlt_properties_close( global_properties );
		mlt_repository_close( repository );
		free( mlt_directory );
		mlt_directory = NULL;
		mlt_pool_close( );
	}
}
