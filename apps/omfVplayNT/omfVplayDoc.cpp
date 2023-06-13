// omfVplayDoc.cpp : implementation of the COmfVplayDoc class
//

#include "stdafx.h"
#include "omfVplay.h"

#include "omfVplayDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COmfVplayDoc

IMPLEMENT_DYNCREATE(COmfVplayDoc, CDocument)

BEGIN_MESSAGE_MAP(COmfVplayDoc, CDocument)
	//{{AFX_MSG_MAP(COmfVplayDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COmfVplayDoc construction/destruction

COmfVplayDoc::COmfVplayDoc()
{
	// TODO: add one-time construction code here

}

COmfVplayDoc::~COmfVplayDoc()
{
}

BOOL COmfVplayDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// COmfVplayDoc serialization

void COmfVplayDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// COmfVplayDoc diagnostics

#ifdef _DEBUG
void COmfVplayDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void COmfVplayDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COmfVplayDoc commands
