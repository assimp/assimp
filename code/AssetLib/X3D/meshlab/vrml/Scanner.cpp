
/****************************************************************************
* MeshLab                                                           o o     *
* An extendible mesh processor                                    o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005, 2006                                          \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
 History
 $Log$
 Revision 1.2  2008/02/22 17:58:42  cignoni
 New version of coco_swprintf that supports also mingw compiler

 Revision 1.1  2008/02/20 22:02:00  gianpaolopalma
 First working version of Vrml to X3D translator

*****************************************************************************/
#include <memory.h>
#include <string.h>
#include "Scanner.h"

// string handling, wide character

wchar_t* coco_string_create(const wchar_t* value) {
	wchar_t* data;
	int len = 0;
	if (value) { len = wcslen(value); }
	data = new wchar_t[len + 1];
	wcsncpy(data, value, len);
	data[len] = 0;
	return data;
}

wchar_t* coco_string_create(const wchar_t *value , int startIndex, int length) {
	int len = 0;
	wchar_t* data;

	if (value) { len = length; }
	data = new wchar_t[len + 1];
	wcsncpy(data, &(value[startIndex]), len);
	data[len] = 0;

	return data;
}

wchar_t* coco_string_create_upper(const wchar_t* data) {
	if (!data) { return NULL; }

	int dataLen = 0;
	if (data) { dataLen = wcslen(data); }

	wchar_t *newData = new wchar_t[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		if ((L'a' <= data[i]) && (data[i] <= L'z')) {
			newData[i] = data[i] + (L'A' - L'a');
		}
		else { newData[i] = data[i]; }
	}

	newData[dataLen] = L'\0';
	return newData;
}

wchar_t* coco_string_create_lower(const wchar_t* data) {
	if (!data) { return NULL; }
	int dataLen = wcslen(data);
	return coco_string_create_lower(data, 0, dataLen);
}

wchar_t* coco_string_create_lower(const wchar_t* data, int startIndex, int dataLen) {
	if (!data) { return NULL; }

	wchar_t* newData = new wchar_t[dataLen + 1];

	for (int i = 0; i <= dataLen; i++) {
		wchar_t ch = data[startIndex + i];
		if ((L'A' <= ch) && (ch <= L'Z')) {
			newData[i] = ch - (L'A' - L'a');
		}
		else { newData[i] = ch; }
	}
	newData[dataLen] = L'\0';
	return newData;
}

wchar_t* coco_string_create_append(const wchar_t* data1, const wchar_t* data2) {
	wchar_t* data;
	int data1Len = 0;
	int data2Len = 0;

	if (data1) { data1Len = wcslen(data1); }
	if (data2) {data2Len = wcslen(data2); }

	data = new wchar_t[data1Len + data2Len + 1];

	if (data1) { wcscpy(data, data1); }
	if (data2) { wcscpy(data + data1Len, data2); }

	data[data1Len + data2Len] = 0;

	return data;
}

wchar_t* coco_string_create_append(const wchar_t *target, const wchar_t appendix) {
	int targetLen = coco_string_length(target);
	wchar_t* data = new wchar_t[targetLen + 2];
	wcsncpy(data, target, targetLen);
	data[targetLen] = appendix;
	data[targetLen + 1] = 0;
	return data;
}

void coco_string_delete(wchar_t* &data) {
	delete [] data;
	data = NULL;
}

int coco_string_length(const wchar_t* data) {
	if (data) { return wcslen(data); }
	return 0;
}

bool coco_string_endswith(const wchar_t* data, const wchar_t *end) {
	int dataLen = wcslen(data);
	int endLen = wcslen(end);
	return (endLen <= dataLen) && (wcscmp(data + dataLen - endLen, end) == 0);
}

int coco_string_indexof(const wchar_t* data, const wchar_t value) {
	const wchar_t* chr = wcschr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

int coco_string_lastindexof(const wchar_t* data, const wchar_t value) {
	const wchar_t* chr = wcsrchr(data, value);

	if (chr) { return (chr-data); }
	return -1;
}

void coco_string_merge(wchar_t* &target, const wchar_t* appendix) {
	if (!appendix) { return; }
	wchar_t* data = coco_string_create_append(target, appendix);
	delete [] target;
	target = data;
}

bool coco_string_equal(const wchar_t* data1, const wchar_t* data2) {
	return wcscmp( data1, data2 ) == 0;
}

int coco_string_compareto(const wchar_t* data1, const wchar_t* data2) {
	return wcscmp(data1, data2);
}

int coco_string_hash(const wchar_t *data) {
	int h = 0;
	if (!data) { return 0; }
	while (*data != 0) {
		h = (h * 7) ^ *data;
		++data;
	}
	if (h < 0) { h = -h; }
	return h;
}

// string handling, ascii character

wchar_t* coco_string_create(const char* value) {
	int len = 0;
	if (value) { len = strlen(value); }
	wchar_t* data = new wchar_t[len + 1];
	for (int i = 0; i < len; ++i) { data[i] = (wchar_t) value[i]; }
	data[len] = 0;
	return data;
}

char* coco_string_create_char(const wchar_t *value) {
	int len = coco_string_length(value);
	char *res = new char[len + 1];
	for (int i = 0; i < len; ++i) { res[i] = (char) value[i]; }
	res[len] = 0;
	return res;
}

void coco_string_delete(char* &data) {
	delete [] data;
	data = NULL;
}


namespace VrmlTranslator {


Token::Token() {
	kind = 0;
	pos  = 0;
	col  = 0;
	line = 0;
	val  = NULL;
	next = NULL;
}

Token::~Token() {
	coco_string_delete(val);
}

Buffer::Buffer(FILE* s, bool isUserStream) {
// ensure binary read on windows
#if _MSC_VER >= 1300
	_setmode(_fileno(s), _O_BINARY);
#endif
	stream = s; this->isUserStream = isUserStream;
	if (CanSeek()) {
		fseek(s, 0, SEEK_END);
		fileLen = ftell(s);
		fseek(s, 0, SEEK_SET);
		bufLen = (fileLen < MAX_BUFFER_LENGTH) ? fileLen : MAX_BUFFER_LENGTH;
		bufStart = INT_MAX; // nothing in the buffer so far
	} else {
		fileLen = bufLen = bufStart = 0;
	}
	bufCapacity = (bufLen>0) ? bufLen : MIN_BUFFER_LENGTH;
	buf = new unsigned char[bufCapacity];	
	if (fileLen > 0) SetPos(0);          // setup  buffer to position 0 (start)
	else bufPos = 0; // index 0 is already after the file, thus Pos = 0 is invalid
	if (bufLen == fileLen && CanSeek()) Close();
}

Buffer::Buffer(Buffer *b) {
	buf = b->buf;
	bufCapacity = b->bufCapacity;
	b->buf = NULL;
	bufStart = b->bufStart;
	bufLen = b->bufLen;
	fileLen = b->fileLen;
	bufPos = b->bufPos;
	stream = b->stream;
	b->stream = NULL;
	isUserStream = b->isUserStream;
}

Buffer::Buffer(const unsigned char* buf, int len) {
	this->buf = new unsigned char[len];
	memcpy(this->buf, buf, len*sizeof(unsigned char));
	bufStart = 0;
	bufCapacity = bufLen = len;
	fileLen = len;
	bufPos = 0;
	stream = NULL;
}

Buffer::~Buffer() {
	Close(); 
	if (buf != NULL) {
		delete [] buf;
		buf = NULL;
	}
}

void Buffer::Close() {
	if (!isUserStream && stream != NULL) {
		fclose(stream);
		stream = NULL;
	}
}

int Buffer::Read() {
	if (bufPos < bufLen) {
		return buf[bufPos++];
	} else if (GetPos() < fileLen) {
		SetPos(GetPos()); // shift buffer start to Pos
		return buf[bufPos++];
	} else if ((stream != NULL) && !CanSeek() && (ReadNextStreamChunk() > 0)) {
		return buf[bufPos++];
	} else {
		return EoF;
	}
}

int Buffer::Peek() {
	int curPos = GetPos();
	int ch = Read();
	SetPos(curPos);
	return ch;
}

wchar_t* Buffer::GetString(int beg, int end) {
	int len = end - beg;
	wchar_t *buf = new wchar_t[len];
	int oldPos = GetPos();
	SetPos(beg);
	for (int i = 0; i < len; ++i) buf[i] = (wchar_t) Read();
	SetPos(oldPos);
	return buf;
}

int Buffer::GetPos() {
	return bufPos + bufStart;
}

void Buffer::SetPos(int value) {
	if ((value >= fileLen) && (stream != NULL) && !CanSeek()) {
		// Wanted position is after buffer and the stream
		// is not seek-able e.g. network or console,
		// thus we have to read the stream manually till
		// the wanted position is in sight.
		while ((value >= fileLen) && (ReadNextStreamChunk() > 0));
	}

	if ((value < 0) || (value > fileLen)) {
		char msg[50];
		sprintf(msg, "Buffer out of bounds access, position: %d", value);
		throw msg;
		
	}

	if ((value >= bufStart) && (value < (bufStart + bufLen))) { // already in buffer
		bufPos = value - bufStart;
	} else if (stream != NULL) { // must be swapped in
		fseek(stream, value, SEEK_SET);
		bufLen = fread(buf, sizeof(unsigned char), bufCapacity, stream);
		bufStart = value; bufPos = 0;
	} else {
		bufPos = fileLen - bufStart; // make Pos return fileLen
	}
}

// Read the next chunk of bytes from the stream, increases the buffer
// if needed and updates the fields fileLen and bufLen.
// Returns the number of bytes read.
int Buffer::ReadNextStreamChunk() {
	int free = bufCapacity - bufLen;
	if (free == 0) {
		// in the case of a growing input stream
		// we can neither seek in the stream, nor can we
		// foresee the maximum length, thus we must adapt
		// the buffer size on demand.
		bufCapacity = bufLen * 2;
		unsigned char *newBuf = new unsigned char[bufCapacity];
		memcpy(newBuf, buf, bufLen*sizeof(unsigned char));
		delete [] buf;
		buf = newBuf;
		free = bufLen;
	}
	int read = fread(buf + bufLen, sizeof(unsigned char), free, stream);
	if (read > 0) {
		fileLen = bufLen = (bufLen + read);
		return read;
	}
	// end of stream reached
	return 0;
}

bool Buffer::CanSeek() {
	return (stream != NULL) && (ftell(stream) != -1);
}

int UTF8Buffer::Read() {
	int ch;
	do {
		ch = Buffer::Read();
		// until we find a uft8 start (0xxxxxxx or 11xxxxxx)
	} while ((ch >= 128) && ((ch & 0xC0) != 0xC0) && (ch != EoF));
	if (ch < 128 || ch == EoF) {
		// nothing to do, first 127 chars are the same in ascii and utf8
		// 0xxxxxxx or end of file character
	} else if ((ch & 0xF0) == 0xF0) {
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x07; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F; ch = Buffer::Read();
		int c4 = ch & 0x3F;
		ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
	} else if ((ch & 0xE0) == 0xE0) {
		// 1110xxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x0F; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F;
		ch = (((c1 << 6) | c2) << 6) | c3;
	} else if ((ch & 0xC0) == 0xC0) {
		// 110xxxxx 10xxxxxx
		int c1 = ch & 0x1F; ch = Buffer::Read();
		int c2 = ch & 0x3F;
		ch = (c1 << 6) | c2;
	}
	return ch;
}

Scanner::Scanner(const unsigned char* buf, int len) {
	buffer = new Buffer(buf, len);
	Init();
}

Scanner::Scanner(const wchar_t* fileName) {
	FILE* stream;
	char *chFileName = coco_string_create_char(fileName);
	if ((stream = fopen(chFileName, "rb")) == NULL) {
		char msg[50];
		sprintf(msg, "Can not open file: %s", chFileName);
		coco_string_delete(chFileName);
		throw msg;
	}
	coco_string_delete(chFileName);
	buffer = new Buffer(stream, false);
	Init();
}

Scanner::Scanner(FILE* s) {
	buffer = new Buffer(s, true);
	Init();
}

Scanner::~Scanner() {
	char* cur = (char*) firstHeap;

	while(cur != NULL) {
		cur = *(char**) (cur + HEAP_BLOCK_SIZE);
		free(firstHeap);
		firstHeap = cur;
	}
	delete [] tval;
	delete buffer;
}

void Scanner::Init() {
	EOL    = '\n';
	eofSym = 0;
	maxT = 85;
	noSym = 85;
	int i;
	for (i = 33; i <= 33; ++i) start.set(i, 1);
	for (i = 36; i <= 38; ++i) start.set(i, 1);
	for (i = 40; i <= 42; ++i) start.set(i, 1);
	for (i = 47; i <= 47; ++i) start.set(i, 1);
	for (i = 58; i <= 85; ++i) start.set(i, 1);
	for (i = 87; i <= 90; ++i) start.set(i, 1);
	for (i = 94; i <= 122; ++i) start.set(i, 1);
	for (i = 124; i <= 124; ++i) start.set(i, 1);
	for (i = 126; i <= 126; ++i) start.set(i, 1);
	for (i = 128; i <= 65535; ++i) start.set(i, 1);
	for (i = 49; i <= 57; ++i) start.set(i, 35);
	start.set(48, 36);
	for (i = 43; i <= 43; ++i) start.set(i, 37);
	for (i = 45; i <= 45; ++i) start.set(i, 37);
	start.set(46, 51);
	start.set(34, 16);
	start.set(86, 38);
	start.set(35, 45);
	start.set(91, 46);
	start.set(93, 47);
	start.set(123, 48);
	start.set(125, 49);
	start.set(44, 50);
		start.set(Buffer::EoF, -1);
	keywords.set(L"X3D", 8);
	keywords.set(L"VRML", 9);
	keywords.set(L"utf8", 10);
	keywords.set(L"PROFILE", 11);
	keywords.set(L"COMPONENT", 12);
	keywords.set(L":", 13);
	keywords.set(L"EXPORT", 14);
	keywords.set(L"AS", 15);
	keywords.set(L"IMPORT", 16);
	keywords.set(L"META", 18);
	keywords.set(L"DEF", 19);
	keywords.set(L"USE", 20);
	keywords.set(L"PROTO", 21);
	keywords.set(L"inputOnly", 26);
	keywords.set(L"eventIn", 27);
	keywords.set(L"outputOnly", 28);
	keywords.set(L"eventOut", 29);
	keywords.set(L"initializeOnly", 30);
	keywords.set(L"field", 31);
	keywords.set(L"inputOutput", 32);
	keywords.set(L"exposedField", 33);
	keywords.set(L"EXTERNPROTO", 34);
	keywords.set(L"ROUTE", 35);
	keywords.set(L"TO", 36);
	keywords.set(L"Script", 38);
	keywords.set(L"IS", 39);
	keywords.set(L"MFBool", 40);
	keywords.set(L"MFColor", 41);
	keywords.set(L"MFColorRGBA", 42);
	keywords.set(L"MFDouble", 43);
	keywords.set(L"MFFloat", 44);
	keywords.set(L"MFImage", 45);
	keywords.set(L"MFInt32", 46);
	keywords.set(L"MFMatrix3d", 47);
	keywords.set(L"MFMatrix3f", 48);
	keywords.set(L"MFMatrix4d", 49);
	keywords.set(L"MFMatrix4f", 50);
	keywords.set(L"MFNode", 51);
	keywords.set(L"MFRotation", 52);
	keywords.set(L"MFString", 53);
	keywords.set(L"MFTime", 54);
	keywords.set(L"MFVec2d", 55);
	keywords.set(L"MFVec2f", 56);
	keywords.set(L"MFVec3d", 57);
	keywords.set(L"MFVec3f", 58);
	keywords.set(L"MFVec4d", 59);
	keywords.set(L"MFVec4f", 60);
	keywords.set(L"SFBool", 61);
	keywords.set(L"SFColor", 62);
	keywords.set(L"SFColorRGBA", 63);
	keywords.set(L"SFDouble", 64);
	keywords.set(L"SFFloat", 65);
	keywords.set(L"SFImage", 66);
	keywords.set(L"SFInt32", 67);
	keywords.set(L"SFMatrix3d", 68);
	keywords.set(L"SFMatrix3f", 69);
	keywords.set(L"SFMatrix4d", 70);
	keywords.set(L"SFMatrix4f", 71);
	keywords.set(L"SFNode", 72);
	keywords.set(L"SFRotation", 73);
	keywords.set(L"SFString", 74);
	keywords.set(L"SFTime", 75);
	keywords.set(L"SFVec2d", 76);
	keywords.set(L"SFVec2f", 77);
	keywords.set(L"SFVec3d", 78);
	keywords.set(L"SFVec3f", 79);
	keywords.set(L"SFVec4d", 80);
	keywords.set(L"SFVec4f", 81);
	keywords.set(L"TRUE", 82);
	keywords.set(L"FALSE", 83);
	keywords.set(L"SALVE", 84);


	tvalLength = 128;
	tval = new wchar_t[tvalLength]; // text of current token

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	heap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	firstHeap = heap;
	heapEnd = (void**) (((char*) heap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heapTop = heap;
	if (sizeof(Token) > HEAP_BLOCK_SIZE) {
		throw "Too small HEAP_BLOCK_SIZE";
	}

	pos = -1; line = 1; col = 0;
	oldEols = 0;
	NextCh();
	if (ch == 0xEF) { // check optional byte order mark for UTF-8
		NextCh(); int ch1 = ch;
		NextCh(); int ch2 = ch;
		if (ch1 != 0xBB || ch2 != 0xBF) {
			throw "Illegal byte order mark at start of file";
		}
		Buffer *oldBuf = buffer;
		buffer = new UTF8Buffer(buffer); col = 0;
		delete oldBuf; oldBuf = NULL;
		NextCh();
	}


	pt = tokens = CreateToken(); // first token is a dummy
}

void Scanner::NextCh() {
	if (oldEols > 0) { ch = EOL; oldEols--; }
	else {
		pos = buffer->GetPos();
		ch = buffer->Read(); col++;
		// replace isolated '\r' by '\n' in order to make
		// eol handling uniform across Windows, Unix and Mac
		if (ch == L'\r' && buffer->Peek() != L'\n') ch = EOL;
		if (ch == EOL) { line++; col = 0; }
	}

}

void Scanner::AddCh() {
	if (tlen >= tvalLength) {
		tvalLength *= 2;
		wchar_t *newBuf = new wchar_t[tvalLength];
		memcpy(newBuf, tval, tlen*sizeof(wchar_t));
		delete [] tval;
		tval = newBuf;
	}
		tval[tlen++] = ch;
	NextCh();
}


bool Scanner::Comment0() {
	int level = 1, line0 = line/*, pos0 = pos, col0 = col*/;
	NextCh();
	for(;;) {
		if (ch == 10) {
			level--;
			if (level == 0) { oldEols = line - line0; NextCh(); return true; }
			NextCh();
		} else if (ch == buffer->EoF) return false;
		else NextCh();
	}
}


void Scanner::CreateHeapBlock() {
	void* newHeap;
	char* cur = (char*) firstHeap;

	while(((char*) tokens < cur) || ((char*) tokens > (cur + HEAP_BLOCK_SIZE))) {
		cur = *((char**) (cur + HEAP_BLOCK_SIZE));
		free(firstHeap);
		firstHeap = cur;
	}

	// HEAP_BLOCK_SIZE byte heap + pointer to next heap block
	newHeap = malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	*heapEnd = newHeap;
	heapEnd = (void**) (((char*) newHeap) + HEAP_BLOCK_SIZE);
	*heapEnd = 0;
	heap = newHeap;
	heapTop = heap;
}

Token* Scanner::CreateToken() {
	Token *t;
	if (((char*) heapTop + (int) sizeof(Token)) >= (char*) heapEnd) {
		CreateHeapBlock();
	}
	t = (Token*) heapTop;
	heapTop = (void*) ((char*) heapTop + sizeof(Token));
	t->val = NULL;
	t->next = NULL;
	return t;
}

void Scanner::AppendVal(Token *t) {
	int reqMem = (tlen + 1) * sizeof(wchar_t);
	if (((char*) heapTop + reqMem) >= (char*) heapEnd) {
		if (reqMem > HEAP_BLOCK_SIZE) {
			throw "Too long token value";
		}
		CreateHeapBlock();
	}
	t->val = (wchar_t*) heapTop;
	heapTop = (void*) ((char*) heapTop + reqMem);

	wcsncpy(t->val, tval, tlen);
	t->val[tlen] = L'\0';
}

Token* Scanner::NextToken() {
	while (ch == ' ' ||
			(ch >= 9 && ch <= 10) || ch == 13
	) NextCh();
	if ((ch == L'#' && Comment0())) return NextToken();
	t = CreateToken();
	t->pos = pos; t->col = col; t->line = line;
	int state = start.state(ch);
	tlen = 0; AddCh();

	switch (state) {
		case -1: { t->kind = eofSym; break; } // NextCh already done
		case 0: { t->kind = noSym; break; }   // NextCh already done
		case 1:
			case_1:
			if (ch == L'!' || (ch >= L'$' && ch <= L'&') || (ch >= L'(' && ch <= L'+') || ch == L'-' || (ch >= L'/' && ch <= L'Z') || (ch >= L'^' && ch <= L'z') || ch == L'|' || ch == L'~' || (ch >= 128 && ch <= 65535)) {AddCh(); goto case_1;}
			else {t->kind = 1; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 2:
			case_2:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_3;}
			else {t->kind = noSym; break;}
		case 3:
			case_3:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_3;}
			else {t->kind = 2; break;}
		case 4:
			case_4:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_5;}
			else {t->kind = noSym; break;}
		case 5:
			case_5:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_5;}
			else if (ch == L'E' || ch == L'e') {AddCh(); goto case_6;}
			else {t->kind = 3; break;}
		case 6:
			case_6:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_8;}
			else if (ch == L'+' || ch == L'-') {AddCh(); goto case_7;}
			else {t->kind = noSym; break;}
		case 7:
			case_7:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_8;}
			else {t->kind = noSym; break;}
		case 8:
			case_8:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_8;}
			else {t->kind = 3; break;}
		case 9:
			case_9:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_9;}
			else if (ch == L'E' || ch == L'e') {AddCh(); goto case_10;}
			else {t->kind = 3; break;}
		case 10:
			case_10:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_12;}
			else if (ch == L'+' || ch == L'-') {AddCh(); goto case_11;}
			else {t->kind = noSym; break;}
		case 11:
			case_11:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_12;}
			else {t->kind = noSym; break;}
		case 12:
			case_12:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_12;}
			else {t->kind = 3; break;}
		case 13:
			case_13:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_15;}
			else if (ch == L'+' || ch == L'-') {AddCh(); goto case_14;}
			else {t->kind = noSym; break;}
		case 14:
			case_14:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_15;}
			else {t->kind = noSym; break;}
		case 15:
			case_15:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_15;}
			else {t->kind = 3; break;}
		case 16:
			case_16:
			if (ch <= L'!' || (ch >= L'#' && ch <= L'[') || (ch >= L']' && ch <= 65535)) {AddCh(); goto case_16;}
			else if (ch == L'"') {AddCh(); goto case_31;}
			else if (ch == 92) {AddCh(); goto case_39;}
			else {t->kind = noSym; break;}
		case 17:
			case_17:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_18;}
			else {t->kind = noSym; break;}
		case 18:
			case_18:
			if (ch <= L'!' || (ch >= L'#' && ch <= L'/') || (ch >= L':' && ch <= L'@') || (ch >= L'G' && ch <= L'[') || (ch >= L']' && ch <= L'`') || (ch >= L'g' && ch <= 65535)) {AddCh(); goto case_16;}
			else if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_40;}
			else if (ch == L'"') {AddCh(); goto case_31;}
			else if (ch == 92) {AddCh(); goto case_39;}
			else {t->kind = noSym; break;}
		case 19:
			case_19:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_20;}
			else {t->kind = noSym; break;}
		case 20:
			case_20:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_21;}
			else {t->kind = noSym; break;}
		case 21:
			case_21:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_22;}
			else {t->kind = noSym; break;}
		case 22:
			case_22:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_16;}
			else {t->kind = noSym; break;}
		case 23:
			case_23:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_24;}
			else {t->kind = noSym; break;}
		case 24:
			case_24:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_25;}
			else {t->kind = noSym; break;}
		case 25:
			case_25:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_26;}
			else {t->kind = noSym; break;}
		case 26:
			case_26:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_27;}
			else {t->kind = noSym; break;}
		case 27:
			case_27:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_28;}
			else {t->kind = noSym; break;}
		case 28:
			case_28:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_29;}
			else {t->kind = noSym; break;}
		case 29:
			case_29:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_30;}
			else {t->kind = noSym; break;}
		case 30:
			case_30:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_16;}
			else {t->kind = noSym; break;}
		case 31:
			case_31:
			{t->kind = 4; break;}
		case 32:
			case_32:
			{t->kind = 5; break;}
		case 33:
			case_33:
			if (ch == L'0') {AddCh(); goto case_34;}
			else {t->kind = noSym; break;}
		case 34:
			case_34:
			{t->kind = 6; break;}
		case 35:
			case_35:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_35;}
			else if (ch == L'.') {AddCh(); goto case_9;}
			else if (ch == L'E' || ch == L'e') {AddCh(); goto case_13;}
			else {t->kind = 2; break;}
		case 36:
			case_36:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_35;}
			else if (ch == L'X' || ch == L'x') {AddCh(); goto case_2;}
			else if (ch == L'.') {AddCh(); goto case_9;}
			else if (ch == L'E' || ch == L'e') {AddCh(); goto case_13;}
			else {t->kind = 2; break;}
		case 37:
			if ((ch >= L'1' && ch <= L'9')) {AddCh(); goto case_35;}
			else if (ch == L'0') {AddCh(); goto case_36;}
			else if (ch == L'.') {AddCh(); goto case_4;}
			else {t->kind = noSym; break;}
		case 38:
			if (ch == L'!' || (ch >= L'$' && ch <= L'&') || (ch >= L'(' && ch <= L'+') || ch == L'-' || (ch >= L'/' && ch <= L'1') || (ch >= L'4' && ch <= L'Z') || (ch >= L'^' && ch <= L'z') || ch == L'|' || ch == L'~' || (ch >= 128 && ch <= 65535)) {AddCh(); goto case_1;}
			else if (ch == L'3') {AddCh(); goto case_42;}
			else if (ch == L'2') {AddCh(); goto case_43;}
			else {t->kind = 1; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 39:
			case_39:
			if (ch == L'"' || ch == 39 || ch == L'0' || ch == 92 || (ch >= L'a' && ch <= L'b') || ch == L'f' || ch == L'n' || ch == L'r' || ch == L't' || ch == L'v') {AddCh(); goto case_16;}
			else if (ch == L'x') {AddCh(); goto case_17;}
			else if (ch == L'u') {AddCh(); goto case_19;}
			else if (ch == L'U') {AddCh(); goto case_23;}
			else {t->kind = noSym; break;}
		case 40:
			case_40:
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'F') || (ch >= L'a' && ch <= L'f')) {AddCh(); goto case_41;}
			else if (ch <= L'!' || (ch >= L'#' && ch <= L'/') || (ch >= L':' && ch <= L'@') || (ch >= L'G' && ch <= L'[') || (ch >= L']' && ch <= L'`') || (ch >= L'g' && ch <= 65535)) {AddCh(); goto case_16;}
			else if (ch == L'"') {AddCh(); goto case_31;}
			else if (ch == 92) {AddCh(); goto case_39;}
			else {t->kind = noSym; break;}
		case 41:
			case_41:
			if (ch <= L'!' || (ch >= L'#' && ch <= L'[') || (ch >= L']' && ch <= 65535)) {AddCh(); goto case_16;}
			else if (ch == L'"') {AddCh(); goto case_31;}
			else if (ch == 92) {AddCh(); goto case_39;}
			else {t->kind = noSym; break;}
		case 42:
			case_42:
			if (ch == L'!' || (ch >= L'$' && ch <= L'&') || (ch >= L'(' && ch <= L'+') || ch == L'-' || (ch >= L'/' && ch <= L'Z') || (ch >= L'^' && ch <= L'z') || ch == L'|' || ch == L'~' || (ch >= 128 && ch <= 65535)) {AddCh(); goto case_1;}
			else if (ch == L'.') {AddCh(); goto case_44;}
			else {t->kind = 1; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 43:
			case_43:
			if (ch == L'!' || (ch >= L'$' && ch <= L'&') || (ch >= L'(' && ch <= L'+') || ch == L'-' || (ch >= L'/' && ch <= L'Z') || (ch >= L'^' && ch <= L'z') || ch == L'|' || ch == L'~' || (ch >= 128 && ch <= 65535)) {AddCh(); goto case_1;}
			else if (ch == L'.') {AddCh(); goto case_33;}
			else {t->kind = 1; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 44:
			case_44:
			if ((ch >= L'0' && ch <= L'2')) {AddCh(); goto case_32;}
			else {t->kind = noSym; break;}
		case 45:
			{t->kind = 7; break;}
		case 46:
			{t->kind = 22; break;}
		case 47:
			{t->kind = 23; break;}
		case 48:
			{t->kind = 24; break;}
		case 49:
			{t->kind = 25; break;}
		case 50:
			{t->kind = 37; break;}
		case 51:
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_5;}
			else {t->kind = 17; break;}

	}
	AppendVal(t);
	return t;
}

// get the next token (possibly a token already seen during peeking)
Token* Scanner::Scan() {
	if (tokens->next == NULL) {
		return pt = tokens = NextToken();
	} else {
		pt = tokens = tokens->next;
		return tokens;
	}
}

// peek for the next token, ignore pragmas
Token* Scanner::Peek() {
	if (pt->next == NULL) {
		do {
			pt = pt->next = NextToken();
		} while (pt->kind > maxT); // skip pragmas
	} else {
		do {
			pt = pt->next;
		} while (pt->kind > maxT);
	}
	return pt;
}

// make sure that peeking starts at the current scan position
void Scanner::ResetPeek() {
	pt = tokens;
}

} // namespace


