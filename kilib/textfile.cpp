#include "stdafx.h"
#include "app.h"
#include "textfile.h"
#include "ktlarray.h"
using namespace ki;



//=========================================================================
// �e�L�X�g�t�@�C���ǂݏo�����ʃC���^�[�t�F�C�X
//=========================================================================

struct ki::TextFileRPimpl : public Object
{
	inline TextFileRPimpl()
		: state(EOL) {}

	virtual size_t ReadLine( unicode* buf, ulong siz )
		= 0;

	enum { EOF=0, EOL=1, EOB=2 } state;
};



//-------------------------------------------------------------------------
// Unicode�n�p�̃x�[�X�N���X
//	UTF-8�ȊO�͂���Ȃɏo���Ȃ����낤����x���Ă��悵�Ƃ���B
//-------------------------------------------------------------------------

struct rBasicUTF : public ki::TextFileRPimpl
{
	virtual unicode PeekC() = 0;
	virtual unicode GetC() {unicode ch=PeekC(); Skip(); return ch;}
	virtual void Skip() = 0;
	virtual bool Eof() = 0;

	size_t ReadLine( unicode* buf, ulong siz )
	{
		state = EOF;

		// ���s���o��܂œǂ�
		unicode *w=buf, *e=buf+siz;
		while( !Eof() )
		{
			*w = GetC();
			if( *w==L'\r' || *w==L'\n' )
			{
				state = EOL;
				break;
			}
			else if( *w!=0xfeff && ++w==e )
			{
				state = EOB;
				break;
			}
		}

		// ���s�R�[�h�X�L�b�v����
		if( state == EOL )
			if( *w==L'\r' && !Eof() && PeekC()==L'\n' )
				Skip();

		// �ǂ񂾕�����
		return w-buf;
	}
};



//-------------------------------------------------------------------------
// UCS2�x�^/UCS4�x�^�B���ꂼ��UTF16, UTF32�̑���Ƃ��Ďg���B
// ���łɓ���template�ŁAISO-8859-1���������Ă��܂��B^^;
//-------------------------------------------------------------------------

template<typename T>
struct rUCS : public rBasicUTF
{
	rUCS( const uchar* b, ulong s, bool bigendian )
		: fb( reinterpret_cast<const T*>(b) )
		, fe( reinterpret_cast<const T*>(b+(s/sizeof(T))*sizeof(T)) )
		, be( bigendian ) {}

	const T *fb, *fe;
	const bool be;

	// �G���f�B�A���ϊ�
	inline  byte swap(  byte val ) { return val; }
	inline dbyte swap( dbyte val ) { return (val<<8) |(val>>8); }
	inline qbyte swap( qbyte val ) { return ((val>>24)&0xff |
	                                         (val>>8)&0xff00 |
											 (val<<8)&0xff0000|
											 (val<<24)); }

	virtual void Skip() { ++fb; }
	virtual bool Eof() { return fb==fe; }
	virtual unicode PeekC() { return (unicode)(be ? swap(*fb) : *fb); }
};

typedef rUCS< byte> rWest;
typedef rUCS<dbyte> rUtf16;

// UTF-32�ǂݍ���
struct rUtf32 : public rUCS<qbyte>
{
	rUtf32( const uchar* b, ulong s, bool bigendian )
		: rUCS<qbyte>(b,s,bigendian)
		, state(0) {}

	int state;
	qbyte curChar() { return be ? swap(*fb) : *fb; }
	bool inBMP(qbyte c) { return c<0x10000; }

	virtual unicode PeekC()
	{
		qbyte c = curChar();
		if( inBMP(c) )
			return (unicode)c;
		return (unicode)(state==0 ? 0xD800 + (((c-0x10000) >> 10)&0x3ff)
			                      : 0xDC00 + ( (c-0x10000)       &0x3ff));
	}

	virtual void Skip()
	{
		if( inBMP(curChar()) )
			++fb;
		else if( state==0 )
			state=1;
		else
			++fb, state=0;
	}
};



//-------------------------------------------------------------------------
// UTF-5
//     0-  F : 1bbbb
//    10- FF : 1bbbb 0bbbb
//   100-FFF : 1bbbb 0bbbb 0bbbb
// �Ƃ����悤�ɁA16�i�ł̈ꌅ���ꕶ���ŕ\���Ă����t�H�[�}�b�g�B
// �e 0bbbb �� '0', '1', ... '9', 'A', ... 'F'
// �e 1bbbb �� 'G', 'H', ... 'P', 'Q', ... 'V' �̎��ŕ\���B
//-------------------------------------------------------------------------

struct rUtf5 : public rBasicUTF
{
	rUtf5( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s ) {}

	const uchar *fb, *fe;

	// 16�i�������琮���l�֕ϊ�
	inline byte conv( uchar x )
	{
		if( '0'<=x && x<='9' ) return x-'0';
		else                   return x-'A'+0x0A;
	}

	void Skip() { do ++fb; while( fb<fe && *fb<'G' ); }
	bool Eof() { return fb==fe; }
	unicode PeekC()
	{
		unicode ch = (*fb-'G');
		for( const uchar* p=fb+1; p<fe && *p<'G'; ++p )
			ch = (ch<<4)|conv(*p);
		return ch;
	}
};



//-------------------------------------------------------------------------
// UTF-7
//   ASCII�͈͂̎��͂��̂܂܁B����ȊO��UTF-16�̒l��base64�G���R�[�h
//   ���ďo�́B�G���R�[�h���ꂽ������ + �� - �ŋ��܂��B�܂� '+' ��
//   ���������̂�\�����邽�߂� "+-" �Ƃ����`����p����B
//-------------------------------------------------------------------------

namespace
{
	static const uchar u7c[128]={
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3e,0xff,0xff,0xff,0x3f,
	0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
	0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0xff,0xff,0xff,0xff,0xff,
	0xff,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
	0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0xff,0xff,0xff,0xff };
}

struct rUtf7 : public rBasicUTF
{
	rUtf7( const uchar* b, ulong s )
		: fb( b )
		, fe( b+s )
		, rest( -1 ) { fillbuf(); }

	const uchar *fb, *fe;
	unicode buf[3]; // b64���W�������ɓǂ�Ńo�b�t�@�ɗ��߂Ă���
	int rest;       // �o�b�t�@�̋�
	bool inB64;     // base64�G���A���Ȃ�true

	void Skip() { if(--rest==0) fillbuf(); }
	bool Eof() { return fb==fe && rest==0; }
	unicode PeekC() { return buf[rest-1]; }

	void fillbuf()
	{
		if( fb<fe )
			if( !inB64 )
				if( *fb=='+' )
					if( fb+1<fe && fb[1]=='-' )
						rest=1, buf[0]=L'+', fb+=2;  // +-
					else
						++fb, inB64=true, fillbuf(); // �P�� +
				else
					rest=1, buf[0]=*fb++;            // ���ʂ̎�
			else
			{
				// �������f�R�[�h�ł��邩������
				int N=0, E=Max(int(fb-fe),8);
				while( N<E && fb[N]<0x80 && u7c[fb[N]]!=0xff )
					++N;

				// �f�R�[�h
				buf[0]=buf[1]=buf[2]=0;
				switch( N )
				{
				case 8: buf[2]|= u7c[fb[7]];
				case 7: buf[2]|=(u7c[fb[6]]<< 6);
				case 6: buf[2]|=(u7c[fb[5]]<<12), buf[1]|=(u7c[fb[5]]>>4);
				case 5: buf[1]|=(u7c[fb[4]]<< 2);
				case 4: buf[1]|=(u7c[fb[3]]<< 8);
				case 3: buf[1]|=(u7c[fb[2]]<<14), buf[0]|=(u7c[fb[2]]>>2);
				case 2: buf[0]|=(u7c[fb[1]]<< 4);
				case 1: buf[0]|=(u7c[fb[0]]<<10);
					unicode t;
					rest = 1;
					if( N==8 )
						rest=3, t=buf[0], buf[0]=buf[2], buf[2]=t;
					else if( N>=6 )
						rest=2, t=buf[0], buf[0]=buf[1], buf[1]=t;
				}

				// �g�������i��
				if( N<E )
				{
					inB64=false;
					if( fb[N]=='-' )
						++fb;
				}
				fb += N;
				if( N==0 )
					fillbuf();
			}
	}
};



//-------------------------------------------------------------------------
// UTF8/MBCS
//  CR,LF���P�o�C�g�����Ƃ��Ă�����Əo�Ă���̂ŁA
//  �؂蕪�����ȒP�Ȍ`���������ł܂Ƃ߂Ĉ����BUTF8�ȊO�̕ϊ���
//  Windows�ɑS�ĔC���Ă���B
//-------------------------------------------------------------------------

namespace 
{
	typedef char* (WINAPI * uNextFunc)(WORD,const char*,DWORD);

	static const byte mask[] = { 0, 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
	static inline int GetMaskIndex(uchar n)
	{
		if( uchar(n+2) < 0xc2 ) return 1; // 00�`10111111, fe, ff
		if( n          < 0xe0 ) return 2; // 110xxxxx
		if( n          < 0xf0 ) return 3; // 1110xxxx
		if( n          < 0xf8 ) return 4; // 11110xxx
		if( n          < 0xfc ) return 5; // 111110xx
		                        return 6; // 1111110x
	}

	static char* WINAPI CharNextUtf8( WORD, const char* p, DWORD )
	{
		return const_cast<char*>( p+GetMaskIndex(uchar(*p)) );
	}

	// Win95�΍�B
	//   http://support.microsoft.com/default.aspx?scid=%2Fisapi%2Fgomscom%2Easp%3Ftarget%3D%2Fjapan%2Fsupport%2Fkb%2Farticles%2Fjp175%2F3%2F92%2Easp&LN=JA
	// MSDN�ɂ�Win95�ȍ~�ŃT�|�[�g�Ə����Ă���̂�CP_UTF8��
	// �g���Ȃ��炵���̂ŁA���O�̕ϊ��֐��ŁB
	typedef int (WINAPI * uConvFunc)(UINT,DWORD,const char*,int,wchar_t*,int);
	static int WINAPI Utf8ToWideChar( UINT, DWORD, const char* sb, int ss, wchar_t* wb, int ws )
	{
		const uchar *p = reinterpret_cast<const uchar*>(sb);
		const uchar *e = reinterpret_cast<const uchar*>(sb+ss);
		wchar_t     *w = wb; // �o�b�t�@�T�C�Y�`�F�b�N�����i�d�l�j

		for( int t; p<e; ++w )
		{
			t  = GetMaskIndex(*p);
			qbyte qch = (*p++ & mask[t]);
			while( p<e && --t )
				qch<<=6, qch|=(*p++)&0x3f;
			if(qch<0x10000)
				*w = (wchar_t)qch;
			else
				*w++ = (wchar_t)(0xD800 + (((qch-0x10000)>>10)&0x3ff)),
				*w   = (wchar_t)(0xDC00 + (((qch-0x10000)    )&0x3ff));
		}
		return int(w-wb);
	}
}

struct rMBCS : public TextFileRPimpl
{
	// �t�@�C���|�C���^���R�[�h�y�[�W
	const char* fb;
	const char* fe;
	const int   cp;
	uNextFunc next;
	uConvFunc conv;

	// �����ݒ�
	rMBCS( const uchar* b, ulong s, int c )
		: fb( reinterpret_cast<const char*>(b) )
		, fe( reinterpret_cast<const char*>(b+s) )
		, cp( c==UTF8 ? UTF8N : c )
		, next( cp==UTF8N ?   CharNextUtf8 : CharNextExA )
		, conv( cp==UTF8N && app().isWin95()
		                  ? Utf8ToWideChar : MultiByteToWideChar )
	{
		if( cp==UTF8N && fe-fb>=3
		 && b[0]==0xef && b[1]==0xbb && b[2]==0xbf )
			fb += 3; // BOM�X�L�b�v
	}

	size_t ReadLine( unicode* buf, ulong siz )
	{
		// �o�b�t�@�̏I�[���A�t�@�C���̏I�[�̋߂����܂œǂݍ���
		const char *p, *end = Min( fb+siz/2, fe );
		state = (end==fe ? EOF : EOB);

		// ���s���o��܂Ői��
		for( p=fb; p<end; )
			if( *p=='\r' || *p=='\n' )
			{
				state = EOL;
				break;
			}
			else if( (*p) & 0x80 )
			{
				p = next(cp,p,0);
			}
			else
			{
				++p;
			}

		// Unicode�֕ϊ�
#ifndef _UNICODE
		ulong len = conv( cp, 0, fb, p-fb, buf, siz );
#else
		ulong len = ::MultiByteToWideChar( cp, 0, fb, int(p-fb), buf, siz );
#endif
		// ���s�R�[�h�X�L�b�v����
		if( state == EOL )
			if( *(p++)=='\r' && p<fe && *p=='\n' )
				++p;
		fb = p;

		// �I��
		return len;
	}
};



//-------------------------------------------------------------------------
// ISO-2022 �̓K���Ȏ���
//
//	�R�[�h�y�[�W�Ƃ��āAG0, G1, G2, G3 �̎l�ʂ����B
//	���ꂼ�ꌈ�܂����G�X�P�[�v�V�[�P���X�ɂ���āA
//	���܂��܂ȕ����W�����e�ʂɌĂяo�����Ƃ��o����B
//	�Ƃ肠�������݂̃o�[�W�����őΉ����Ă���ӂ�Ȃ̂�
//	���̒ʂ�B()���ɁA���̂����������Ղ�������B
//
//		<<���ł�>>
//		1B 28 42    : G0�� ASCII
//		1B 28 4A    : G0�� JIS X 0201 ���[�}�� (�̂�����ASCII)
//		1B 29 4A    : G1�� JIS X 0201 ���[�}�� (�̂�����ASCII)
//		1B 2A 4A    : G2�� JIS X 0201 ���[�}�� (�̂�����ASCII)
//		1B 3B 4A    : G3�� JIS X 0201 ���[�}�� (�̂�����ASCII)
//		1B 2E 41    : G2�� ISO-8859-1
//		<<CP932���L���ȏꍇ>>
//		1B 28 49    : G0�� JIS X 0201 �J�i
//		1B 29 49    : G1�� JIS X 0201 �J�i
//		1B 2A 49    : G2�� JIS X 0201 �J�i
//		1B 2B 49    : G3�� JIS X 0201 �J�i
//		1B 24 40    : G0�� JIS X 0208(1978)
//		1B 24 42    : G0�� JIS X 0208(1983)    (�N�x�͋�ʂ��Ȃ�)
//		<<CP936���L���ȏꍇ>>
//		1B 24 41    : G0�� GB 2312
//		1B 24 29 41 : G1�� GB 2312
//		1B 24 2A 41 : G2�� GB 2312
//		1B 24 2B 41 : G3�� GB 2312
//		<<CP949���L���ȏꍇ>>
//		1B 24 28 43 : G0�� KS X 1001
//		1B 24 29 43 : G1�� KS X 1001
//		1B 24 2A 43 : G2�� KS X 1001
//		1B 24 2B 43 : G3�� KS X 1001
//
//	�e�ʂɌĂяo���������W���́A
//		GL (0x21�`0xfe) GR (0xa0�`0xff)
//	�̂ǂ��炩�փ}�b�v���邱�ƂŁA���ۂ̃o�C�g�l�ƂȂ�B
//	�}�b�v���߂ƂȂ�o�C�g��́A���̒ʂ�
//
//		0F    : GL        ��G0���Ăяo��
//		0E    : GL        ��G1���Ăяo��
//		1B 7E : GR        ��G1���Ăяo��
//		8E    : GL/GR���� ��G2����u�����Ăяo���B1B 4E �����`
//		8F    : GL/GR���� ��G3����u�����Ăяo���B1B 4F �����`
//
//-------------------------------------------------------------------------

enum CodeSet { ASCII, LATIN, KANA, JIS, KSX, GB };

struct rIso2022 : public TextFileRPimpl
{
	// Helper: JIS X 0208 => SJIS
	void jis2sjis( uchar k, uchar t, char* s )
	{
		if(k>=0x3f)	s[0] = (char)(((k+1)>>1)+0xc0);
		else		s[0] = (char)(((k+1)>>1)+0x80);
		if( k&1 )	s[1] = (char)((t>>6) ? t+0x40 : t+0x3f);
		else		s[1] = (char)(t+0x9e);
	}

	// �t�@�C���|�C���^
	const uchar* fb;
	const uchar* fe;
	bool      fixed; // ESC�ɂ��؂�ւ����s��Ȃ��Ȃ�true
	bool    mode_hz; // HZ�̏ꍇ�B

	// ��ƕϐ�
	CodeSet *GL, *GR, G[4];
	int gWhat; // ���̎��� 1:GL/GR 2:G2 3:G3 �ŏo��
	ulong len;

	// ������
	rIso2022( const uchar* b, ulong s, bool f, bool hz,
		CodeSet g0, CodeSet g1, CodeSet g2=ASCII, CodeSet g3=ASCII )
		: fb( b )
		, fe( b+s )
		, fixed( f )
		, mode_hz( hz )
		, GL( &G[0] )
		, GR( &G[1] )
		, gWhat( 1 )
	{
		G[0]=g0, G[1]=g1, G[2]=g2, G[3]=g3;
	}

	void DoSwitching( const uchar*& p )
	{
		if( fixed )
		{
			if( p[0]==0x24 && p[1]!=0x40 && p[1]!=0x41 && p[1]!=0x42
			 && p+2 < fe && (p[2]==0x41 || p[2]==0x43) )
				++p;
		}
		else
		{
			if( p[1]==0x4A )
				G[ (p[0]-0x28)%4 ] = ASCII;         // 1B [28-2B] 4A
			else if( p[1]==0x49 )
				G[ (p[0]-0x28)%4 ] = KANA;          // 1B [28-2B] 49
			else if( *reinterpret_cast<const dbyte*>(p)==0x4228 )
				G[ 0 ] = ASCII;                     // 1B 28 42
			else if( *reinterpret_cast<const dbyte*>(p)==0x412E )
				G[ 2 ] = LATIN;                     // 1B 2E 41
			else if( p[0]==0x24 )
				if( p[1]==0x40 || p[1]==0x42 )
					G[ 0 ] = JIS;                   // 1B 24 [40|42]
				else if( p[1]==0x41 )
					G[ 0 ] = GB;                    // 1B 24 41
				else if( p+2 < fe )
					if( p[2]==0x41 )
						G[ ((*++p)-0x28)%4 ] = GB;  // 1B 24 [28-2B] 41
					else if( p[2]==0x43 )
						G[ ((*++p)-0x28)%4 ] = KSX; // 1B 24 [28-2B] 43
		}
		++p;
	}

	void DoOutput( unicode*& buf, const uchar*& p )
	{
		// �����W�����o��
		CodeSet cs =
			(gWhat==2 ? G[2] : 
			(gWhat==3 ? G[3] :
			(*p&0x80  ? *GR  : *GL)));

		char c[2];
		ulong wt=1;
		switch( cs )
		{
		case ASCII:
			*buf = (*p)&0x7f;
			break;
		case LATIN:
			*buf = (*p)|0x80;
			break;
		case KANA:
			c[0] = (*p)|0x80;
			wt = ::MultiByteToWideChar(
				932, MB_PRECOMPOSED, c, 1, buf, 2 );
			break;
		case GB:
		case KSX:
			c[0] = (*  p)|0x80;
			c[1] = (*++p)|0x80;
			wt = ::MultiByteToWideChar(
				(cs==GB?936:949), MB_PRECOMPOSED, c, 2, buf, 2 );
			break;
		case JIS:
			jis2sjis( (p[0]&0x7f)-0x20, (p[1]&0x7f)-0x20, c );
			++p;
			wt = ::MultiByteToWideChar(
				932, MB_PRECOMPOSED, c, 2, buf, 2 );
			break;
		}
		buf+=wt;
		len+=wt;
	}

	size_t ReadLine( unicode* buf, ulong siz )
	{
		len=0;

		// �o�b�t�@�̏I�[���A�t�@�C���̏I�[�̋߂����܂œǂݍ���
		const uchar *p, *end = Min( fb+siz/2, fe );
		state = (end==fe ? EOF : EOB);

		// ���s���o��܂Ői��
		for( p=fb; p<end; ++p )
			switch( *p )
			{
			case '\r':
			case '\n': state =   EOL; goto outofloop;
			case 0x0F:    GL = &G[0]; break;
			case 0x0E:    GL = &G[1]; break;
			case 0x8E: gWhat =     2; break;
			case 0x8F: gWhat =     3; break;
			case 0x1B:
				if( p+1<fe ) {
					++p; if( *p==0x7E )    GR = &G[1];
					else if( *p==0x4E ) gWhat =  2;
					else if( *p==0x4F ) gWhat =  3;
					else if( p+1<fe )   DoSwitching(p);
				}break;
			case 0x7E: if( mode_hz && p+1<fe ) {
					++p; if( *p==0x7D ){ GL = &G[0]; break; }
					else if( *p==0x7B ){ GL = &G[1]; break; }
				} // fall through...
			default:   DoOutput( buf, p ); gWhat=1; break;
			}
		outofloop:

		// ���s�R�[�h�X�L�b�v����
		if( state == EOL )
			if( *(p++)=='\r' && p<fe && *p=='\n' )
				++p;
		fb = p;

		// �I��
		return len;
	}
};



//-------------------------------------------------------------------------
// ��������ȂǂȂ�
//-------------------------------------------------------------------------

TextFileR::TextFileR( int charset )
	: cs_( charset )
	, nolbFound_(true)
{
}

TextFileR::~TextFileR()
{
	Close();
}

size_t TextFileR::ReadLine( unicode* buf, ulong siz )
{
	return impl_->ReadLine( buf, siz );
}

int TextFileR::state() const
{
	return impl_->state;
}

void TextFileR::Close()
{
	fp_.Close();
}

bool TextFileR::Open( const TCHAR* fname )
{
	// �t�@�C�����J��
	if( !fp_.Open(fname) )
		return false;
	const uchar* buf = fp_.base();
	const ulong  siz = fp_.size();

	// �K�v�Ȃ玩������
	cs_ = AutoDetection( cs_, buf, Min<ulong>(siz,16<<10) ); // �擪16KB

	// �Ή�����f�R�[�_���쐬
	switch( cs_ )
	{
	case Western: impl_ = new rWest(buf,siz,true); break;
	case UTF16b:
	case UTF16BE: impl_ = new rUtf16(buf,siz,true); break;
	case UTF16l:
	case UTF16LE: impl_ = new rUtf16(buf,siz,false); break;
	case UTF32b:
	case UTF32BE: impl_ = new rUtf32(buf,siz,true); break;
	case UTF32l:
	case UTF32LE: impl_ = new rUtf32(buf,siz,false); break;
	case UTF5:    impl_ = new rUtf5(buf,siz); break;
	case UTF7:    impl_ = new rUtf7(buf,siz); break;
	case EucJP:   impl_ = new rIso2022(buf,siz,true,false,ASCII,JIS,KANA); break;
	case IsoJP:   impl_ = new rIso2022(buf,siz,false,false,ASCII,KANA); break;
	case IsoKR:   impl_ = new rIso2022(buf,siz,true,false,ASCII,KSX); break;
	case IsoCN:   impl_ = new rIso2022(buf,siz,true,false,ASCII,GB); break;
	case HZ:      impl_ = new rIso2022(buf,siz,true,true, ASCII,GB); break;
	default:      impl_ = new rMBCS(buf,siz,cs_); break;
	}

	return true;
}

int TextFileR::AutoDetection( int cs, const uchar* ptr, ulong siz )
{
//-- �܂��A�����̏o���񐔂̓��v�����

	int  freq[256];
	bool bit8 = false;
	mem00( freq, sizeof(freq) );
	for( ulong i=0; i<siz; ++i )
	{
		if( ptr[i] >= 0x80 )
			bit8 = true;
		++freq[ ptr[i] ];
	}

//-- ���s�R�[�h���� (UTF16/32/7�̂Ƃ���肠��BUTF5�Ɏ����Ă͔���s�c)

	     if( freq['\r'] > freq['\n']*2 ) lb_ = CR;
	else if( freq['\n'] > freq['\r']*2 ) lb_ = LF;
	else                                 lb_ = CRLF;
	nolbFound_ = freq['\r']==0 && freq['\n']==0;

//-- �f�t�H���g�R�[�h

	int defCs = ::GetACP();

//-- ����������ꍇ�͂����ŏI��

	if( siz <= 4 )
		return cs==AutoDetect ? defCs : cs;

//-- �����w�肪����ꍇ�͂����ŏI��

	ulong bom4 = (ptr[0]<<24) + (ptr[1]<<16) + (ptr[2]<<8) + (ptr[3]);
	ulong bom2 = (ptr[0]<<8)  + (ptr[1]);

	if( cs==UTF8 || cs==UTF8N )
		cs = (bom4>>8==0xefbbbf ? UTF8 : UTF8N);
	else if( cs==UTF32b || cs==UTF32BE )
		cs = (bom4==0x0000feff ? UTF32b : UTF32BE);
	else if( cs==UTF32l || cs==UTF32LE )
		cs = (bom4==0xfffe0000 ? UTF32l : UTF32LE);
	else if( cs==UTF16b || cs==UTF16BE )
		cs = (bom2==0xfeff ? UTF16b : UTF16BE);
	else if( cs==UTF16l || cs==UTF16LE )
		cs = (bom2==0xfffe ? UTF16l : UTF16LE);

	if( cs != AutoDetect )
		return cs;

//-- BOM�`�F�b�N�E7bit�`�F�b�N

	bool Jp = ::IsValidCodePage(932)!=FALSE;

	if( (bom4>>8) == 0xefbbbf )   cs = UTF8;
	else if( bom4 == 0x0000feff ) cs = UTF32b;
	else if( bom4 == 0xfffe0000 ) cs = UTF32l;
	else if( bom2 == 0xfeff )     cs = UTF16b;
	else if( bom2 == 0xfffe )     cs = UTF16l;
	else if( bom4 == 0x1b242943 && ::IsValidCodePage(949) ) cs = IsoKR;
	else if( bom4 == 0x1b242941 && ::IsValidCodePage(936) ) cs = IsoCN;
	else if( Jp && !bit8 && freq[0x1b]>0 )                  cs = IsoJP;

	if( cs != AutoDetect )
		return cs;

//-- UTF-5 �`�F�b�N

	ulong u5sum = 0;
	for( uchar c='0'; c<='9'; ++c ) u5sum += freq[c];
	for( uchar c='A'; c<='V'; ++c ) u5sum += freq[c];
	if( siz == u5sum )
		return UTF5;

//-- �b��� UTF-8 / ���{��EUC �`�F�b�N

	cs = defCs;

	// ���s�R�[�h��LF���A������x�̑傫�����A�łȂ���
	// �������� ANSI-CP �ƌ��Ȃ��Ă��܂��B
	if( bit8 && (siz>4096 || lb_==1
	 || freq[0xfd]>0 || freq[0xfe]>0 || freq[0xff]>0 || freq[0x80]>0) )
	{
		// UHC��GBK��EUC-JP�Ɣ��ɍ������₷���̂ŁA���������f�t�H���g�̏ꍇ��
		// EUC-JP���������؂�
		if( Jp && ::GetACP()!=UHC && ::GetACP()!=GBK && ::GetACP()!=Big5 )
		{
			// EUC�Ƃ��Ă��������l���������`�F�b�N
			bool be=true;
			for( int k=0x90; k<=0xa0; ++k )if( freq[k]>0 ){be=false;break;}
			for( int k=0x7f; k<=0x8d; ++k )if( freq[k]>0 ){be=false;break;}
			if( be )
				return EucJP;
		}
		{
			// UTF8�Ƃ��ēǂ߂邩�ǂ����`�F�b�N
			bool b8=true;
			int mi=1;
			for( ulong i=0; i<siz && b8; ++i )
				if( --mi )
				{
					if( ptr[i]<0x80 || ptr[i]>=0xc0 )
						b8 = false;
				}
				else
				{
					mi = 1;
					if( ptr[i] > 0x7f )
					{
						mi = GetMaskIndex( ptr[i] );
						if( mi == 1 )//ptr[i] >= 0xfe )
							b8 = false;
					}
				}
			if( b8 )
				return UTF8N;
		}
	}

//-- ���茋��

	return cs;
}



//=========================================================================
// �e�L�X�g�t�@�C���o�͋��ʃC���^�[�t�F�C�X
//=========================================================================

struct ki::TextFileWPimpl : public Object
{
	virtual void WriteLine( const unicode* buf, ulong siz )
		{ while( siz-- ) WriteChar( *buf++ ); }

	virtual void WriteLB( const unicode* buf, ulong siz )
		{ WriteLine( buf, siz ); }

	virtual void WriteChar( unicode ch )
		{}

	~TextFileWPimpl()
		{ delete [] buf_; }

protected:

	TextFileWPimpl( FileW& w )
		: fp_   (w)
		, bsiz_ (65536)
		, buf_  (new char[bsiz_]) {}

	void ReserveMoreBuffer()
		{
			char* nBuf = new char[bsiz_<<=1];
			delete [] buf_;
			buf_ = nBuf;
		}

	FileW& fp_;
	ulong  bsiz_;
	char*  buf_;
};



//-------------------------------------------------------------------------
// Unicode�e�L�X�g
//-------------------------------------------------------------------------

struct wUtf16LE : public TextFileWPimpl
{
	wUtf16LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom){ unicode ch=0xfeff; fp_.Write(&ch,2); } }
	void WriteLine( const unicode* buf, ulong siz ) {fp_.Write(buf,siz*2);}
};

struct wUtf16BE : public TextFileWPimpl
{
	wUtf16BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) WriteChar(0xfeff); }
	void WriteChar( unicode ch ) { fp_.WriteC(ch>>8), fp_.WriteC(ch&0xff); }
};

struct wUtf32LE : public TextFileWPimpl
{
	wUtf32LE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(ch&0xff), fp_.WriteC(ch>>8), fp_.WriteC(0), fp_.WriteC(0); }
	virtual void WriteLine( const unicode* buf, ulong siz )
	{
		while( siz-- )
		{
			unicode c = *buf++;
			qbyte  cc = c;
			if( (0xD800<=c && c<=0xDBFF) && siz>0 ) // trail char �����������ǂ����̓`�F�b�N����C���Ȃ�
			{
				unicode c2 = *buf++; siz--;
				cc = 0x10000 + (((c-0xD800)&0x3ff)<<10) + ((c2-0xDC00)&0x3ff);
			}
			for(int i=0; i<=3; ++i)
				fp_.WriteC( (uchar)(cc>>(8*i)) );
		}
	}
};

struct wUtf32BE : public TextFileWPimpl
{
	wUtf32BE( FileW& w, bool bom ) : TextFileWPimpl(w)
		{ if(bom) {unicode c=0xfeff; WriteLine(&c,1);} }
//	void WriteChar( unicode ch )
//		{ fp_.WriteC(0), fp_.WriteC(0), fp_.WriteC(ch>>8), fp_.WriteC(ch&0xff); }
	virtual void WriteLine( const unicode* buf, ulong siz )
	{
		while( siz-- )
		{
			unicode c = *buf++;
			qbyte  cc = c;
			if( (0xD800<=c && c<=0xDBFF) && siz>0 ) // trail char �����������ǂ����̓`�F�b�N����C���Ȃ�
			{
				unicode c2 = *buf++; siz--;
				cc = 0x10000 + (((c-0xD800)&0x3ff)<<10) + ((c2-0xDC00)&0x3ff);
			}
			for(int i=3; i>=0; --i)
				fp_.WriteC( (uchar)(cc>>(8*i)) );
		}
	}
};

struct wWest : public TextFileWPimpl
{
	wWest( FileW& w ) : TextFileWPimpl(w) {}
	void WriteChar( unicode ch ) { fp_.WriteC(ch>0xff ? '?' : (uchar)ch); }
};

struct wUtf5 : public TextFileWPimpl
{
	wUtf5( FileW& w ) : TextFileWPimpl(w) {}
	void WriteChar( unicode ch )
	{
		static const char conv[] = {
			'0','1','2','3','4','5','6','7',
				'8','9','A','B','C','D','E','F' };
		if(ch<0x10)
		{
			fp_.WriteC(ch+'G');
		}
		else if(ch<0x100)
		{
			fp_.WriteC((ch>>4)+'G');
			fp_.WriteC(conv[ch&0xf]);
		}
		else if(ch<0x1000)
		{
			fp_.WriteC((ch>>8)+'G');
			fp_.WriteC(conv[(ch>>4)&0xf]);
			fp_.WriteC(conv[ch&0xf]);
		}
		else
		{
			fp_.WriteC((ch>>12)+'G');
			fp_.WriteC(conv[(ch>>8)&0xf]);
			fp_.WriteC(conv[(ch>>4)&0xf]);
			fp_.WriteC(conv[ch&0xf]);
		}
	}
};



//-------------------------------------------------------------------------
// Win95�΍�̎��OUTF8/UTF7����
//-------------------------------------------------------------------------
#ifndef _UNICODE

struct wUTF8 : public TextFileWPimpl
{
	wUTF8( FileW& w, int cp )
		: TextFileWPimpl(w)
	{
		if( cp == UTF8 ) // BOM��������
			fp_.Write( "\xEF\xBB\xBF", 3 );
	}

	void WriteLine( const unicode* str, ulong len )
	{
		//        0000-0000-0xxx-xxxx | 0xxxxxxx
		//        0000-0xxx-xxyy-yyyy | 110xxxxx 10yyyyyy
		//        xxxx-yyyy-yyzz-zzzz | 1110xxxx 10yyyyyy 10zzzzzz
		// x-xxyy-yyyy-zzzz-zzww-wwww | 11110xxx 10yyyyyy 10zzzzzz 10wwwwww
		// ...
		while( len-- )
		{
			qbyte ch = *str;
			if( (0xD800<=ch&&ch<=0xDBFF) && len )
				ch = 0x10000 + (((ch-0xD800)&0x3ff)<<10) + ((*++str-0xDC00)&0x3ff), len--;

			if( ch <= 0x7f )
				fp_.WriteC( static_cast<uchar>(ch) );
			else if( ch <= 0x7ff )
				fp_.WriteC( 0xc0 | static_cast<uchar>(ch>>6) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0xffff )
				fp_.WriteC( 0xe0 | static_cast<uchar>(ch>>12) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0x1fffff )
				fp_.WriteC( 0xf0 | static_cast<uchar>(ch>>18) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else if( ch<= 0x3ffffff )
				fp_.WriteC( 0xf8 | static_cast<uchar>(ch>>24) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			else
				fp_.WriteC( 0xfc | static_cast<uchar>(ch>>30) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>24)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>18)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>12)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>((ch>>6)&0x3f) ),
				fp_.WriteC( 0x80 | static_cast<uchar>(ch&0x3f) );
			++str;
		}
	}
};



struct wUTF7 : public TextFileWPimpl
{
	wUTF7( FileW& w ) : TextFileWPimpl(w) {}

	void WriteLine( const unicode* str, ulong len )
	{
		static const uchar mime[64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'};

		// XxxxxxYyyyyyZzzz | zzWwwwwwUuuuuuVv | vvvvTtttttSsssss
		bool mode_m = false;
		while( len )
		{
			if( *str <= 0x7f )
			{
				fp_.WriteC( static_cast<uchar>(*str) );
				if( *str == L'+' )
					fp_.WriteC( '-' );
				++str, --len;
			}
			else
			{
				if(!mode_m) fp_.WriteC( '+' ), mode_m=true;
				unicode tx[3] = {0,0,0};
				int n=0;
				tx[0] = *str, ++str, --len, ++n;
				if( len && *str>0x7f )
				{
					tx[1] = *str, ++str, --len, ++n;
					if( len && *str>0x7f )
						tx[2] = *str, ++str, --len, ++n;
				}
				{
					fp_.WriteC( mime[ tx[0]>>10 ] );
					fp_.WriteC( mime[ (tx[0]>>4)&0x3f ] );
					fp_.WriteC( mime[ (tx[0]<<2|tx[1]>>14)&0x3f ] );
					if( n>=2 )
					{
						fp_.WriteC( mime[ (tx[1]>>8)&0x3f ] );
						fp_.WriteC( mime[ (tx[1]>>2)&0x3f ] );
						fp_.WriteC( mime[ (tx[1]<<4|tx[2]>>12)&0x3f ] );
						if( n>=3 )
						{
							fp_.WriteC( mime[ (tx[2]>>6)&0x3f ] );
							fp_.WriteC( mime[ tx[2]&0x3f ] );
						}
					}
				}
				if( len && *str<=0x7f )
					fp_.WriteC( '-' ), mode_m = false;
			}
		}
	}
};



#endif
//-------------------------------------------------------------------------
// Windows����̕ϊ�
//-------------------------------------------------------------------------

struct wMBCS : public TextFileWPimpl
{
	wMBCS( FileW& w, int cp )
		: TextFileWPimpl(w), cp_(cp)
	{
		if( cp == UTF8 )
		{
			// BOM��������
			cp_ = UTF8N;
			fp_.Write( "\xEF\xBB\xBF", 3 );
		}
	}

	void WriteLine( const unicode* str, ulong len )
	{
		// WideCharToMultiByte API �𗘗p�����ϊ�
		int r;
		while(
			0==(r=::WideCharToMultiByte(cp_,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		// �t�@�C���֏�������
		fp_.Write( buf_, r );
	}

	int cp_;
};



//-------------------------------------------------------------------------
// ISO-2022 �T�u�Z�b�g���̂P�B
// ASCII�Ƃ�������������W�����g��Ȃ�����
//-------------------------------------------------------------------------

struct wIso2022 : public TextFileWPimpl
{
	wIso2022( FileW& w, int cp )
		: TextFileWPimpl(w), hz_(cp==HZ)
	{
		switch( cp )
		{
		case IsoKR:
			fp_.Write( "\x1B\x24\x29\x43", 4 );
			cp_ = UHC;
			break;

		case IsoCN:
			fp_.Write( "\x1B\x24\x29\x41", 4 );
			// fall through...
		default:
			cp_ = GBK;
			break;
		}
	}

	void WriteLine( const unicode* str, ulong len )
	{
		// �܂� WideCharToMultiByte API �𗘗p���ĕϊ�
		int r;
		while(
			0==(r=::WideCharToMultiByte(cp_,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		bool ascii = true;
		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				// ��ASCII�����͍ŏ�ʃr�b�g�𗎂Ƃ��Ă���o��
				if( ascii )
				{
					if( hz_ )
						fp_.WriteC( 0x7E ), fp_.WriteC( 0x7B );
					else
						fp_.WriteC( 0x0E );
					ascii = false;
				}
				fp_.WriteC( buf_[i++] & 0x7F );
				fp_.WriteC( buf_[i]   & 0x7F );
			}
			else
			{
				// ASCII�����͂��̂܂܏o��
				if( !ascii )
				{
					if( hz_ )
						fp_.WriteC( 0x7E ), fp_.WriteC( 0x7D );
					else
						fp_.WriteC( 0x0F );
					ascii = true;
				}
				fp_.WriteC( buf_[i] );

				// ������HZ�̏ꍇ�A0x7E �� 0x7E 0x7E �ƕ\��
				if( hz_ && buf_[i]==0x7E )
					fp_.WriteC( 0x7E );
			}

		// �Ō�͊m����ASCII�ɖ߂�
		if( !ascii )
			if( hz_ )
				fp_.WriteC( 0x7E ), fp_.WriteC( 0x7D );
			else
				fp_.WriteC( 0x0F );
	}

	int  cp_;
	bool hz_;
};




//-------------------------------------------------------------------------
// ISO-2022 �T�u�Z�b�g���̂Q�B���{��EUC
//-------------------------------------------------------------------------

// Helper: SJIS ==> JIS X 0208
static void sjis2jis( uchar s1, uchar s2, char* k )
{
	if( ((s1==0xfa || s1==0xfb) && s2>=0x40) || (s1==0xfc && (s2&0xf0)==0x40) )
	{
		// IBM�O���̃}�b�s���O
static const WORD IBM_sjis2kuten[] = {
/*fa40*/0x5c51,0x5c52,0x5c53,0x5c54,0x5c55,0x5c56,0x5c57,0x5c58,0x5c59,0x5c5a,0x0d15,0x0d16,0x0d17,0x0d18,0x0d19,0x0d1a,
/*fa50*/0x0d1b,0x0d1c,0x0d1d,0x0d1e,0x022c,0x5c5c,0x5c5d,0x5c5e,0x0d4a,0x0d42,0x0d44,0x0248,0x5901,0x5902,0x5903,0x5904,
/*fa60*/0x5905,0x5906,0x5907,0x5908,0x5909,0x590a,0x590b,0x590c,0x590d,0x590e,0x590f,0x5910,0x5911,0x5912,0x5913,0x5914,
/*fa70*/0x5915,0x5916,0x5917,0x5918,0x5919,0x591a,0x591b,0x591c,0x591d,0x591e,0x591f,0x5920,0x5921,0x5922,0x5923,/**/0x0109,
/*fa80*/0x5924,0x5925,0x5926,0x5927,0x5928,0x5929,0x592a,0x592b,0x592c,0x592d,0x592e,0x592f,0x5930,0x5931,0x5932,0x5933,
/*fa90*/0x5934,0x5935,0x5936,0x5937,0x5938,0x5939,0x593a,0x593b,0x593c,0x593d,0x593e,0x593f,0x5940,0x5941,0x5942,0x5943,
/*faa0*/0x5944,0x5945,0x5946,0x5947,0x5948,0x5949,0x594a,0x594b,0x594c,0x594d,0x594e,0x594f,0x5950,0x5951,0x5952,0x5953,
/*fab0*/0x5954,0x5955,0x5956,0x5957,0x5958,0x5959,0x595a,0x595b,0x595c,0x595d,0x595e,0x5a01,0x5a02,0x5a03,0x5a04,0x5a05,
/*fac0*/0x5a06,0x5a07,0x5a08,0x5a09,0x5a0a,0x5a0b,0x5a0c,0x5a0d,0x5a0e,0x5a0f,0x5a10,0x5a11,0x5a12,0x5a13,0x5a14,0x5a15,
/*fad0*/0x5a16,0x5a17,0x5a18,0x5a19,0x5a1a,0x5a1b,0x5a1c,0x5a1d,0x5a1e,0x5a1f,0x5a20,0x5a21,0x5a22,0x5a23,0x5a24,0x5a25,
/*fae0*/0x5a26,0x5a27,0x5a28,0x5a29,0x5a2a,0x5a2b,0x5a2c,0x5a2d,0x5a2e,0x5a2f,0x5a30,0x5a31,0x5a32,0x5a33,0x5a34,0x5a35,
/*faf0*/0x5a36,0x5a37,0x5a38,0x5a39,0x5a3a,0x5a3b,0x5a3c,0x5a3d,0x5a3e,0x5a3f,0x5a40,0x5a41,0x5a42,/**/0x0109,0x0109,0x0109,
/*fb40*/0x5a43,0x5a44,0x5a45,0x5a46,0x5a47,0x5a48,0x5a49,0x5a4a,0x5a4b,0x5a4c,0x5a4d,0x5a4e,0x5a4f,0x5a50,0x5a51,0x5a52,
/*fb50*/0x5a53,0x5a54,0x5a55,0x5a56,0x5a57,0x5a58,0x5a59,0x5a5a,0x5a5b,0x5a5c,0x5a5d,0x5a5e,0x5b01,0x5b02,0x5b03,0x5b04,
/*fb60*/0x5b05,0x5b06,0x5b07,0x5b08,0x5b09,0x5b0a,0x5b0b,0x5b0c,0x5b0d,0x5b0e,0x5b0f,0x5b10,0x5b11,0x5b12,0x5b13,0x5b14,
/*fb70*/0x5b15,0x5b16,0x5b17,0x5b18,0x5b19,0x5b1a,0x5b1b,0x5b1c,0x5b1d,0x5b1e,0x5b1f,0x5b20,0x5b21,0x5b22,0x5b23,/**/0x0109,
/*fb80*/0x5b24,0x5b25,0x5b26,0x5b27,0x5b28,0x5b29,0x5b2a,0x5b2b,0x5b2c,0x5b2d,0x5b2e,0x5b2f,0x5b30,0x5b31,0x5b32,0x5b33,
/*fb90*/0x5b34,0x5b35,0x5b36,0x5b37,0x5b38,0x5b39,0x5b3a,0x5b3b,0x5b3c,0x5b3d,0x5b3e,0x5b3f,0x5b40,0x5b41,0x5b42,0x5b43,
/*fba0*/0x5b44,0x5b45,0x5b46,0x5b47,0x5b48,0x5b49,0x5b4a,0x5b4b,0x5b4c,0x5b4d,0x5b4e,0x5b4f,0x5b50,0x5b51,0x5b52,0x5b53,
/*fbb0*/0x5b54,0x5b55,0x5b56,0x5b57,0x5b58,0x5b59,0x5b5a,0x5b5b,0x5b5c,0x5b5d,0x5b5e,0x5c01,0x5c02,0x5c03,0x5c04,0x5c05,
/*fbc0*/0x5c06,0x5c07,0x5c08,0x5c09,0x5c0a,0x5c0b,0x5c0c,0x5c0d,0x5c0e,0x5c0f,0x5c10,0x5c11,0x5c12,0x5c13,0x5c14,0x5c15,
/*fbd0*/0x5c16,0x5c17,0x5c18,0x5c19,0x5c1a,0x5c1b,0x5c1c,0x5c1d,0x5c1e,0x5c1f,0x5c20,0x5c21,0x5c22,0x5c23,0x5c24,0x5c25,
/*fbe0*/0x5c26,0x5c27,0x5c28,0x5c29,0x5c2a,0x5c2b,0x5c2c,0x5c2d,0x5c2e,0x5c2f,0x5c30,0x5c31,0x5c32,0x5c33,0x5c34,0x5c35,
/*fbf0*/0x5c36,0x5c37,0x5c38,0x5c39,0x5c3a,0x5c3b,0x5c3c,0x5c3d,0x5c3e,0x5c3f,0x5c40,0x5c41,0x5c42,/**/0x0109,0x0109,0x0109,
/*fc40*/0x5c43,0x5c44,0x5c45,0x5c46,0x5c47,0x5c48,0x5c49,0x5c4a,0x5c4b,0x5c4c,0x5c4d,0x5c4e,/**/0x0109,0x0109,0x0109,0x0109,
};
		k[0] = IBM_sjis2kuten[ (s1-0xfa)*12*16 + (s2-0x40) ]>>8;
		k[1] = IBM_sjis2kuten[ (s1-0xfa)*12*16 + (s2-0x40) ]&0xff;
	}
	else
	{
		// ���̑�
		if( s2>=0x9f )
		{
			if( s1>=0xe0 ) k[0] = ((s1-0xc0)<<1);
			else           k[0] = ((s1-0x80)<<1);
			k[1] = s2-0x9e;
		}
		else
		{
			if( s1>=0xe0 ) k[0] = ((s1-0xc0)<<1)-1;
			else           k[0] = ((s1-0x80)<<1)-1;

			if( s2 & 0x80 )	k[1] = s2-0x40;
			else			k[1] = s2-0x3f;
		}
	}

	k[0] += 0x20;
	k[1] += 0x20;
}

struct wEucJp : public TextFileWPimpl
{
	wEucJp( FileW& w )
		: TextFileWPimpl(w) {}

	void WriteLine( const unicode* str, ulong len )
	{
		// �܂� WideCharToMultiByte API �𗘗p���ĕϊ�
		int r;
		while(
			0==(r=::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				if( 0xA1<=(uchar)buf_[i] && (uchar)buf_[i]<=0xDF )
				{
					// �J�i
					fp_.WriteC( 0x8E );
					fp_.WriteC( buf_[i] );
				}
				else
				{
					// JIS X 0208
					sjis2jis( buf_[i], buf_[i+1], buf_+i );
					fp_.WriteC( buf_[i++] | 0x80 );
					fp_.WriteC( buf_[i]   | 0x80 );
				}
			}
			else
			{
				// ASCII�����͂��̂܂܏o��
				fp_.WriteC( buf_[i] );
			}
	}
};



//-------------------------------------------------------------------------
// ISO-2022 �T�u�Z�b�g���̂R�BISO-2022-JP
//-------------------------------------------------------------------------

struct wIsoJp : public TextFileWPimpl
{
	wIsoJp( FileW& w )
		: TextFileWPimpl(w)
	{
		fp_.Write( "\x1b\x28\x42", 3 );
	}

	void WriteLine( const unicode* str, ulong len )
	{
		// �܂� WideCharToMultiByte API �𗘗p���ĕϊ�
		int r;
		while(
			0==(r=::WideCharToMultiByte(932,0,str,len,buf_,bsiz_,NULL,NULL))
		 && ::GetLastError()==ERROR_INSUFFICIENT_BUFFER )
			ReserveMoreBuffer();

		enum { ROMA, KANJI, KANA } state = ROMA;
		for( int i=0; i<r; ++i )
			if( buf_[i] & 0x80 )
			{
				if( 0xA1<=(uchar)buf_[i] && (uchar)buf_[i]<=0xDF )
				{
					// �J�i
					if( state != KANA )
						fp_.Write( "\x1b\x28\x49", 3 ), state = KANA;
					fp_.WriteC( buf_[i] & 0x7f );
				}
				else
				{
					// JIS X 0208
					if( state != KANJI )
						fp_.Write( "\x1b\x24\x42", 3 ), state = KANJI;
					sjis2jis( buf_[i], buf_[i+1], buf_+i );
					fp_.WriteC( buf_[i++] );
					fp_.WriteC( buf_[i]   );
				}
			}
			else
			{
				// ASCII�����͂��̂܂܏o��
				if( state != ROMA )
					fp_.Write( "\x1b\x28\x42", 3 ), state = ROMA;
				fp_.WriteC( buf_[i] );
			}

		if( state != ROMA )
			fp_.Write( "\x1b\x28\x42", 3 ), state = ROMA;
	}
};




//-------------------------------------------------------------------------
// �������݃��[�`���̏������X
//-------------------------------------------------------------------------

TextFileW::TextFileW( int charset, int linebreak )
	: cs_( charset ), lb_(linebreak)
{
}

TextFileW::~TextFileW()
{
	Close();
}

void TextFileW::Close()
{
	impl_ = NULL;
	fp_.Close();
}

void TextFileW::WriteLine( const unicode* buf, ulong siz, bool lastline )
{
	impl_->WriteLine( buf, siz );
	if( !lastline )
	{
		static const ulong lbLst[] = {0x0D, 0x0A, 0x000A000D};
		static const ulong lbLen[] = {   1,    1,          2};
		impl_->WriteLB(
			reinterpret_cast<const unicode*>(&lbLst[lb_]), lbLen[lb_] );
	}
}

bool TextFileW::Open( const TCHAR* fname )
{
	if( !fp_.Open( fname, true ) )
		return false;

	switch( cs_ )
	{
	case Western: impl_ = new wWest( fp_ ); break;
	case UTF5:    impl_ = new wUtf5( fp_ ); break;
	case UTF16l:
	case UTF16LE: impl_ = new wUtf16LE( fp_, cs_==UTF16l ); break;
	case UTF16b:
	case UTF16BE: impl_ = new wUtf16BE( fp_, cs_==UTF16b ); break;
	case UTF32l:
	case UTF32LE: impl_ = new wUtf32LE( fp_, cs_==UTF32l ); break;
	case UTF32b:
	case UTF32BE: impl_ = new wUtf32BE( fp_, cs_==UTF32b ); break;
	case EucJP:   impl_ = new wEucJp( fp_ ); break;
	case IsoJP:   impl_ = new wIsoJp( fp_ ); break;
	case IsoKR:   impl_ = new wIso2022( fp_, cs_ ); break;
	case IsoCN:   impl_ = new wIso2022( fp_, cs_ ); break;
	case HZ:      impl_ = new wIso2022( fp_, cs_ ); break;
	case UTF8:
	case UTF8N:
	default:
#ifndef _UNICODE
		if( app().isWin95() && (cs_==UTF8 || cs_==UTF8N) )
			impl_ = new wUTF8( fp_, cs_ );
		else if( app().isWin95() && cs_==UTF7 )
			impl_ = new wUTF7( fp_ );
		else
#endif
		impl_ = new wMBCS( fp_, cs_ );
		break;
	}
	return true;
}
