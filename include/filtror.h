#ifndef filtror_h
#define filtror_h

#include "cromwell_types.h"


#if INCLUDE_FILTROR

	typedef struct {
		DWORD m_dwBlocksFromPc;
		DWORD m_dwCountChecksumErrorsSeenFromPc;
		DWORD m_dwBlocksToPc;
		/* this member should be incremented by the 
		   higher-level protocol when expected response does not happen */
		DWORD m_dwCountTimeoutErrorsSeenToPc; 		                                     
	} BOOTFILTROR_CHANNEL_QUALITY_STATS;

	extern BOOTFILTROR_CHANNEL_QUALITY_STATS bfcqs;

		// helpers
	int BootFiltrorGetIncomingMessageLength(void);
	void BootFiltrorMarkIncomingMessageAsHavingBeenRead(void) ;
	bool BootFiltrorDoesPcHaveAMessageWaitingForItToRead(void);
	void BootFiltrorSetMessageLengthForPcToRead(WORD wChecksum, WORD wLength) ;
	int DumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
		// main functions
	int BootFiltrorSendArrayToPc(const BYTE * pba, WORD wLength);
	int BootFiltrorGetArrayFromPc( BYTE * pba, WORD wLengthMax);
	int BootFiltrorSendArrayToPcModal(const BYTE * pba, WORD wLength);
	int BootFiltrorSendStringToPcModal(const char *szFormat, ...);
		// alias
#endif

#endif /* #ifndef filtror_h */
