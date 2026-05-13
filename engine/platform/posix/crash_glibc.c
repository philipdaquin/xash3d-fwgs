/*
crash_glibc.c - advanced crashhandler based on glibc's execinfo API
Copyright (C) 2016 Mittorn
Copyright (C) 2025 Alibek Omarov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

// on Glibc (which potentially might not be only Linux) systems we
// have backtrace() and backtrace_symbols() calls, which replace for us
// platform-specific code
#if HAVE_EXECINFO
#include <execinfo.h>
#include <link.h>
#include <signal.h>
#include "common.h"
#include "input.h"
#include "crash.h"

static int Sys_PrintLibraryInfo( struct dl_phdr_info *info, size_t size, void *data )
{
	int logfd = *(int *)data;
	char line[512];
	const char *name = info->dlpi_name && info->dlpi_name[0] ? info->dlpi_name : "<main>";
	int len = Q_snprintf( line, sizeof( line ), "LIB: %s @ %p\n", name, (void *)info->dlpi_addr );

	(void)size;

	if( len > 0 )
	{
		write( logfd, line, len );
		write( STDERR_FILENO, line, len );
	}

	return 0;
}

void Sys_PrintLoadedLibraries( int logfd )
{
	const char header[] = "\n=== LOADED LIBRARIES ===\n";

	write( logfd, header, sizeof( header ) - 1 );
	write( STDERR_FILENO, header, sizeof( header ) - 1 );

	dl_iterate_phdr( Sys_PrintLibraryInfo, &logfd );
}

void Sys_PrintStackTrace( int logfd )
{
	enum { MAX_FRAMES = 128 };
	void *frames[MAX_FRAMES];
	int count = backtrace( frames, ARRAYSIZE( frames ));
	const char header[] = "\n=== C++ STACK TRACE ===\n";
	const char footer[] = "=== END STACK TRACE ===\n";

	write( logfd, header, sizeof( header ) - 1 );
	write( STDERR_FILENO, header, sizeof( header ) - 1 );

	if( count > 0 )
	{
		backtrace_symbols_fd( frames, count, logfd );
		if( logfd != STDERR_FILENO )
			backtrace_symbols_fd( frames, count, STDERR_FILENO );
	}
	else
	{
		const char note[] = "Crash: backtrace() returned no frames\n";
		write( logfd, note, sizeof( note ) - 1 );
		write( STDERR_FILENO, note, sizeof( note ) - 1 );
	}

	write( logfd, footer, sizeof( footer ) - 1 );
	write( STDERR_FILENO, footer, sizeof( footer ) - 1 );
}

int Sys_CrashDetailsExecinfo( int logfd, char *message, int len, size_t max_len )
{
	void *addrs[16];
	int size = backtrace( addrs, sizeof( addrs ) / sizeof( addrs[0] ));
	char **syms = backtrace_symbols( addrs, size );
	char note[128];
	int note_len;

	note_len = Q_snprintf( note, sizeof( note ), "Crash: execinfo stack trace follows (%d frame%s)\n", size, size == 1 ? "" : "s" );
	write( logfd, note, note_len );
	write( STDERR_FILENO, note, note_len );
	len += Q_snprintf( message + len, max_len - len, "%s", note );

	for( int i = 0; i < size && syms; i++ )
	{
		size_t symlen = Q_strlen( syms[i] );
		char ch = '\n';

		write( logfd, syms[i], symlen );
		write( logfd, &ch, 1 );

		write( STDERR_FILENO, syms[i], symlen );
		write( STDERR_FILENO, &ch, 1 );

		len += Q_snprintf( message + len, max_len - len, "%2d: %s\n", i, syms[i] );
	}

	if( size <= 0 || !syms )
	{
		note_len = Q_snprintf( note, sizeof( note ), "Crash: execinfo could not resolve any frames\n" );
		write( logfd, note, note_len );
		write( STDERR_FILENO, note, note_len );
		len += Q_snprintf( message + len, max_len - len, "%s", note );
	}

	return len;
}
#endif // HAVE_EXECINFO
