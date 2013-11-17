#ifndef _KILIB_TEXTFILE_H_
#define _KILIB_TEXTFILE_H_
#include "types.h"
#include "ktlaptr.h"
#include "memory.h"
#include "file.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	���p�\�R�[�h�Z�b�g
//
//	�������A�����Ń��X�g�A�b�v���ꂽ���̂̂����AWindows�ɂ�����
//	����T�|�[�g���C���X�g�[������Ă�����̂��������ۂɂ͑Ή��\�B
//	�l��-100��菬�����R�[�h�́A���̂�����ɂ���R�[�h�y�[�W�̌���
//	�T�|�[�g�𗘗p���ĕϊ����s�����߁A����Ɉˑ�����B
//@}
//=========================================================================

enum charset {
	AutoDetect = 0,    // ��������
	                   // SJIS/EucJP/IsoJP/IsoKR/IsoCN
					   // UTF5/UTF8/UTF8N/UTF16b/UTF16l/UTF32b/UTF32l
					   // �𔻒肷��B���͒m��Ȃ��B(^^;

	Western    = 1252, // ����      (Windows1252 >> ISO-8859-1)
	Turkish    = 1254, // �g���R��  (Windows1254 >> ISO-8859-9)
	Hebrew     = 1255, // �w�u���C��(Windows1255 >> ISO-8859-8)
	Arabic     = 1256, // �A���r�A��(Windows1256 �` ISO-8859-6)
	Baltic     = 1257, // �o���g��  (Windows1257 >> ISO-8859-13)
	Vietnamese = 1258, // �x�g�i����(Windows1258 != VISCII)
	Central    = 1250, // ����ְۯ��(Windows1250 �` ISO-8859-2)
	Greek      = 1253, // �M���V����(Windows1253 �` ISO-8859-7)
	Thai       = 874,  // �^�C��

	Cyrillic   = 1251, // �L������(Windows1251 != ISO-8859-5)
	Koi8R      = 20866,// �L������(KOI8-R)
	Koi8U      = 21866,// �L������(KOI8-U �E�N���C�i�n)

	UHC        = 949,  // �؍���P (Unified Hangle Code >> EUC-KR)
	IsoKR      = -950, // �؍���Q (ISO-2022-KR)
	Johab      = 1361, // �؍���R (Johab)

	GBK        = 936,  // ������P (�ȑ̎� GBK >> EUC-CN)
	IsoCN      = -936, // ������Q (�ȑ̎� ISO-2022-CN)
	HZ         = -937, // ������R (�ȑ̎� HZ-GB2312)
	Big5       = 950,  // ������S (�ɑ̎� Big5)

	SJIS       = 932,  // ���{��P (Shift_JIS)
	EucJP      = -932, // ���{��Q (���{��EUC)
	IsoJP      = -933, // ���{��R (ISO-2022-JP)

	UTF5       = -2,   // Unicode  (UTF-5)   : BOM����
	UTF7       = 65000,// Unicode  (UTF-7)   : BOM����
	UTF8       =-65001,// Unicode  (UTF-8)   : BOM�L��
	UTF8N      = 65001,// Unicode  (UTF-8N)  : BOM����
	UTF16b     = -3,   // Unicode  (UTF-16)  : BOM�L�� BE
	UTF16l     = -4,   // Unicode  (UTF-16)  : BOM�L�� LE
	UTF16BE    = -5,   // Unicode  (UTF-16BE): BOM����
	UTF16LE    = -6,   // Unicode  (UTF-16LE): BOM����
	UTF32b     = -7,   // Unicode  (UTF-32)  : BOM�L�� BE
	UTF32l     = -8,   // Unicode  (UTF-32)  : BOM�L�� LE
	UTF32BE    = -9,   // Unicode  (UTF-32BE): BOM����
	UTF32LE    = -10,  // Unicode  (UTF-32LE): BOM����

	DOSUS      = 437   // DOSLatinUS (CP437)
};

//=========================================================================
//@{
//	���s�R�[�h
//@}
//=========================================================================

enum lbcode {
	CR   = 0,
	LF   = 1,
	CRLF = 2
};

struct TextFileRPimpl;
struct TextFileWPimpl;



//=========================================================================
//@{
//	�e�L�X�g�t�@�C���Ǎ�
//
//	�t�@�C�����w�肳�ꂽ�����R�[�h�ŉ��߂��AUnicode������Ƃ���
//	��s���ɕԂ��B�����R�[�h����s�R�[�h�̎���������\�B
//@}
//=========================================================================

class TextFileR : public Object
{
public:

	//@{ �R���X�g���N�^�i�R�[�h�w��j//@}
	TextFileR( int charset=AutoDetect );

	//@{ �f�X�g���N�^ //@}
	~TextFileR();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname );

	//@{ ���� //@}
	void Close();

	//@{
	//	�ǂݍ��� (�ǂ񂾒�����Ԃ�)
	//
	//	���Ȃ��Ƃ�20���炢�̃T�C�Y���m�ۂ����o�b�t�@���w�肵�Ă��������B
	//@}
	size_t ReadLine( unicode* buf, ulong siz );

public:

	//@{ �ǂ�ł�t�@�C���̃R�[�h�y�[�W //@}
	int codepage() const;

	//@{ ���s�R�[�h (0:CR, 1:LF, 2:CRLF) //@}
	int linebreak() const;

	//@{ �ǂݍ��ݏ� (0:EOF, 1:EOL, 2:EOB) //@}
	int state() const;

	//@{ �t�@�C���T�C�Y //@}
	ulong size() const;

	//@{ ���s�����������Ȃ������t���O //@}
	bool nolb_found() const;
private:

	dptr<TextFileRPimpl> impl_;
	FileR                fp_;
	int                  cs_;
	int                  lb_;
	bool          nolbFound_;

private:

	int AutoDetection( int cs, const uchar* ptr, ulong siz );

private:

	NOCOPY(TextFileR);
};



//-------------------------------------------------------------------------

inline int TextFileR::codepage() const
	{ return cs_; }

inline int TextFileR::linebreak() const
	{ return lb_; }

inline ulong TextFileR::size() const
	{ return fp_.size(); }

inline bool TextFileR::nolb_found() const
	{ return nolbFound_; }


//=========================================================================
//@{
//	�e�L�X�g�t�@�C������
//
//	Unicode��������󂯎��A�w�肳�ꂽ�����R�[�h�ɕϊ����Ȃ���o�͂���B
//@}
//=========================================================================

class TextFileW : public Object
{
public:

	//@{ �R���X�g���N�^�i����,���s�R�[�h�w��j//@}
	TextFileW( int charset, int linebreak );
	~TextFileW();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname );

	//@{ ���� //@}
	void Close();

	//@{ ��s�����o�� //@}
	void WriteLine( const unicode* buf, ulong siz, bool lastline );

private:

	dptr<TextFileWPimpl> impl_;
	FileW                fp_;
	const int            cs_;
	const int            lb_;

private:

	NOCOPY(TextFileW);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_TEXTFILE_H_
