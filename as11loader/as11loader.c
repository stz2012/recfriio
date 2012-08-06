/*-
 * Copyright (c) 2008 FUKAUMI Naoki.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usb_linux.h"

#define IDVENDOR 0x1738
#define IDPRODUCT 0x5211

int
main(int argc, char *argv[])
{
    char* buffer = NULL;
    size_t syssize;

    usb_dev_handle* ugen = NULL;
    uint8_t *firm;

	int16_t idx;
	int sys;

	if ((sys = open("SKNET_AS11Loader.sys", O_RDONLY)) == -1){
            fprintf( stderr, "could not open SKNET_AS11Loader.sys\n");
            goto ERROR;
        }

        syssize = 0x4000 + 0xa90;
        buffer = (char*)malloc( syssize );
        if( read( sys, buffer, syssize ) != syssize ){
            fprintf( stderr, "could not read SKNET_AS11Loader.sys\n");
            goto ERROR;
        }

        firm = buffer + 0xa88;
	if ((firm[0] == 0x66 && firm[1] == 0x4d) /* CD 1.0 */ ||
	    (firm[0] == 0x21 && firm[1] == 0x51) /* CD 1.1 */){
		idx = ((int16_t)(firm[1]) << 8) | firm[0];
                fprintf( stderr, "idx = 0x%x\n", idx );
        }
	else {
		fprintf(stderr, "unknown SKNET_AS11Loader.sys\n");
		exit(EXIT_FAILURE);
	}

        ugen = init_device( IDVENDOR, IDPRODUCT );
        if( ! ugen ) goto ERROR;

        firm = buffer + 0xa90;
	send_firm(ugen, 0xab, idx, firm, 0x0000, 0x0c00);
	send_firm(ugen, 0xab, idx, firm, 0x2000, 0x0400);
	send_firm(ugen, 0xab, idx, firm, 0x2800, 0x1000);
	send_firm(ugen, 0xac, idx, firm, 0x3800, 0x0800);

  ERROR:        

        close_device( ugen );
        if( buffer ) free( buffer );
	return EXIT_SUCCESS;
}
