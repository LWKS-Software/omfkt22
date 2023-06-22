// omfVplayView.cpp : implementation of the COmfVplayView class
//

#ifdef AVID_CODEC_SUPPORT
	#define AVID_CODEC_SUPPORT 1
#endif

#include "stdafx.h"
#include "omfVplay.h"

#include "omfVplayDoc.h"
#include "omfVplayView.h"

#include "omFile.h"
#include "omMobGet.h"
#include "omMedia.h"


#include "omcAvJPED.h"
#include "omcAvidJFIF.h"
#include "omMobBld.h"
#include "omCvt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define COLORS_PER_PIXELS 4
/////////////////////////////////////////////////////////////////////////////
// COmfVplayView construction/destruction
COmfVplayView::COmfVplayView()
	{
	m_buf =				NULL;	// single global image in memory

//OMF STUFF	
	sessionPtr =		NULL;
	filePtr =			NULL;

	mobIter =			NULL;
	trackIter =			NULL;

	frameLayout =		kFullFrame;//these could be set from command line --
	outputFrameLayout =	kFullFrame;//see omfVplay in the apps directory for the
	stripBlank =		TRUE;//MAC/UNIX version

	status =			OM_ERR_NONE;
//	search;

	
	XPROTECT(NULL)
		{
		CHECK(omfsBeginSession(NULL, &sessionPtr));
		
		CHECK(omfmInit(sessionPtr));
#if AVID_CODEC_SUPPORT
		CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvJPED, kOMFRegisterLinked));
		CHECK(omfmRegisterCodec(sessionPtr, omfCodecAvidJFIF, kOMFRegisterLinked));
#endif
		}//XPROTECT(NULL)
	XEXCEPT
		{
		}//XEXCEPT
	XEND_VOID;
	}//COmfVplayView


/////////////////////////////////////////////////////////////////////////////
COmfVplayView::~COmfVplayView()
	{
	XPROTECT(NULL)
		{
		if (trackIter)
			{
			CHECK(omfiIteratorDispose(filePtr, trackIter));
			trackIter =	NULL;
			}//if
		if (mobIter)
			{
			CHECK(omfiIteratorDispose(filePtr, mobIter));
			mobIter =	NULL;
			}//if
		if (filePtr)
			{
			CHECK(omfsCloseFile(filePtr));
			filePtr =	NULL;
			}//if
		if (sessionPtr)
			{
			CHECK(omfsEndSession(sessionPtr));
			sessionPtr =	NULL;
			}//if
		}//XPROTECT(NULL)
	XEXCEPT
		{
		if (trackIter)
			{
			omfiIteratorDispose(filePtr, trackIter);
			trackIter =	NULL;
			}//if
		if (mobIter)
			{
			omfiIteratorDispose(filePtr, mobIter);
			mobIter =	NULL;
			}//if
		if (filePtr)
			{
			omfsCloseFile(filePtr);
			filePtr =	NULL;
			}//if
		if (sessionPtr)
			{
			omfsEndSession(sessionPtr);
			sessionPtr =	NULL;
			}//if
		}//XEXCEPT
	XEND_VOID;

	// clean up
	if (m_buf!=NULL)
		{
		free(m_buf);
		m_buf =	NULL;
		}//if
	}//~COmfVplayView

/////////////////////////////////////////////////////////////////////////////
// COmfVplayView message handlers
////////////////////////////////////////////////////////////////////////////
void COmfVplayView::OnFileOpen()
	{
	omfObject_t	masterMob;

	CString		fileName;
	char		*filename =	NULL;

	CString		filt =		"OMF File (*.OMF)|*.OMF|All files (*.*)|*.*||";

	CFileDialog	fileDlg(TRUE,"*.OMF","*.OMF",NULL,filt,this);

	fileDlg.m_ofn.Flags |=		OFN_FILEMUSTEXIST;
	fileDlg.m_ofn.lpstrTitle =	"File to load";

	if (fileDlg.DoModal()==IDOK)
		{
		AfxGetApp()->DoWaitCursor(1);

		fileName =	fileDlg.GetPathName();

		filename =	(char *)(const char *) fileName;//fileName is a CString

		omfsOpenFile((fileHandleType)filename, sessionPtr, &filePtr);

#if OMFI_ENABLE_SEMCHECK
		omfsSemanticCheckOff(filePtr);
#endif		
		if (mobIter)
			{
			omfiIteratorDispose(filePtr, mobIter);
			mobIter =	NULL;
			}//if

		omfiIteratorAlloc(filePtr, &mobIter);
		
		search.searchTag =		kByMobKind;
		search.tags.mobKind =	kMasterMob;
		status = omfiGetNextMob(mobIter, &search, &masterMob);
		if (mobIter)
			{
			omfiIteratorDispose(filePtr, mobIter);
			mobIter =	NULL;
			}//if
		AfxGetApp()->DoWaitCursor(-1);

		RedrawWindow();
		PlayOMF(masterMob);
		}//if
	}//OnFileOpen


////////////////////////////////////////////////////////////////////////////
//	read a OMF to our global buffer
void COmfVplayView::PlayOMF(omfObject_t	masterMob)
	{
	omfMediaHdl_t		mediaPtr;
	omfRational_t		editRate;
	omfPixelFormat_t	pixelFormat =		kOmfPixNone;
	omfInt16			bitsPerPixel =		0;
	omfDDefObj_t		pictureKind;
	omfVideoMemOp_t		memFmt[6];
	omfInt16			numCh =				0;
	omfNumTracks_t		numTracks =			0;
	char				name[64];
	omfSearchCrit_t		searchCrit;
	omfMSlotObj_t		track;
	omfTrackID_t		trackID =			0;
	omfRect_t			displayRect;
	long				bsize =				0;


	// m_buf is the global buffer
	if (m_buf!=NULL)
		{
		free(m_buf);
		m_buf=NULL;
		}//if

	XPROTECT(NULL)
		{
		while((status == OM_ERR_NONE) && (masterMob != NULL))
			{
			omfiDatakindLookup(filePtr, PICTUREKIND, &pictureKind, &status);

			CHECK(omfiMobGetNumTracks(filePtr, masterMob, &numTracks));
			CHECK(omfiIteratorAlloc(filePtr, &trackIter));
			searchCrit.searchTag =	kByDatakind;
			strcpy( (char*) searchCrit.tags.datakind, PICTUREKIND);
			for(long n = 1; n <= numTracks; n++)
				{
			
				if(omfiMobGetNextTrack(trackIter, masterMob, &searchCrit, 
									   &track) == OM_ERR_NO_MORE_OBJECTS)
					{
					break;
					}//if
					
				CHECK(omfiTrackGetInfo(filePtr, masterMob, track, NULL, 0, 
									   NULL, NULL, &trackID, NULL));
				CHECK(omfmGetNumChannels(	
							filePtr,		/* IN -- For this file */
							masterMob,		/* IN -- In this fileMob */
							trackID,
							NULL,
							pictureKind,	/* IN -- for this media type */
							&numCh));		/* OUT -- How many channels? */
				if(numCh != 0)
					{
					CHECK(omfiMobGetInfo(filePtr, masterMob, NULL, 
										 sizeof(name), name, NULL, NULL));
	
					status = omfmMediaOpen(filePtr, masterMob, 
								trackID, NULL, kMediaOpenReadOnly,
								kToolkitCompressionEnable, &mediaPtr);
					if(status == OM_ERR_MEDIA_NOT_FOUND)
						{
						continue;
						}//if
					else if(status != OM_ERR_NONE)
						{
						RAISE(status);
						}//else if
							
					CHECK(omfmGetVideoInfo(mediaPtr,
									&frameLayout,
									&m_Width,
									&m_Height,
									&editRate,
									&bitsPerPixel,
									&pixelFormat));
				
					if(stripBlank)
						{
						CHECK(omfmGetDisplayRect(mediaPtr, &displayRect));
						m_DisplayHeight =	m_Height - displayRect.yOffset;
						}//if
					else
						{
						m_DisplayHeight =	m_Height;
						}//else
					
					if(outputFrameLayout != kOneField)
						{
						if((frameLayout == kSeparateFields) ||
							(frameLayout == kMixedFields) ||
							(frameLayout == kOneField))
							{
							m_Height *=		2;
							m_DisplayHeight *=	2;
							}//if
						}//if
					else if(frameLayout == kFullFrame)
						{
						m_Height /=		2;
						m_DisplayHeight /=	2;
						}//else if

					/* Ask for the media as one frame (re-interlace if needed)
					 */
					memFmt[0].opcode =					kOmfFrameLayout;
					memFmt[0].operand.expFrameLayout =	outputFrameLayout;
					
					memFmt[1].opcode =					kOmfPixelFormat;
					memFmt[1].operand.expPixelFormat =	kOmfPixRGBA;
										
					memFmt[2].opcode =					kOmfRGBCompLayout;
					memFmt[2].operand.expCompArray[0] =	'B';
					memFmt[2].operand.expCompArray[1] =	'G';
					memFmt[2].operand.expCompArray[2] =	'R';
					memFmt[2].operand.expCompArray[3] =	'A';
					memFmt[2].operand.expCompArray[4] =	0;

					memFmt[3].opcode =						kOmfRGBCompSizes;
					memFmt[3].operand.expCompSizeArray[0] =	8;
					memFmt[3].operand.expCompSizeArray[1] =	8;
					memFmt[3].operand.expCompSizeArray[2] =	8;
					memFmt[3].operand.expCompSizeArray[3] =	8;
					memFmt[3].operand.expCompSizeArray[4] =	0;
					
					memFmt[4].opcode =	kOmfVFmtEnd;

			
					CHECK(omfmSetVideoMemFormat(mediaPtr, memFmt));
					
					bsize =	m_Height * m_Width * COLORS_PER_PIXELS;
					if (m_buf)
						{
						free(m_buf);
						}//if

					m_buf =	(BYTE *) malloc((sizeof(BYTE *)) * bsize);//BYTE -- unsigned char					
					if(m_buf == NULL)
						{
						RAISE(OM_ERR_NOMEMORY);
						}//if
					memset(m_buf, 0, ((sizeof(BYTE *)) * bsize));////BYTE -- unsigned char


					AfxGetApp()->DoWaitCursor(1);
					PlayTrack(mediaPtr);
					AfxGetApp()->DoWaitCursor(-1);
					}//if
				}//for
			CHECK(omfiIteratorDispose(filePtr, trackIter));
			trackIter =	NULL;
			status =	omfiGetNextMob(mobIter, &search, &masterMob);
			}//while
		}//XPROTECT
	XEXCEPT
		{
		if (trackIter)
			{
			omfiIteratorDispose(filePtr, trackIter);
			trackIter =	NULL;
			}//if
		if (m_buf!=NULL)
			{
			free(m_buf);
			m_buf =	NULL;
			}//if
		}//XEXCEPT
	XEND_VOID;
	}//PlayOMF

////////////////////////////////////////////////////////////////////////////
omfErr_t COmfVplayView::PlayTrack(omfMediaHdl_t mediaPtr)
	{
	omfUInt32		bytesRead;
	omfLength_t		numSamples;
	omfInt32		numSamples32;
	omfInt32		loop;
	long			bsize =				0;
	
	XPROTECT(NULL)
		{
		CHECK(omfmGetSampleCount(mediaPtr, &numSamples));
		omfsTruncInt64toInt32(numSamples, &numSamples32);
		bsize = m_Height * m_Width * COLORS_PER_PIXELS;

		for (loop = 1; (loop <= numSamples32); loop++)
			{
			CHECK(omfmReadDataSamples(mediaPtr,	/* IN -- */
										1,	/* IN -- */
										bsize,	/* IN -- */
										(void *)m_buf,	/* IN/OUT -- */
										&bytesRead));	/* OUT -- */


			DrawBuffer();
			}//for
		}
	XEXCEPT
	XEND;
	return(OM_ERR_NONE);
	}//PlayTrack

////////////////////////////////////////////////////////////////////////////
//		Drawing Functions
////////////////////////////////////////////////////////////////////////////
void COmfVplayView::DrawBuffer()
	{
	if (m_buf==NULL)
		{
		return;
		}//if


	// vertical flip for display
	VertFlipBuf();


	PAINTSTRUCT ps;
	::BeginPaint(m_hWnd, &ps);

	HDC hDC =	::GetDC(m_hWnd);

	ASSERT(hDC!=NULL);

	HDC hMemDC=CreateCompatibleDC(hDC);
	
	// set up a DIB 
	BITMAPINFO	bmi;
	ZeroMemory(&bmi.bmiHeader,sizeof(BITMAPINFOHEADER)); 
	bmi.bmiHeader.biSize =			sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth =			m_Width;
	bmi.bmiHeader.biHeight =		m_DisplayHeight;
	bmi.bmiHeader.biPlanes =		1;
	bmi.bmiHeader.biBitCount =		32;
	bmi.bmiHeader.biCompression =	BI_RGB;
	bmi.bmiHeader.biSizeImage =		0;
	bmi.bmiHeader.biXPelsPerMeter =	72;
	bmi.bmiHeader.biYPelsPerMeter =	72;
	bmi.bmiHeader.biClrUsed =		0;
	bmi.bmiHeader.biClrImportant =	0;

	DWORD	*pclrBits =	NULL; //same as COLORREF
	HBITMAP	hbm; 
	hbm =	CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, 
						(void**)&pclrBits, NULL, 0); 

	memcpy((pclrBits), m_buf, ((m_Width)*(m_DisplayHeight)*sizeof(DWORD)));

	SelectObject(hMemDC, hbm); 

	BitBlt(hDC,0,0, m_Width, m_DisplayHeight, hMemDC, 0,0, SRCCOPY);


	::ReleaseDC(m_hWnd, hDC);
	DeleteDC(hMemDC);
	DeleteObject(hbm);
	::EndPaint(m_hWnd, &ps);
	}//DrawBuffer

////////////////////////////////////////////////////////////////////////////
//		Helper Functions
////////////////////////////////////////////////////////////////////////////
//	vertically flip a buffer 
//	note, this operates on a buffer of widthBytes bytes, not pixels!!!
BOOL COmfVplayView::VertFlipBuf()
	{
	BYTE	*tb1;
	BYTE	*tb2;

	if (m_buf==NULL)
		{
		return FALSE;
		}//if

	UINT	bufsize;

	bufsize =	m_Width * COLORS_PER_PIXELS;

	tb1 =	(BYTE *)new BYTE[bufsize];
	if (tb1==NULL)
		{
		return FALSE;
		}//if

	tb2 =	(BYTE *)new BYTE [bufsize];
	if (tb2==NULL)
		{
		delete [] tb1;
		return FALSE;
		}//if
	
	long	row_cnt;
	ULONG	off1 =	0;
	ULONG	off2 =	0;

	for (row_cnt = 0;row_cnt<(m_Height+1)/2;row_cnt++)
		{
		off1 =	row_cnt*bufsize;
		off2 =	((m_Height-1)-row_cnt)*bufsize;
		
		memcpy(tb1,m_buf+off1,bufsize);
		memcpy(tb2,m_buf+off2,bufsize);
		memcpy(m_buf+off1,tb2,bufsize);
		memcpy(m_buf+off2,tb1,bufsize);
		}//for

	delete [] tb1;
	delete [] tb2;

	return TRUE;
	}//VertFlipBuf





/////////////////////////////////////////////////////////////////////////////
//WINDOWS SPECIFIC STUFF:
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// COmfVplayView
IMPLEMENT_DYNCREATE(COmfVplayView, CView)

BEGIN_MESSAGE_MAP(COmfVplayView, CView)
	//{{AFX_MSG_MAP(COmfVplayView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL COmfVplayView::PreCreateWindow(CREATESTRUCT& cs)
	{
	return CView::PreCreateWindow(cs);
	}//PreCreateWindow

/////////////////////////////////////////////////////////////////////////////
// COmfVplayView drawing
void COmfVplayView::OnDraw(CDC* pDC)
	{
	COmfVplayDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	}//OnDraw

/////////////////////////////////////////////////////////////////////////////
// COmfVplayView diagnostics
#ifdef _DEBUG
void COmfVplayView::AssertValid() const
	{
	CView::AssertValid();
	}//AssertValid

void COmfVplayView::Dump(CDumpContext& dc) const
	{
	CView::Dump(dc);
	}//Dump

COmfVplayDoc* COmfVplayView::GetDocument() // non-debug version is inline
	{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(COmfVplayDoc)));
	return (COmfVplayDoc*)m_pDocument;
	}//GetDocument
#endif //_DEBUG


