/*
 * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include <JagGlobalDef.h>

#include <JagLang.h>
#include <utfcpp/utf8.h>
#include <JagUtil.h>

/************
GB 2312 对任意一个图形字符都采用两个字节表示，并对所收汉字进行了“分区”处理，每区含有 94 个汉字／符号，
分别对应第一字节和第二字节。这种表示方式也称为区位码。
01-09 区为特殊符号。
16-55 区为一级汉字，按拼音排序。
56-87 区为二级汉字，按部首／笔画排序。
10-15 区及 88-94 区则未有编码。
GB 2312 的编码范围为 2121H-777EH，与 ASCII 有重叠，通行方法是将 GB 码两个字节的最高位置 1 以示区别。


GBK 采用双字节表示，总体编码范围为 8140-FEFE 之间，首字节在 81-FE 之间，尾字节在 40-FE 之间，剔除 XX7F 一条线。GBK 编码区分三部分：
汉字区　包括
GBK/2：OXBOA1-F7FE, 收录 GB 2312 汉字 6763 个，按原序排列；
GBK/3：OX8140-AOFE，收录 CJK 汉字 6080 个；
GBK/4：OXAA40-FEAO，收录 CJK 汉字和增补的汉字 8160 个。
图形符号区　包括
GBK/1：OXA1A1-A9FE，除 GB 2312 的符号外，还增补了其它符号
GBK/5：OXA840-A9AO，扩除非汉字区。
用户自定义区
GBK 区域中的空白区，用户可以自己定义字符。


GB 18030，全称：国家标准 GB 18030-2005《信息技术中文编码字符集》，是中华人民共和国现时最新的内码字集，
是 GB 18030-2000《信息技术信息交换用汉字编码字符集基本集的扩充》的修订版。
GB 18030 与 GB 2312-1980 和 GBK 兼容，共收录汉字70244个。
与 UTF-8 相同，采用多字节编码，每个字可以由 1 个、2 个或 4 个字节组成。
编码空间庞大，最多可定义 161 万个字符。
支持中国国内少数民族的文字，不需要动用造字区。
汉字收录范围包含繁体汉字以及日韩汉字
GB 18030 编码是一二四字节变长编码。
单字节，其值从 0 到 0x7F，与 ASCII 编码兼容。
双字节，第一个字节的值从 0x81 到 0xFE，第二个字节的值从 0x40 到 0xFE（不包括0x7F），与 GBK 标准兼容。
四字节，第一个字节的值从 0x81 到 0xFE，第二个字节的值从 0x30 到 0x39，第三个字节从0x81 到 0xFE，第四个字节从 0x30 到 0x39。
************/

JagLang::JagLang()
{
	_vec = new JagVector<Jstr>(16);
}

JagLang::~JagLang()
{
	delete _vec;
}


jagint JagLang::parse( const char *instr, const char *encode )
{
	jagint n = 0;
	if ( 0 == strcasecmp( encode, "UTF8" ) ||
		 0 == strcasecmp( encode, "UTF-8" ) ) {
		 n = JagLang::_parseUTF8( instr );
		 return n;
	}

	if ( 0 == strcasecmp( encode, "GB2312" ) ||
		 0 == strcasecmp( encode, "GBK" ) ) {
		 n = JagLang::_parseGB2312( instr );
		 return n;
	}

	if ( 0 == strcasecmp( encode, "GB18030" ) ) {
		 n = JagLang::_parseGB18030( instr );
		 return n;
	}

	return -1;
}

jagint JagLang::length()
{
	return (*_vec).length();
}

jagint  JagLang:: size()
{
	return (*_vec).length();
}

Jstr JagLang::at(int i)
{
	return (*_vec)[i];
}

jagint JagLang::_parseUTF8( const char *instr )
{
    char* str = (char*)instr;    
    char* str_i = (char*)str; 
    char* end = str+ strlen(instr)+1; 
    unsigned char symbol[5] = {0,0,0,0,0};
	uint32_t code;
	jagint n = 0;
    do {
        code = utf8::next(str_i, end);
        if (code == 0) continue;
        utf8::append(code, symbol); 
		_vec->append( (char*)symbol );
		++n;
    } while ( str_i < end );	

	return n;
}

void JagLang::rangeFixString( int buflen, int start, int len, JagFixString &res )
{
	if ( start  < 0 ) start = 0;

	char *buf = (char*)jagmalloc( buflen + 1 );
	memset( buf, 0, buflen+1 );
	if ( len <= 0 ) {
		res = JagFixString( buf, buflen );
		free( buf );
		return;
	}

	int pos = 0;
	int cnt = 0;
	for ( int i = start; i < _vec->size(); ++i ) {
		memcpy( buf+pos, (*_vec)[i].c_str(), (*_vec)[i].size() );
		pos += (*_vec)[i].size();
		++ cnt;
		if ( cnt >= len ) break;
	}

	res = JagFixString( buf, buflen );
	free( buf );
}


jagint JagLang::_parseGB2312( const char *instr )
{
	jagint n = 0;
	char b1[2];
	char b2[3];
	char *p = (char*) instr;

	while ( *p != '\0' ) {
		if ( ! (*p & 0x80 ) ) {
			b1[0] = *p;
			b1[1] = '\0';
			_vec->append( (char*)b1 );
			++n;
		} else {
			b2[0] = b2[1] = b2[2] = '\0';
			b2[0] = *p;
			++p; if ( *p == '\0' ) { break; }
			b2[1] = *p;
			_vec->append( (char*)b2 );
			++n;
		}

		p++;
	}

	return n;
}

jagint JagLang::_parseGB18030( const char *instr )
{
	jagint n = 0;
	char b1[2];
	char b2[3];
	char b4[5];
	char *p = (char*) instr;

	while ( *p != '\0' ) {
		if ( ! (*p & 0x80 ) ) {
			b1[0] = *p;
			b1[1] = '\0';
			_vec->append( (char*)b1 );
			++n;
		} else {
			b2[0] = *p; b2[1] = b2[2] = '\0'; 

			++p; if ( *p == '\0' ) break;
			b2[1] = *p; 

			if ( b2[1] >= 0x40 ) {
				_vec->append( (char*)b2 );
				++n;
			} else {
				b4[0] = b2[0]; b4[1] = b2[1]; 
				b4[2]= b4[3] = b4[4] = '\0';
				++p; if ( *p == '\0' ) break;
				b4[2] = *p;
				++p; if ( *p == '\0' ) break;
				b4[3] = *p; 
				_vec->append( (char*)b4 );
				++n;
			}
		}

		p++;
	}

	return n;
}
