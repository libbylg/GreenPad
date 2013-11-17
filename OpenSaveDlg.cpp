#include "stdafx.h"
#include "rsrc/resource.h"
#include "kilib/kilib.h"
#include "OpenSaveDlg.h"
using namespace ki;



//------------------------------------------------------------------------
// �����R�[�h���X�g
//------------------------------------------------------------------------

CharSetList::CharSetList()
	: list_( 30 )
{
	static const TCHAR* const lnmJp[] = {
		TEXT("��������"),
		TEXT("���{��(ShiftJIS)"),
		TEXT("���{��(EUC)"),
		TEXT("���{��(ISO-2022-JP)"),
		TEXT("UTF-5"),
		TEXT("UTF-7"),
		TEXT("UTF-8"),
		TEXT("UTF-8N"),
		TEXT("UTF-16BE(BOM)"),
		TEXT("UTF-16LE(BOM)"),
		TEXT("UTF-16BE"),
		TEXT("UTF-16LE"),
		TEXT("UTF-32BE(BOM)"),
		TEXT("UTF-32LE(BOM)"),
		TEXT("UTF-32BE"),
		TEXT("UTF-32LE"),
		TEXT("����"),
		TEXT("����"),
		TEXT("�؍���(EUC-KR)"),
		TEXT("�؍���(ISO-2022-KR)"),
		TEXT("�؍���(Johab)"),
		TEXT("������(GB2312)"),
		TEXT("������(ISO-2022-CN)"),
		TEXT("������(HZ)"),
		TEXT("������(Big5)"),
		TEXT("�L������(Windows)"),
		TEXT("�L������(KOI8-R)"),
		TEXT("�L������(KOI8-U)"),
		TEXT("�^�C��"),
		TEXT("�g���R��"),
		TEXT("�o���g��"),
		TEXT("�x�g�i����"),
		TEXT("�M���V����"),
		TEXT("MSDOS(us)")
	};
	static const TCHAR* const lnmEn[] = {
		TEXT("AutoDetect"),
		TEXT("Japanese(ShiftJIS)"),
		TEXT("Japanese(EUC)"),
		TEXT("Japanese(ISO-2022-JP)"),
		TEXT("UTF-5"),
		TEXT("UTF-7"),
		TEXT("UTF-8"),
		TEXT("UTF-8N"),
		TEXT("UTF-16BE(BOM)"),
		TEXT("UTF-16LE(BOM)"),
		TEXT("UTF-16BE"),
		TEXT("UTF-16LE"),
		TEXT("UTF-32BE(BOM)"),
		TEXT("UTF-32LE(BOM)"),
		TEXT("UTF-32BE"),
		TEXT("UTF-32LE"),
		TEXT("Latin-1"),
		TEXT("Latin-2"),
		TEXT("Korean(EUC-KR)"),
		TEXT("Korean(ISO-2022-KR)"),
		TEXT("Korean(Johab)"),
		TEXT("Chinese(GB2312)"),
		TEXT("Chinese(ISO-2022-CN)"),
		TEXT("Chinese(HZ)"),
		TEXT("Chinese(Big5)"),
		TEXT("Cyrillic(Windows)"),
		TEXT("Cyrillic(KOI8-R)"),
		TEXT("Cyrillic(KOI8-U)"),
		TEXT("Thai"),
		TEXT("Turkish"),
		TEXT("Baltic"),
		TEXT("Vietnamese"),
		TEXT("Greek"),
		TEXT("MSDOS(us)")
	};
	static const TCHAR* const snm[] = {
		TEXT(""),
		TEXT("SJIS"),
		TEXT("EUC"),
		TEXT("JIS"),
		TEXT("UTF5"),
		TEXT("UTF7"),
		TEXT("UTF8"),
		TEXT("UTF8"),
		TEXT("U16B"),
		TEXT("U16L"),
		TEXT("U16B"),
		TEXT("U16L"),
		TEXT("U32B"),
		TEXT("U32L"),
		TEXT("U32B"),
		TEXT("U32L"),
		TEXT("LTN1"),
		TEXT("LTN2"),
		TEXT("UHC"),
		TEXT("I2KR"),
		TEXT("Jhb"),
		TEXT("GBK"),
		TEXT("I2CN"),
		TEXT("HZ"),
		TEXT("BIG5"),
		TEXT("CYRL"),
		TEXT("KO8R"),
		TEXT("KO8U"),
		TEXT("THAI"),
		TEXT("TRK"),
		TEXT("BALT"),
		TEXT("VTNM"),
		TEXT("GRK"),
		TEXT("DOS")
	};

	// ���{����Ȃ���{��\����I��
	const TCHAR* const * lnm = (::GetACP()==932 ? lnmJp : lnmEn);

	// �������������̖ʓ|�Ȃ̂ŒZ�k�\�L(^^;
	CsInfo cs;
	#define Enroll(_id,_nm)   cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=LOAD|SAVE,    list_.Add( cs )
	#define EnrollS(_id,_nm)  cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=SAVE,         list_.Add( cs )
	#define EnrollL(_id,_nm)  cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=LOAD,         list_.Add( cs )

	// �K�X�o�^
	                               EnrollL( AutoDetect,0 );
	if( ::IsValidCodePage(932) )   Enroll(  SJIS,      1 ),
	                               Enroll(  EucJP,     2 ),
	                               Enroll(  IsoJP,     3 );
	/* if( always ) */             Enroll(  UTF5,      4 );
	                               Enroll(  UTF7,      5 );
	                               Enroll(  UTF8,      6 );
	                               EnrollS( UTF8N,     7 );
	                               EnrollS( UTF16b,    8 );
	                               EnrollS( UTF16l,    9 );
	                               Enroll(  UTF16BE,  10 );
	                               Enroll(  UTF16LE,  11 );
	                               EnrollS( UTF32b,   12 );
	                               EnrollS( UTF32l,   13 );
	                               Enroll(  UTF32BE,  14 );
	                               Enroll(  UTF32LE,  15 );
	                               Enroll(  Western,  16 );
	if( ::IsValidCodePage(28592) ) Enroll(  Central,  17 );
	if( ::IsValidCodePage(949) )   Enroll(  UHC,      18 ),
	                               Enroll(  IsoKR,    19 );
	if( ::IsValidCodePage(1361) )  Enroll(  Johab,    20 );
	if( ::IsValidCodePage(936) )   Enroll(  GBK,      21 ),
	                               Enroll(  IsoCN,    22 ),
	                               Enroll(  HZ   ,    23 );
	if( ::IsValidCodePage(950) )   Enroll(  Big5 ,    24 );
	if( ::IsValidCodePage(28595) ) Enroll(  Cyrillic, 25 );
	if( ::IsValidCodePage(20866) ) Enroll(  Koi8R,    26 );
	if( ::IsValidCodePage(21866) ) Enroll(  Koi8U,    27 );
	if( ::IsValidCodePage(874) )   Enroll(  Thai,     28 );
	if( ::IsValidCodePage(1254) )  Enroll(  Turkish,  29 );
	if( ::IsValidCodePage(1257) )  Enroll(  Baltic,   30 );
	if( ::IsValidCodePage(1258) )  Enroll( Vietnamese,31 );
	if( ::IsValidCodePage(28597) ) Enroll(  Greek,    32 );
	                               Enroll(  DOSUS,    33 );

	// �I��
	#undef Enroll
	#undef EnrollS
	#undef EnrollL
}

int CharSetList::defaultCs() const
{
	return ::GetACP();
/*
	switch( ::GetACP() )
	{
	case 932: return SJIS;
	case 936: return GBK;
	case 949: return UHC;
	case 950: return Big5;
	default:  return Western;
	}
*/
}

ulong CharSetList::defaultCsi() const
{
	return findCsi( defaultCs() );
}

ulong CharSetList::findCsi( int cs ) const
{
	for( ulong i=0,ie=list_.size(); i<ie; ++i )
		if( list_[i].ID == cs )
			return i;
	return 0xffffffff;
}



//------------------------------------------------------------------------
// �u�J���v�_�C�A���O
//------------------------------------------------------------------------

namespace
{
	// �֐��I�����ɁA�J�����g�f�B���N�g�������ɖ߂�
	class CurrentDirRecovery
	{
		Path cur_;
	public:
		CurrentDirRecovery() : cur_(Path::Cur) {}
		~CurrentDirRecovery() { ::SetCurrentDirectory(cur_.c_str()); }
	};
}

OpenFileDlg* OpenFileDlg::pThis;

bool OpenFileDlg::DoModal( HWND wnd, const TCHAR* fltr, const TCHAR* fnm )
{
	CurrentDirRecovery cdr;

	if( fnm == NULL )
		filename_[0] = TEXT('\0');
	else
		::lstrcpy( filename_, fnm );

	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner      = wnd;
	ofn.hInstance      = app().hinst();
	ofn.lpstrFilter    = fltr;
	ofn.lpstrFile      = filename_;
	ofn.nMaxFile       = countof(filename_);
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENFILEHOOK);
	ofn.lpfnHook       = OfnHook;
	ofn.Flags = OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY  |
				OFN_EXPLORER      |
				OFN_ENABLESIZING  |
				OFN_ENABLEHOOK    |
				OFN_ENABLETEMPLATE;

	pThis = this;
	return ( ::GetOpenFileName(&ofn) != 0 );
}

UINT_PTR CALLBACK OpenFileDlg::OfnHook( HWND dlg, UINT msg, WPARAM, LPARAM lp )
{
	if( msg==WM_INITDIALOG )
	{
		// �R���{�{�b�N�X�𖄂߂āA�u�����I���v��I��
		ComboBox cb( dlg, IDC_CODELIST );
		const CharSetList& csl = pThis->csl_;
		for( ulong i=0; i<csl.size(); ++i )
			if( csl[i].type & 2 ) // 2:=LOAD
				cb.Add( csl[i].longName );
		cb.Select( csl[0].longName );
	}
	else if( msg==WM_NOTIFY )
	{
		// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
		if( reinterpret_cast<NMHDR*>(lp)->code==CDN_FILEOK )
		{
			ulong j=0, i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			for(;;++j,--i)
			{
				while( !(pThis->csl_[j].type & 2) ) // !LOAD
					++j;
				if( i==0 )
					break;
			}
			pThis->csIndex_ = j;
		}
	}
	return FALSE;
}



//------------------------------------------------------------------------
// �u�ۑ��v�_�C�A���O
//------------------------------------------------------------------------

SaveFileDlg* SaveFileDlg::pThis;

bool SaveFileDlg::DoModal( HWND wnd, const TCHAR* fltr, const TCHAR* fnm )
{
	CurrentDirRecovery cdr;

	if( fnm == NULL )
		filename_[0] = TEXT('\0');
	else
		::lstrcpy( filename_, fnm );

	OPENFILENAME ofn = {sizeof(ofn)};
    ofn.hwndOwner      = wnd;
    ofn.hInstance      = app().hinst();
    ofn.lpstrFilter    = fltr;
    ofn.lpstrFile      = filename_;
    ofn.nMaxFile       = countof(filename_);
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEFILEHOOK);
    ofn.lpfnHook       = OfnHook;
    ofn.Flags = OFN_HIDEREADONLY    |
				OFN_PATHMUSTEXIST   |
				OFN_EXPLORER        |
				OFN_ENABLESIZING    |
				OFN_ENABLEHOOK      |
				OFN_ENABLETEMPLATE  |
				OFN_OVERWRITEPROMPT;

	pThis        = this;
	return ( ::GetSaveFileName(&ofn) != 0 );
}

UINT_PTR CALLBACK SaveFileDlg::OfnHook( HWND dlg, UINT msg, WPARAM, LPARAM lp )
{
	if( msg==WM_INITDIALOG )
	{
		// �R���{�{�b�N�X�𖄂߂āA�K�؂Ȃ̂�I��
		{
			ComboBox cb( dlg, IDC_CODELIST );
			const CharSetList& csl = pThis->csl_;

			for( ulong i=0; i<csl.size(); ++i )
				if( csl[i].type & 1 ) // 1:=SAVE
					cb.Add( csl[i].longName );
			cb.Select( csl[pThis->csIndex_].longName );
		}
		{
			ComboBox cb( dlg, IDC_CRLFLIST );
			static const TCHAR* const lbList[] = {
				TEXT("CR"),
				TEXT("LF"),
				TEXT("CRLF")
			};

			for( ulong i=0; i<countof(lbList); ++i )
				cb.Add( lbList[i] );
			cb.Select( lbList[pThis->lb_] );
		}
	}
	else if( msg==WM_NOTIFY )
	{
		if( reinterpret_cast<NMHDR*>(lp)->code==CDN_FILEOK )
		{
			// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
			ulong j=0, i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			for(;;++j,--i)
			{
				while( !(pThis->csl_[j].type & 1) ) // !SAVE
					++j;
				if( i==0 )
					break;
			}
			pThis->csIndex_ = j;
			// ���s�R�[�h��
			pThis->lb_ = ComboBox(dlg,IDC_CRLFLIST).GetCurSel();
		}
	}
	return FALSE;
}



//------------------------------------------------------------------------
// ���[�e�B���e�B�[
//------------------------------------------------------------------------

ki::aarr<TCHAR> OpenFileDlg::ConnectWithNull( String lst[], int num )
{
	int TtlLen = 1;
	for( int i=0; i<num; ++i )
		TtlLen += (lst[i].len() + 1);

	aarr<TCHAR> a( new TCHAR[TtlLen] );

	TCHAR* p = a.get();
	for( int i=0; i<num; ++i )
	{
		::lstrcpy( p, lst[i].c_str() );
		p += (lst[i].len() + 1);
	}
	*p = TEXT('\0');

	return a;
}




//------------------------------------------------------------------------
// �u�J�������v�_�C�A���O
//------------------------------------------------------------------------

ReopenDlg::ReopenDlg( const CharSetList& csl, int csi )
	: DlgImpl(IDD_REOPENDLG), csl_(csl), csIndex_(csi)
{
}

void ReopenDlg::on_init()
{
	// �R���{�{�b�N�X�𖄂߂āA�u�����I���v��I��
	ComboBox cb( hwnd(), IDC_CODELIST );
	for( ulong i=0; i<csl_.size(); ++i )
		if( csl_[i].type & 1 ) // 2:=SAVE
			cb.Add( csl_[i].longName );
	cb.Select( csl_[csIndex_].longName );
}

bool ReopenDlg::on_ok()
{
	// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
	ulong j=0, i=ComboBox(hwnd(),IDC_CODELIST).GetCurSel();
	for(;;++j,--i)
	{
		while( !(csl_[j].type & 1) ) // !SAVE
			++j;
		if( i==0 )
			break;
	}
	csIndex_ = j;
	return true;
}
