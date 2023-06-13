// omfVplayView.h : interface of the COmfVplayView class
//
/////////////////////////////////////////////////////////////////////////////

#include "omTypes.h"
#include "omErr.h"



class COmfVplayView : public CView
{
protected: // create from serialization only
	COmfVplayView();
	DECLARE_DYNCREATE(COmfVplayView)

// Attributes
public:
	COmfVplayDoc* GetDocument();

// Operations
public:
	// global image params
	BYTE				*m_buf;
	int 				m_Width;
	int 				m_Height;
	int 				m_DisplayHeight;


	omfSessionHdl_t		sessionPtr;
	omfHdl_t			filePtr;
	omfIterHdl_t		mobIter;
	omfIterHdl_t		trackIter;

	omfFrameLayout_t	frameLayout;
	omfFrameLayout_t	outputFrameLayout;
	omfBool				stripBlank;

	omfErr_t			status;
	omfSearchCrit_t		search;

	



	//OMF Player:
	void			PlayOMF(omfObject_t	masterMob);
	omfErr_t		PlayTrack(omfMediaHdl_t mediaPtr);
	void			DrawBuffer();
	BOOL			VertFlipBuf();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COmfVplayView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COmfVplayView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(COmfVplayView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	afx_msg void OnFileOpen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in omfVplayView.cpp
inline COmfVplayDoc* COmfVplayView::GetDocument()
   { return (COmfVplayDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
