/**************************SPMSAMPL.C*****************************************/


#define  INCL_NOPM
#define  INCL_DOS
#define  INCL_DOSERRORS
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "spmapi.h"
#include "spmctrgp.h"

#define INIT_LOG    TRUE    /* activate logging */
#define MAXDISKS    26

typedef struct _RETURN_AREA
{
   QUERYHDR    quHdr;
   CHAR        returnstr[10000];
}  RETURN_AREA;

VOID do_return (USHORT, PEXTENDEDRC, PVOID, USHORT, USHORT);
VOID do_data (PVOID, USHORT, USHORT);


#define MAX_TRIES          5    /* arbitrary */

VOID main (argc, argv)
INT  argc;
CHAR *argv[];
{


USHORT            rc;
CHAR              tmpstr[256];
CHAR              szLogFileName[256], szQFileName[256], szIlogName[256];
USHORT            n, counter;
BOOL              TERM_LOG, CLOSE_LOG;
FILTERSPEC        fsFilters;
RETURN_AREA       return1, return2;
USHORT            usLogRecLen;
PVOID             pbLogRec;
QUILOGSTAT        *pquilogstat;
LOGFILEHANDLE     LogHandle;
EXTENDEDRC        *pxrcRC, xrcRC;
USHORT            hFileHandle;
USHORT            AAction, ABytesWritten;
LONG              lNewFilePtr;

    n = 0;
    strcpy(tmpstr, "c:\\apilog.txt");
    while ((rc = DosOpen(tmpstr,&hFileHandle,&AAction,(ULONG) 0,
        (USHORT) 0x0000,(USHORT) 0x11,(USHORT) 0x0042,(ULONG) 0))
        && (++n < MAX_TRIES));
    if (rc)
    {
        printf ("\nc:\\apilog.txt was not created or failed to open\n");
        exit (1);
    }
    else
    {
        if (AAction == (USHORT)1)
            DosChgFilePtr(hFileHandle, 0L, 2, &lNewFilePtr);
        strcpy(tmpstr, "\n\0");
        DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);
    }

    /* initialize logfilename */
    strcpy ( szLogFileName,"aLogFile.log");

    /* initialize start and end times for logging and data get */
    /* realtime = 0e0 */
    if (INIT_LOG) tmpstr[0] = '\0';
    else strcpy(tmpstr,"01/06/1993:00:00:00");

    fsFilters.dtStartDateTime = '\0';
    if ((fsFilters.dtStartDateTime = str2dt(tmpstr)) == DTERROR)
    {
        if (tmpstr[0])
        {
           strcpy (tmpstr, "invalid Start parameter entered\n");
           DosWrite(hFileHandle, tmpstr, strlen(tmpstr), &ABytesWritten);
           exit(1);
        }
        else fsFilters.dtStartDateTime = '\0';
    }

    if (INIT_LOG) tmpstr[0] = '\0';
    else strcpy(tmpstr,"01/06/1993:09:15:00");
    fsFilters.dtEndDateTime = '\0';
    if ((fsFilters.dtEndDateTime = str2dt(tmpstr)) == DTERROR)
    {
        if (tmpstr[0])
        {
           strcpy (tmpstr, "invalid End parameter entered\n");
           DosWrite(hFileHandle, tmpstr, strlen(tmpstr), &ABytesWritten);
           exit(1);
        }
        else fsFilters.dtEndDateTime = '\0';
    }


    /* initialize space for record data to be returned to */
    usLogRecLen = SPM_TYPE10_MAX_SIZE;
    pbLogRec = (PVOID) malloc (usLogRecLen);


    if (INIT_LOG)
    {
        rc = SPMAPIInit((PSZ SPMPTR) &szLogFileName, -1,(PEXTENDEDRC) &xrcRC);
        if ((!rc) || (xrcRC.FunctionRC == SPMAPI_ERR_OBJECTINUSE))
        {
            if(xrcRC.FunctionRC == SPMAPI_ERR_OBJECTINUSE) TERM_LOG = FALSE;
            else TERM_LOG = TRUE;
            DosSleep(1000);
            n = 0;
            return1.quHdr.usResultMax = sizeof(return1);
            return1.quHdr.pbResult = return1.returnstr;
            while((n++ < MAX_TRIES) &&
                (rc = SPMAPIQuery((QUOPTION) SPM_QU_ILOG_LIST,
                (PSZ SPMPTR) &szLogFileName,(PVOID SPMPTR) &return1,
                (PEXTENDEDRC) &xrcRC)));
            if(!rc)
            {
                return2.quHdr.usResultMax = sizeof(return2);
                pquilogstat = (QUILOGSTAT *)return2.returnstr;
                szIlogName[0] = '\0';
                /* get path to logfile */
                rc = SPMAPIQualifyFileName ((PSZ SPMPTR) &szLogFileName,
                    (USHORT)256, (PSZ SPMPTR) &szQFileName,
                    (PEXTENDEDRC) &xrcRC);
            }
            n = 0;
            /* wait for logfile to activate */
            while ((return1.returnstr[n] != '\0') && (szIlogName[0] == '\0'))
            {
                counter = 0;
                while (return1.returnstr[n] != '\0')
                szIlogName[counter++] = return1.returnstr[n++];
                szIlogName[counter] = '\0';
                n++;
                if ((rc = SPMAPIQuery((QUOPTION) SPM_QU_ILOG_STAT,
                    (PSZ SPMPTR) &szIlogName,(PVOID SPMPTR) &return2,
                    (PEXTENDEDRC) &xrcRC)) ||
                    (strcmp(pquilogstat->szLogFileName,
                    szQFileName))) szIlogName[0] = '\0';
            }
        }
    }

    /* open logfile */
    rc = SPMAPIOpen((PSZ SPMPTR) &szLogFileName,
        (PFILTERSPEC) &fsFilters, (PLOGFILEHANDLE) &LogHandle,
        (PEXTENDEDRC) &xrcRC);
    /* error process here */

    /* get data from logfile */
    while((n++ < MAX_TRIES) &&
        (rc = SPMAPIGetData((PLOGFILEHANDLE) &LogHandle,
        (PVOID SPMPTR) pbLogRec, (USHORT) usLogRecLen,
        (PUSHORT SPMPTR) &usLogRecLen, (PVOID) NULL, (PVOID) NULL,
        (PEXTENDEDRC) pxrcRC)));
    do_return(rc, &xrcRC, pbLogRec, (USHORT)SPM_TYPE10_MAX_SIZE,
        hFileHandle);

   if (CLOSE_LOG)
   {
      rc = SPMAPIClose((PLOGFILEHANDLE) &LogHandle, (PEXTENDEDRC) &xrcRC);
   }

   if (TERM_LOG)
   {
      rc = SPMAPITerm((PSZ SPMPTR) &szLogFileName, (USHORT) SPMAPI_HALT,
         (PEXTENDEDRC) &xrcRC);

   }

   exit (0);

}

VOID do_return (USHORT rc, PEXTENDEDRC xrcRC, PVOID r_ptr, USHORT len,
   USHORT hFileHandle)
{
CHAR              tmpstr[256], numstr[80];
USHORT            ABytesWritten;


    /* process returns here */
    if(rc)
    {
        itoa (rc, numstr, 10);
        strncpy(tmpstr, "\nrc = \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 23);
        strcat (tmpstr, numstr);
        DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

        if (xrcRC != NULL)
        {
            itoa (xrcRC->FunctionCode, numstr, 10);
            strncpy(tmpstr, "\nFunctionCode = \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                32);
            strcat (tmpstr, numstr);
            itoa (xrcRC->FunctionRC, numstr, 10);
            strncat(tmpstr, "; FunctionRC = \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 30);
            strcat (tmpstr, numstr);
            DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);
        }
    }
    else if (r_ptr != NULL) do_data(r_ptr, len, hFileHandle);
    return;
}

VOID do_data (PVOID r_ptr, USHORT len, USHORT hFileHandle)
{
USHORT          ABytesWritten;
CHAR            tmpstr[256], numstr[80];
SHORT           n, count, offset, grpcnt;
PBYTE           pTop, pByte, pbBuf;
MEM             *pMem;
WKS             *pWKS;
CPU             *pCPU;
DSK             *pDSK;
ULONG           ulMemGroupTotalRAMPages;
ULONG           ulTotalRAMPages;
SHORT           sMemPerCent;
USHORT          usSwapIn, usSwapOut;
USHORT          usNotIdlePerCent;
USHORT          usGroupOrdinal;
USHORT          usTemp;
FPDATETIME      dtActualSamplePeriod, dtGetDataTimeStamp;
BOOL            fHaveValidTotalRAM;
SHORT           sWorkingSet;
USHORT          usWorkingPages;
SHORT           sFixMemPerCent;
DATARECHDR      *pHdr;
DCFGROUP        *pGRP;

   offset = 0;

   // Print HEADER
   n = 0;
   while(n < 256) tmpstr[n++] ='\0';
   pHdr = (DATARECHDR *) r_ptr;
   strcpy (tmpstr, "\nBasic Header:");
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   n = 0;
   while(n < 256) tmpstr[n++] ='\0';

   strcpy (tmpstr, "\n  usLL: ");
   itoa ((SHORT)(pHdr->bhDR.usLL), numstr, 10);
   strcat(tmpstr, numstr);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   n = 0;
   while(n < 256) tmpstr[n++] ='\0';

   strcpy (tmpstr, "\n  ucRecordSeries: ");
   ltoa ((LONG)(pHdr->bhDR.ucRecordSeries), numstr, 10);
   strcat(tmpstr,numstr);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   n = 0;
   while(n < 256) tmpstr[n++] ='\0';

   strcpy (tmpstr, "\n  ucRecordSubType: ");
   ltoa ((LONG)(pHdr->bhDR.ucRecordSubType), numstr, 10);
   strcat(tmpstr,numstr);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   n = 0;
   while(n < 256) tmpstr[n++] ='\0';
   strcpy (tmpstr, "\n  ucRecordType: ");
   ltoa ((LONG)(pHdr->bhDR.ucRecordType), numstr, 10);
   strcat(tmpstr,numstr);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   //Print nodename
   while(n < 256) tmpstr[n++] ='\0';
   strcpy (tmpstr, "\nszNodeName: ");
   strcat(tmpstr, pHdr->szNodeName);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   //Print Group Count
   n = 0;
   while(n < 256) tmpstr[n++] ='\0';
   strcpy (tmpstr, "\nusGrpCnt: ");
   grpcnt = (SHORT)(*(&(pHdr->usGrpCnt) + offset));
   itoa (grpcnt, numstr, 10);
   strcat(tmpstr, numstr);
   DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

   //Print each DCFGROUP and data
   if (grpcnt != 0)
   {
      n = 0;
      while(n < 256) tmpstr[n++] ='\0';
      strcpy (tmpstr, "\nDCFGROUP: ");
      DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);
      pByte = (PBYTE)r_ptr + sizeof(DATARECHDR) + 2*offset;
      dtActualSamplePeriod =((DATARECHDR*)pByte)->dtMedEnd - ((DATARECHDR*)pByte)->dtMedStart;
      dtGetDataTimeStamp = ((DATARECHDR*)pByte)->dtMedEnd;

      pTop = pByte;
   }

   for (count = 0; count < grpcnt; count++)
   {
      pGRP = (DCFGROUP *) pByte;

      //Print length
      n = 0;
      while(n < 256) tmpstr[n++] ='\0';
      strcpy (tmpstr, "\n  usLL: ");
      itoa ((SHORT)pGRP->usLL, numstr, 10);
      strcat(tmpstr, numstr);
      DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

      //Print ordinal
      n = 0;
      while(n < 256) tmpstr[n++] ='\0';
      strcpy (tmpstr, "\n  usSPMGrp: ");
      itoa (pGRP->usSPMGrp, numstr, 10);
      strcat(tmpstr, numstr);
      DosWrite(hFileHandle, tmpstr, strlen(tmpstr),&ABytesWritten);

      //Get data


    switch (pGRP->usSPMGrp)
    {
        case SPMOrdinal_MEM:
              pbBuf = (PBYTE) &(pGRP->byDataStart);
              pMem = (MEM *)pbBuf;

              /***************************************************************/
              /*          Calculate total pages for the MEM group            */
              /***************************************************************/
              ulMemGroupTotalRAMPages =
                 (ULONG)( pMem->qlQUsed.ct + pMem->qlQFree.ct );

              /***************************************************************/
              /*                   Calculate % used memory                   */
              /***************************************************************/
              if ( ulMemGroupTotalRAMPages > 0L )
                 sMemPerCent = (SHORT) ((ULONG)(pMem->qlQUsed.ct) * 100L /
                     ulMemGroupTotalRAMPages );
              else sMemPerCent = 0;

             /***************************************************************/
             /*                   Calculate paging                          */
             /***************************************************************/
             usSwapIn  = (USHORT)(pMem->ctSwapIn  * 10 / dtActualSamplePeriod );
             usSwapOut = (USHORT)(pMem->ctSwapOut * 10 / dtActualSamplePeriod );
             break;

        case SPMOrdinal_WKS:
            pbBuf = (PBYTE) &(pGRP->byDataStart);
            pWKS  = (WKS *)pbBuf;

            /***************************************************************/
            /*               Calculate working pages                       */
            /***************************************************************/
            usWorkingPages = (USHORT)(pWKS->qlQWorking.ct);

            /***************************************************************/
            /*                      Display total RAM                      */
            /***************************************************************/
            ulTotalRAMPages = (ULONG)( pWKS->ctMemSize );
            fHaveValidTotalRAM = TRUE;

            /***************************************************************/
            /*                    Calculate working set                    */
            /***************************************************************/
            if ( fHaveValidTotalRAM )
                sWorkingSet = (SHORT) ((ULONG)usWorkingPages * 100L /
                    ( ulTotalRAMPages ));
            else sWorkingSet = 0;

            /***************************************************************/
            /*                    Calculate % fixed memory                 */
            /***************************************************************/
            if ( fHaveValidTotalRAM )
                sFixMemPerCent = (SHORT) ((ULONG)pWKS->qlQResident.ct * 100L /
                    ( ulTotalRAMPages ));
            else sFixMemPerCent = 0;
            break;

        case SPMOrdinal_CPU:
            pbBuf = (PBYTE) &(pGRP->byDataStart);
            pCPU  = (CPU *)pbBuf;

            usNotIdlePerCent = (USHORT)( pCPU->tvNotIdle * 100.0 /
                dtActualSamplePeriod ) ;
            break;

        default:
            // DISK
            if (( usGroupOrdinal >= SPMOrdinal_DSK ) &&
                ( usGroupOrdinal <= (USHORT)( SPMOrdinal_DSK + MAXDISKS - 1 )))
            {
                pbBuf = (PBYTE)&(pGRP->byDataStart);
                pDSK  = (DSK *)pbBuf;
                if (( pDSK->tvBusy >= (timer)0.0 ) &&
                    ( pDSK->tvBusy <= (timer)dtActualSamplePeriod ))
                {
                    usTemp = (USHORT)(( pDSK->tvBusy ) * 100L /
                                 dtActualSamplePeriod ) ;
                }
            }
            break;

        } // end switch

        pTop += pGRP->usLL;
        pByte = pTop;
    }
    return;
}
