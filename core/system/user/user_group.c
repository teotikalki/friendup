/*©mit**************************************************************************
*                                                                              *
* This file is part of FRIEND UNIFYING PLATFORM.                               *
* Copyright 2014-2017 Friend Software Labs AS                                  *
*                                                                              *
* Permission is hereby granted, free of charge, to any person obtaining a copy *
* of this software and associated documentation files (the "Software"), to     *
* deal in the Software without restriction, including without limitation the   *
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  *
* sell copies of the Software, and to permit persons to whom the Software is   *
* furnished to do so, subject to the following conditions:                     *
*                                                                              *
* The above copyright notice and this permission notice shall be included in   *
* all copies or substantial portions of the Software.                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
* MIT License for more details.                                                *
*                                                                              *
*****************************************************************************©*/
/** @file
 * 
 *  User Group
 *
 * file contain funuctions related to user groups
 *
 *  @author PS (Pawel Stefanski)
 *  @date created 11/2016
 */

#include "user_group.h"
#include <system/fsys/device_handling.h>

/**
 * Create new User Group
 *
 * @param id unique value which will point to UserGroup
 * @param name of new UserGroup
 * @param uid id of user assigned to group
 * @param type type of group as string
 * @return new UserGroup structure when success, otherwise NULL
 */
UserGroup *UserGroupNew( FULONG id, char *name, FULONG uid, char *type )
{
	UserGroup *ug = NULL;
	
	if( ( ug = FCalloc( 1, sizeof( UserGroup ) ) ) != NULL )
	{
		int len = strlen( name );
		
		ug->ug_ID = id;
		int len10 = len + 10;
		
		ug->ug_Name = FCalloc( len10, sizeof(char) );
		if( ug->ug_Name != NULL )
		{
			strncpy( ug->ug_Name, name, len );
		}
	
		ug->ug_UserID = uid;
	
		len = strlen( type );
		ug->ug_Type = FCalloc( len10, sizeof(char) );
		if( ug->ug_Type != NULL )
		{
			strncpy( ug->ug_Type, type, len );
		}
	}
	
	return ug;
}

/**
 * Delete UserGroup
 *
 * @param sb pointer to SystemBase
 * @param ug pointer to UserGroup which will be deleted
 * @return 0 when success, otherwise error number
 */
int UserGroupDelete( void *sb, UserGroup *ug )
{
	if( ug != NULL )
	{
		// remove connection to users
		
		UserGroupAUser *au = ug->ug_UserList;
		UserGroupAUser *remau;
		while( au != NULL )
		{
			remau = au;
			au = (UserGroupAUser *)au->node.mln_Succ;
			
			FFree( remau );
		}
		
		// Remove all mounted devices
		File *lf = ug->ug_MountedDevs;
		File *remdev = lf;
		while( lf != NULL )
		{
			remdev = lf;
			lf = (File *)lf->node.mln_Succ;
			
			if( remdev != NULL )
			{
				DeviceRelease( sb, remdev );

				FileDelete( remdev );
				remdev = NULL;
			}
		}

		if( ug->ug_Name != NULL )
		{
			FFree( ug->ug_Name );
		}
		
		if( ug->ug_Type != NULL )
		{
			FFree( ug->ug_Type );
		}
		
		FFree( ug );
	}
	return 0;
}

/**
 * Delete all UserGroups from list
 *
 * @param sb pointer to SystemBase
 * @param ug pointer to root of UserGroup list which will be deleted
 * @return 0 when success, otherwise error number
 */
int UserGroupDeleteAll( void *sb, UserGroup* ug)
{
	UserGroup *rem = ug;
	UserGroup *next = ug;
	
	while( next != NULL )
	{
		rem = next;
		next = (UserGroup *)next->node.mln_Succ;
		
		UserGroupDelete( sb, rem );
	}
	
	return 0;
}

/**
 * Remove device by name from UserGroup
 *
 * @param ugrlist pointer to first UserGroup list root
 * @param name pointer to name of device which will be removed
 * @param error pointer to int variable where error number will be stured
 * @return pointer to File structure which will be removed from User structure
 */
File *UserGroupRemDeviceByName( UserGroup *ugrlist, const char *name, int *error )
{
	if( name != NULL )
	{
		while( ugrlist != NULL )
		{
			File *lf = ugrlist->ug_MountedDevs;
			File *lastone = NULL;
			File *remdev = NULL;
			if( lf == NULL )
			{
				FERROR( "[UserGroupRemDeviceByName] Seems we have NO mounted devs for usergroup %s!\n", ugrlist->ug_Name );
				continue;
			}

			while( lf != NULL )
			{
				DEBUG( "[UserGroupRemDeviceByName] Checking fs in list %s == %s...\n", lf->f_Name, name );
				if( strcmp( lf->f_Name, name ) == 0 )
				{
					DEBUG( "[UserGroupRemDeviceByName] Found one (%s == %s)\n", lf->f_Name, name );
					remdev = lf;
					break;
				}
				lastone = lf;
				lf = (File *)lf->node.mln_Succ;
			}
		
			if( remdev != NULL )
			{
				if( remdev->f_Operations <= 0 )
				{
					DEBUG("[UserGroupRemDeviceByName] Remove device from list\n");

					if( ugrlist->ug_MountedDevs == remdev )		// checking if its our first entry
					{
						File *next = (File*)remdev->node.mln_Succ;
						ugrlist->ug_MountedDevs = (File *)next;
						if( next != NULL )
						{
							next->node.mln_Pred = NULL;
						}
					}
					else
					{
						File *next = (File *)remdev->node.mln_Succ;
						//next->node.mln_Pred = (struct MinNode *)prev;
						if( lastone != NULL )
						{
							lastone->node.mln_Succ = (struct MinNode *)next;
						}
					}
					return remdev;
				}
				else
				{
					*error = FSys_Error_OpsInProgress;
					return remdev;
				}
			
				ugrlist = (UserGroup *)ugrlist->node.mln_Succ;
			}
		}
	}
	else
	{
		DEBUG("[UserGroupRemDeviceByName] User paramter is equal to NULL\n");
		return NULL;
	}
	return NULL;
}

/**
 * Add user to UserGroup users list
 *
 * @param ug pointer to UserGroup to which user will be added
 * @param u pointer to User structure which will be added to list
 * @return 0 when ssuccess, otherwise error number
 */
int UserGroupAddUser( UserGroup *ug, void *u )
{
	User *locu = (User *)u;
	UserGroupAUser *au = ug->ug_UserList;
	while( au != NULL )
	{
		// user is added, no need to add it second time
		if( au->ugau_User != NULL && locu->u_ID == au->ugau_UserID )
		{
			return 0;
		}
		au = (UserGroupAUser *)au->node.mln_Succ;
	}
	
	if( ( au = (UserGroupAUser *) FCalloc( 1, sizeof( UserGroupAUser ) ) ) != NULL )
	{
		au->node.mln_Succ = (MinNode *)ug->ug_UserList;
		au->ugau_UserID = locu->u_ID;
		au->ugau_User = locu;
		ug->ug_UserList = au;
	}
	else
	{
		return 1;
	}
	
	return 0;
}

/**
 * Remove user from UserGroup users list
 *
 * @param ug pointer to UserGroup to which user will be removed
 * @param u pointer to User structure which will be removed from list
 * @return 0 when ssuccess, otherwise error number
 */
int UserGroupRemoveUser( UserGroup *ug, void *u )
{
	User *locu = (User *)u;
	
	UserGroupAUser *au = ug->ug_UserList;
	UserGroupAUser *auprev = ug->ug_UserList;
	
	while( au != NULL )
	{
		// user is added, no need to add it second time
		if( au->ugau_User != NULL && locu->u_ID == au->ugau_UserID )
		{
			if( au == ug->ug_UserList )
			{
				ug->ug_UserList = (UserGroupAUser *) au->node.mln_Succ;
			}
			else
			{
				auprev->node.mln_Succ = au->node.mln_Succ;
			}
			FFree( au );
			return 0;
		}

		auprev = au;
		au = (UserGroupAUser *)au->node.mln_Succ;
	}
	return 1;
}
