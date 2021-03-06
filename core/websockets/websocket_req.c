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
 *  WebSocket request definition body
 *
 * file contain all functitons related to websocket requests
 *
 *  @author PS (Pawel Stefanski)
 *  @date created 16/08/2017
 */

#include <core/types.h>
#include <core/nodes.h>
#include <libwebsockets.h>
#include "websocket_req.h"
#include <util/base64.h>
#include <time.h>

/**
 * create new WebsocketReq
 * 
 * @param id pointer to request ID as string
 * @param chunk number of chunk UNUSED
 * @param total number of all chunks which create message
 * @param data pointer to request data
 * @param datasize size of request data
 * @return pointer to new WebsocketReq when success, otherwise NULL
 */
WebsocketReq *WebsocketReqNew( char *id, int chunk __attribute__((unused)), int total, char *data, int datasize )
{
	WebsocketReq *req;
	
	if( ( req = FCalloc( 1, sizeof( WebsocketReq ) ) ) != NULL )
	{
		strcpy( req->wr_ID, id );
		DEBUG("[WebsocketReqNew] New request: %s datasize %d\n", req->wr_ID, datasize );
		req->wr_Total = total;
		req->wr_Chunks = 1;
		req->wr_ChunkSize = datasize;
		req->wr_TotalSize = req->wr_ChunkSize * total;
		req->wr_MessageSize = datasize;
		req->wr_CreatedTime = time( NULL );
		
		if( ( req->wr_Message = FMalloc( req->wr_TotalSize+1 ) ) != NULL )
		{
			memcpy( req->wr_Message, data, datasize );
			req->wr_Message[ req->wr_TotalSize ] = 0; // This allow us to see messages by using %s
		}
	}
	
	return req;
}

/**
 * create new WebsocketReq
 * 
 * @param data pointer to request data
 * @param datasize size of request data
 * @return pointer to new WebsocketReq when success, otherwise NULL
 */
WebsocketReq *WebsocketReqRawNew( char *data, int datasize )
{
	WebsocketReq *req;
	
	if( ( req = FCalloc( 1, sizeof( WebsocketReq ) ) ) != NULL )
	{
		DEBUG("[WebsocketReqMainNew] New request\n" );
		req->wr_MessageSize = datasize;
		req->wr_CreatedTime = time( NULL );
		
		if( ( req->wr_Message = FMalloc( datasize+1 ) ) != NULL )
		{
			memcpy( req->wr_Message, data, datasize );
			req->wr_Message[ datasize ] = 0; // This allow us to see messages by using %s
		}
	}
	
	return req;
}

/**
 * delete WebsocketReq
 * 
 * @param wr pointer to WebsocketReq structure which will be deleted
 */
void WebsocketReqDelete( WebsocketReq *wr )
{
	if( wr != NULL )
	{
		if( wr->wr_Message != NULL )
		{
			FFree( wr->wr_Message );
		}
		FFree( wr );
	}
}

/**
 * delete WebsocketReq list
 * 
 * @param wr pointer to WebsocketReq list which will be deleted
 */
void WebsocketReqDeleteAll( WebsocketReq *wr )
{
	while( wr != NULL )
	{
		WebsocketReq *remreq = wr;
		wr = (WebsocketReq *)wr->node.mln_Succ;
		
		WebsocketReqDelete( remreq );
	}
}

char *basedecode64( const char *bufcoded, int len, int *res );

/**
 * add chunk to WebsocketReq
 * 
 * @param req pointer to request
 * @param chunk number of chunk
 * @param data pointer to request data
 * @param datasize size of request data
 * @return pointer to new WebsocketReq when success, otherwise NULL
 */
WebsocketReq *WebsocketReqAddChunk( WebsocketReq *req, int chunk, char *data, int datasize )
{
	if( req != NULL )
	{
		req->wr_Chunks++;
		int pos = chunk * req->wr_ChunkSize;

		memcpy( &(req->wr_Message[ pos ]), data, datasize );
		req->wr_MessageSize += datasize;
		INFO("[WebsocketReqAddChunk] chunk added %d/%d datasize %d message size %d stored data in position %d last char %c\n", chunk, req->wr_Total, datasize, req->wr_MessageSize, pos, req->wr_Message[ (chunk * req->wr_ChunkSize)-1 ] );
		
		if( req->wr_Chunks == req->wr_Total )
		{
			int len = 0;
			//char *dst = basedecode64( req->wr_Message, req->wr_MessageSize, &len );
			char *dst = Base64Decode( (const unsigned char *)req->wr_Message, req->wr_MessageSize, &len );
			if( dst != NULL )
			{
				DEBUG("[WebsocketReqAddChunk] data delivered %d data decoded %d strlen of msg %d total size %d\n", req->wr_MessageSize, len, (int)strlen( req->wr_Message ), req->wr_TotalSize );
				
				FFree( req->wr_Message );
				req->wr_Message = dst;
				req->wr_MessageSize = len;
			}
			else
			{
				DEBUG("[WebsocketReqAddChunk] Base64 decoding fail!\n");
			}
			return req;
		}
	}
	return NULL;
}
