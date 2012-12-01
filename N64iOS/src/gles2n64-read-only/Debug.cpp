#include <stdio.h>
#include "glN64.h"
#include "Debug.h"
#include "RDP.h"
#include "RSP.h"

DebugInfo Debug;

BOOL DumpMessages;
FILE *dumpFile;
char dumpFilename[256];
void OpenDebugDlg()
{
	DumpMessages = FALSE;
}

void CloseDebugDlg()
{
}

void StartDump( char *filename )
{
	DumpMessages = TRUE;
	strcpy( dumpFilename, filename );
	dumpFile = fopen( filename, "w" );
	fclose( dumpFile );
}

void EndDump()
{
	DumpMessages = FALSE;
	fclose( dumpFile );
}
