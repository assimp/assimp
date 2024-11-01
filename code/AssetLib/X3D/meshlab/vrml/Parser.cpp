
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
 Revision 1.2  2008/02/21 13:04:09  gianpaolopalma
 Fixed compile error

 Revision 1.1  2008/02/20 22:02:00  gianpaolopalma
 First working version of Vrml to X3D translator

*****************************************************************************/

#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"


namespace VrmlTranslator {


void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::VrmlTranslator() {
		QDomElement root = doc->createElement("X3D");
		QDomElement scene = doc->createElement("Scene");
		root.appendChild(scene);
		InitX3dNode();
		if (la->kind == 7) {
			HeaderStatement();
		}
		if (la->kind == 11) {
			ProfileStatement();
		}
		ComponentStatements();
		MetaStatements();
		Statements(scene);
		doc->appendChild(root);
}

void Parser::HeaderStatement() {
		Expect(7);
		if (la->kind == 8) {
			Get();
			if (la->kind == 5) {
				Get();
			}
		} else if (la->kind == 9) {
			Get();
			if (la->kind == 6) {
				Get();
			}
		} else SynErr(86);
		Expect(10);
		if (la->kind == 4) {
			Get();
		}
}

void Parser::ProfileStatement() {
		Expect(11);
		ProfileNameId();
}

void Parser::ComponentStatements() {
		while (la->kind == 12) {
			ComponentStatement();
		}
}

void Parser::MetaStatements() {
		while (la->kind == 18) {
			MetaStatement();
		}
}

void Parser::Statements(QDomElement& parent) {
		while (StartOf(1)) {
			Statement(parent);
		}
}

void Parser::ProfileNameId() {
		Expect(1);
}

void Parser::ComponentStatement() {
		Expect(12);
		ComponentNameId();
		Expect(13);
		ComponentSupportLevel();
}

void Parser::ComponentNameId() {
		Expect(1);
}

void Parser::ComponentSupportLevel() {
		Expect(2);
}

void Parser::ExportStatement() {
		QString str;
		Expect(14);
		NodeNameId(str);
		Expect(15);
		ExportedNodeNameId();
}

void Parser::NodeNameId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::ExportedNodeNameId() {
		Expect(1);
}

void Parser::ImportStatement() {
		QString str;
		Expect(16);
		InlineNodeNameId();
		Expect(17);
		ExportedNodeNameId();
		Expect(15);
		NodeNameId(str);
}

void Parser::InlineNodeNameId() {
		Expect(1);
}

void Parser::MetaStatement() {
		Expect(18);
		Metakey();
		Metavalue();
}

void Parser::Metakey() {
		Expect(4);
}

void Parser::Metavalue() {
		Expect(4);
}

void Parser::Statement(QDomElement& parent) {
		if (StartOf(2)) {
			NodeStatement(parent);
		} else if (la->kind == 16) {
			ImportStatement();
		} else if (la->kind == 14) {
			ExportStatement();
		} else if (la->kind == 21 || la->kind == 34) {
			ProtoStatement(parent);
		} else if (la->kind == 35) {
			RouteStatement();
		} else SynErr(87);
}

void Parser::NodeStatement(QDomElement& parent) {
		QString tagName, attrValue;
		if (la->kind == 1 || la->kind == 38) {
			Node(parent, tagName, "");
		} else if (la->kind == 19) {
			Get();
			NodeNameId(attrValue);
			Node(parent, tagName, attrValue);
		} else if (la->kind == 20) {
			Get();
			NodeNameId(attrValue);
			std::map<QString, QString>::const_iterator iter = defNode.find(attrValue);
			if(iter != defNode.end())
			{
			  QDomElement node = doc->createElement(iter->second);
			  node.setAttribute("USE", attrValue);
			  parent.appendChild(node);
			}
		} else SynErr(88);
}

void Parser::ProtoStatement(QDomElement& parent) {
		if (la->kind == 21) {
			Proto(parent);
		} else if (la->kind == 34) {
			Externproto(parent);
		} else SynErr(89);
}

void Parser::RouteStatement() {
		QString str;
		Expect(35);
		NodeNameId(str);
		Expect(17);
		OutputOnlyId(str);
		Expect(36);
		NodeNameId(str);
		Expect(17);
		InputOnlyId(str);
}

void Parser::Node(QDomElement& parent, QString& tagName, const QString defValue) {
		bool flag = false; QDomElement node;
		if (la->kind == 1) {
			NodeTypeId(tagName);
			std::set<QString>::const_iterator iter = proto.find(tagName);
			if (iter != proto.end())
			{
			  node = doc->createElement("ProtoInstance");
			  node.setAttribute("name", tagName);
			  flag = true;
			}
			else
			  node = doc->createElement(tagName);
			if (defValue != "")
			{
			  node.setAttribute("DEF", defValue);
			  defNode[defValue] = tagName;
			}
			Expect(24);
			NodeBody(node, flag);
			Expect(25);
		} else if (la->kind == 38) {
			Get();
			Expect(24);
			ScriptBody();
			Expect(25);
			node = doc->createElement("Script");
		} else SynErr(90);
		parent.appendChild(node);
}

void Parser::RootNodeStatement(QDomElement& parent) {
		QString tagName, attrValue;
		if (la->kind == 1 || la->kind == 38) {
			Node(parent, tagName, "");
		} else if (la->kind == 19) {
			Get();
			NodeNameId(attrValue);
			Node(parent, tagName, attrValue);
		} else SynErr(91);
}

void Parser::Proto(QDomElement& parent) {
		QString name; QDomElement node;
		Expect(21);
		NodeTypeId(name);
		node = doc->createElement("ProtoDeclare");
		node.setAttribute("name", name);
		proto.insert(name);
		Expect(22);
		QDomElement interf = doc->createElement("ProtoInterface");
		InterfaceDeclarations(interf);
		node.appendChild(interf);
		Expect(23);
		Expect(24);
		QDomElement body = doc->createElement("ProtoBody");
		ProtoBody(body);
		node.appendChild(body);
		Expect(25);
		parent.appendChild(node);
}

void Parser::Externproto(QDomElement& parent) {
		QString name, url;
		QDomElement node = doc->createElement("ExternProtoDeclare");
		Expect(34);
		NodeTypeId(name);
		Expect(22);
		ExternInterfaceDeclarations(node);
		Expect(23);
		URLList(url);
		std::set<QString>::const_iterator iter = x3dNode.find(name);
		if (iter == x3dNode.end())
		{
		  node.setAttribute("name", name);
		  node.setAttribute("url", url);
		  parent.appendChild(node);
		  proto.insert(name);
		}
}

void Parser::ProtoStatements(QDomElement& parent) {
		while (la->kind == 21 || la->kind == 34) {
			ProtoStatement(parent);
		}
}

void Parser::NodeTypeId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::InterfaceDeclarations(QDomElement& parent) {
		while (StartOf(3)) {
			InterfaceDeclaration(parent);
		}
}

void Parser::ProtoBody(QDomElement& parent) {
		ProtoStatements(parent);
		RootNodeStatement(parent);
		Statements(parent);
}

void Parser::InterfaceDeclaration(QDomElement& parent) {
		QString name, type, val; QDomElement node;
		if (StartOf(4)) {
			RestrictedInterfaceDeclaration(parent);
		} else if (la->kind == 32 || la->kind == 33) {
			if (la->kind == 32) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			FieldId(name);
			FieldValue(node, "value", false);
			node = doc->createElement("field");
			node.setAttribute("name", name);
			node.setAttribute("type", type);
			node.setAttribute("accessType", "inputOutput");
			parent.appendChild(node);
		} else SynErr(92);
}

void Parser::RestrictedInterfaceDeclaration(QDomElement& parent) {
		QString name; QString type; QString val;
		QDomElement node = doc->createElement("field");
		if (la->kind == 26 || la->kind == 27) {
			if (la->kind == 26) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			InputOnlyId(name);
			node.setAttribute("accessType", "inputOnly");
		} else if (la->kind == 28 || la->kind == 29) {
			if (la->kind == 28) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			OutputOnlyId(name);
			node.setAttribute("accessType", "outputOnly");
		} else if (la->kind == 30 || la->kind == 31) {
			if (la->kind == 30) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			InitializeOnlyId(name);
			FieldValue(node, "value", false);
			node.setAttribute("accessType", "initializeOnly");
		} else SynErr(93);
		node.setAttribute("name", name);
		node.setAttribute("type", type);
		parent.appendChild(node);
}

void Parser::FieldType(QString& str) {
		switch (la->kind) {
		case 40: {
			Get();
			break;
		}
		case 41: {
			Get();
			break;
		}
		case 42: {
			Get();
			break;
		}
		case 43: {
			Get();
			break;
		}
		case 44: {
			Get();
			break;
		}
		case 45: {
			Get();
			break;
		}
		case 46: {
			Get();
			break;
		}
		case 47: {
			Get();
			break;
		}
		case 48: {
			Get();
			break;
		}
		case 49: {
			Get();
			break;
		}
		case 50: {
			Get();
			break;
		}
		case 51: {
			Get();
			break;
		}
		case 52: {
			Get();
			break;
		}
		case 53: {
			Get();
			break;
		}
		case 54: {
			Get();
			break;
		}
		case 55: {
			Get();
			break;
		}
		case 56: {
			Get();
			break;
		}
		case 57: {
			Get();
			break;
		}
		case 58: {
			Get();
			break;
		}
		case 59: {
			Get();
			break;
		}
		case 60: {
			Get();
			break;
		}
		case 61: {
			Get();
			break;
		}
		case 62: {
			Get();
			break;
		}
		case 63: {
			Get();
			break;
		}
		case 64: {
			Get();
			break;
		}
		case 65: {
			Get();
			break;
		}
		case 66: {
			Get();
			break;
		}
		case 67: {
			Get();
			break;
		}
		case 68: {
			Get();
			break;
		}
		case 69: {
			Get();
			break;
		}
		case 70: {
			Get();
			break;
		}
		case 71: {
			Get();
			break;
		}
		case 72: {
			Get();
			break;
		}
		case 73: {
			Get();
			break;
		}
		case 74: {
			Get();
			break;
		}
		case 75: {
			Get();
			break;
		}
		case 76: {
			Get();
			break;
		}
		case 77: {
			Get();
			break;
		}
		case 78: {
			Get();
			break;
		}
		case 79: {
			Get();
			break;
		}
		case 80: {
			Get();
			break;
		}
		case 81: {
			Get();
			break;
		}
		default: SynErr(94); break;
		}
		str = QString(coco_string_create_char(t->val));
}

void Parser::InputOnlyId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::OutputOnlyId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::InitializeOnlyId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::FieldValue(QDomElement& parent, QString fieldName, bool flag) {
		if (StartOf(5)) {
			SingleValue(parent, fieldName, flag);
		} else if (la->kind == 22) {
			MultiValue(parent, fieldName, flag);
		} else SynErr(95);
}

void Parser::FieldId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::ExternInterfaceDeclarations(QDomElement& parent) {
		while (StartOf(3)) {
			ExternInterfaceDeclaration(parent);
		}
}

void Parser::URLList(QString& url) {
		if (la->kind == 4) {
			Get();
			url = QString(coco_string_create_char(t->val));
		} else if (la->kind == 22) {
			Get();
			while (la->kind == 4) {
				Get();
				url.append(coco_string_create_char(t->val)); url.append(" ");
				if (la->kind == 37) {
					Get();
				}
			}
			Expect(23);
		} else SynErr(96);
}

void Parser::ExternInterfaceDeclaration(QDomElement& parent) {
		QString type, name;
		QDomElement node = doc->createElement("field");
		if (la->kind == 26 || la->kind == 27) {
			if (la->kind == 26) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			InputOnlyId(name);
			node.setAttribute("accessType", "inputOnly");
		} else if (la->kind == 28 || la->kind == 29) {
			if (la->kind == 28) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			OutputOnlyId(name);
			node.setAttribute("accessType", "outputOnly");
		} else if (la->kind == 30 || la->kind == 31) {
			if (la->kind == 30) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			InitializeOnlyId(name);
			node.setAttribute("accessType", "initializeOnly");
		} else if (la->kind == 32 || la->kind == 33) {
			if (la->kind == 32) {
				Get();
			} else {
				Get();
			}
			FieldType(type);
			FieldId(name);
			node.setAttribute("accessType", "inputOutput");
		} else SynErr(97);
		node.setAttribute("name" , name);
		node.setAttribute("type", type);
		parent.appendChild(node);
}

void Parser::NodeBody(QDomElement& parent, bool flag) {
		while (StartOf(6)) {
			NodeBodyElement(parent, flag);
		}
}

void Parser::ScriptBody() {
		while (StartOf(7)) {
			ScriptBodyElement();
		}
}

void Parser::NodeBodyElement(QDomElement& parent, bool flag) {
		QString idName, idProto; QDomElement node;
		if (la->kind == 1) {
			Get();
			idName = QString(coco_string_create_char(t->val));
			if (StartOf(8)) {
				FieldValue(parent, idName, flag);
			} else if (la->kind == 39) {
				Get();
				Expect(1);
				idProto = QString(coco_string_create_char(t->val));
				node = doc->createElement("IS");
				QDomElement connect = doc->createElement("connect");
				connect.setAttribute("nodeField", idName);
				connect.setAttribute("protoField", idProto);
				node.appendChild(connect);
				parent.appendChild(node);
				
			} else SynErr(98);
		} else if (la->kind == 35) {
			RouteStatement();
		} else if (la->kind == 21 || la->kind == 34) {
			ProtoStatement(parent);
		} else SynErr(99);
}

void Parser::ScriptBodyElement() {
		QString str; QDomElement elem;
		if (StartOf(6)) {
			NodeBodyElement(elem, false);
		} else if (la->kind == 26 || la->kind == 27) {
			if (la->kind == 26) {
				Get();
			} else {
				Get();
			}
			FieldType(str);
			InputOnlyId(str);
			if (la->kind == 39) {
				Get();
				InputOnlyId(str);
			}
		} else if (la->kind == 28 || la->kind == 29) {
			if (la->kind == 28) {
				Get();
			} else {
				Get();
			}
			FieldType(str);
			OutputOnlyId(str);
			if (la->kind == 39) {
				Get();
				OutputOnlyId(str);
			}
		} else if (la->kind == 30 || la->kind == 31) {
			if (la->kind == 30) {
				Get();
			} else {
				Get();
			}
			FieldType(str);
			InitializeOnlyId(str);
			if (StartOf(8)) {
				FieldValue(elem, "", false);
			} else if (la->kind == 39) {
				Get();
				InitializeOnlyId(str);
			} else SynErr(100);
		} else if (la->kind == 32 || la->kind == 33) {
			if (la->kind == 32) {
				Get();
			} else {
				Get();
			}
			FieldType(str);
			InputOutputId(str);
			Expect(39);
			InputOutputId(str);
		} else SynErr(101);
}

void Parser::InputOutputId(QString& str) {
		Expect(1);
		str = QString(coco_string_create_char(t->val));
}

void Parser::SingleValue(QDomElement& parent, QString fieldName, bool flag) {
		QString value; QDomElement tmpParent = doc->createElement("tmp");
		if (StartOf(9)) {
			if (la->kind == 4) {
				Get();
				value.append(coco_string_create_char(t->val)); value.remove("\"");
			} else if (la->kind == 2 || la->kind == 3) {
				if (la->kind == 2) {
					Get();
				} else {
					Get();
				}
				value.append(coco_string_create_char(t->val));
				if (la->kind == 37) {
					Get();
				}
				while (la->kind == 2 || la->kind == 3) {
					if (la->kind == 2) {
						Get();
					} else {
						Get();
					}
					value.append(" "); value.append(coco_string_create_char(t->val));
					if (la->kind == 37) {
						Get();
					}
				}
			} else if (la->kind == 82) {
				Get();
				value = "true";
			} else {
				Get();
				value = "false";
			}
			if (flag)
			{
			  QDomElement node = doc->createElement("fieldValue");
			  node.setAttribute("name", fieldName);
			  node.setAttribute("value", value);
			  parent.appendChild(node);
			}
			else
			  parent.setAttribute(fieldName, value);
		} else if (StartOf(2)) {
			NodeStatement(tmpParent);
			if (flag)
			{
			  QDomElement tmp = doc->createElement("fieldValue");
			  tmp.setAttribute("name", fieldName);
			  tmp.appendChild(tmpParent.firstChildElement());
			  parent.appendChild(tmp);
			}
			else
			  parent.appendChild(tmpParent.firstChildElement());
		} else SynErr(102);
}

void Parser::MultiValue(QDomElement& parent, QString fieldName, bool flag) {
		QString value; QDomElement tmpParent = doc->createElement("tmp");
		Expect(22);
		if (StartOf(10)) {
			if (la->kind == 2 || la->kind == 3) {
				MultiNumber(value);
			} else if (la->kind == 4) {
				MultiString(value);
			} else {
				MultiBool(value);
			}
			if (flag)
			{
			  QDomElement tmp = doc->createElement("fieldValue");
			  tmp.setAttribute("name", fieldName);
			  tmp.setAttribute("value", value);
			  parent.appendChild(tmp);
			}
			else
			  parent.setAttribute(fieldName, value);
			
		} else if (StartOf(11)) {
			while (StartOf(2)) {
				NodeStatement(tmpParent);
				if (la->kind == 37) {
					Get();
				}
			}
			QDomElement child;
			QDomNodeList list = tmpParent.childNodes();
			QDomElement field = doc->createElement("field");
			field.setAttribute("name", fieldName);
			int i = 0;
			while(i < list.size())
			{
			  child = list.at(i).toElement();
			  if (flag)
			    field.appendChild(child.cloneNode());
			  else
			    parent.appendChild(child.cloneNode());
			  i++;
			}
			if (flag)
			  parent.appendChild(field);
			
		} else SynErr(103);
		Expect(23);
}

void Parser::MultiNumber(QString& value) {
		if (la->kind == 2) {
			Get();
		} else if (la->kind == 3) {
			Get();
		} else SynErr(104);
		value.append(coco_string_create_char(t->val));
		if (la->kind == 37) {
			Get();
		}
		while (la->kind == 2 || la->kind == 3) {
			if (la->kind == 2) {
				Get();
			} else {
				Get();
			}
			value.append(" "); value.append(coco_string_create_char(t->val));
			if (la->kind == 37) {
				Get();
			}
		}
}

void Parser::MultiString(QString& value) {
		Expect(4);
		value.append(coco_string_create_char(t->val));
		if (la->kind == 37) {
			Get();
		}
		while (la->kind == 4) {
			Get();
			value.append(" "); value.append(coco_string_create_char(t->val));
			if (la->kind == 37) {
				Get();
			}
		}
}

void Parser::MultiBool(QString& value) {
		if (la->kind == 82) {
			Get();
		} else if (la->kind == 84) {
			Get();
		} else SynErr(105);
		value.append(coco_string_create_char(t->val));
		if (la->kind == 37) {
			Get();
		}
		while (la->kind == 82 || la->kind == 83) {
			if (la->kind == 82) {
				Get();
			} else {
				Get();
			}
			value.append(" "); value.append(coco_string_create_char(t->val));
			if (la->kind == 37) {
				Get();
			}
		}
}



void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	VrmlTranslator();

	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 85;

	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[12][87] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,x,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,T,T, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x},
		{x,x,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x},
		{x,x,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
	stringError = 0;
}

Errors::~Errors() {
	if(stringError) delete stringError;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"id expected"); break;
			case 2: s = coco_string_create(L"intCont expected"); break;
			case 3: s = coco_string_create(L"realCont expected"); break;
			case 4: s = coco_string_create(L"string expected"); break;
			case 5: s = coco_string_create(L"x3dVersion expected"); break;
			case 6: s = coco_string_create(L"vrmlVersion expected"); break;
			case 7: s = coco_string_create(L"\"#\" expected"); break;
			case 8: s = coco_string_create(L"\"X3D\" expected"); break;
			case 9: s = coco_string_create(L"\"VRML\" expected"); break;
			case 10: s = coco_string_create(L"\"utf8\" expected"); break;
			case 11: s = coco_string_create(L"\"PROFILE\" expected"); break;
			case 12: s = coco_string_create(L"\"COMPONENT\" expected"); break;
			case 13: s = coco_string_create(L"\":\" expected"); break;
			case 14: s = coco_string_create(L"\"EXPORT\" expected"); break;
			case 15: s = coco_string_create(L"\"AS\" expected"); break;
			case 16: s = coco_string_create(L"\"IMPORT\" expected"); break;
			case 17: s = coco_string_create(L"\".\" expected"); break;
			case 18: s = coco_string_create(L"\"META\" expected"); break;
			case 19: s = coco_string_create(L"\"DEF\" expected"); break;
			case 20: s = coco_string_create(L"\"USE\" expected"); break;
			case 21: s = coco_string_create(L"\"PROTO\" expected"); break;
			case 22: s = coco_string_create(L"\"[\" expected"); break;
			case 23: s = coco_string_create(L"\"]\" expected"); break;
			case 24: s = coco_string_create(L"\"{\" expected"); break;
			case 25: s = coco_string_create(L"\"}\" expected"); break;
			case 26: s = coco_string_create(L"\"inputOnly\" expected"); break;
			case 27: s = coco_string_create(L"\"eventIn\" expected"); break;
			case 28: s = coco_string_create(L"\"outputOnly\" expected"); break;
			case 29: s = coco_string_create(L"\"eventOut\" expected"); break;
			case 30: s = coco_string_create(L"\"initializeOnly\" expected"); break;
			case 31: s = coco_string_create(L"\"field\" expected"); break;
			case 32: s = coco_string_create(L"\"inputOutput\" expected"); break;
			case 33: s = coco_string_create(L"\"exposedField\" expected"); break;
			case 34: s = coco_string_create(L"\"EXTERNPROTO\" expected"); break;
			case 35: s = coco_string_create(L"\"ROUTE\" expected"); break;
			case 36: s = coco_string_create(L"\"TO\" expected"); break;
			case 37: s = coco_string_create(L"\",\" expected"); break;
			case 38: s = coco_string_create(L"\"Script\" expected"); break;
			case 39: s = coco_string_create(L"\"IS\" expected"); break;
			case 40: s = coco_string_create(L"\"MFBool\" expected"); break;
			case 41: s = coco_string_create(L"\"MFColor\" expected"); break;
			case 42: s = coco_string_create(L"\"MFColorRGBA\" expected"); break;
			case 43: s = coco_string_create(L"\"MFDouble\" expected"); break;
			case 44: s = coco_string_create(L"\"MFFloat\" expected"); break;
			case 45: s = coco_string_create(L"\"MFImage\" expected"); break;
			case 46: s = coco_string_create(L"\"MFInt32\" expected"); break;
			case 47: s = coco_string_create(L"\"MFMatrix3d\" expected"); break;
			case 48: s = coco_string_create(L"\"MFMatrix3f\" expected"); break;
			case 49: s = coco_string_create(L"\"MFMatrix4d\" expected"); break;
			case 50: s = coco_string_create(L"\"MFMatrix4f\" expected"); break;
			case 51: s = coco_string_create(L"\"MFNode\" expected"); break;
			case 52: s = coco_string_create(L"\"MFRotation\" expected"); break;
			case 53: s = coco_string_create(L"\"MFString\" expected"); break;
			case 54: s = coco_string_create(L"\"MFTime\" expected"); break;
			case 55: s = coco_string_create(L"\"MFVec2d\" expected"); break;
			case 56: s = coco_string_create(L"\"MFVec2f\" expected"); break;
			case 57: s = coco_string_create(L"\"MFVec3d\" expected"); break;
			case 58: s = coco_string_create(L"\"MFVec3f\" expected"); break;
			case 59: s = coco_string_create(L"\"MFVec4d\" expected"); break;
			case 60: s = coco_string_create(L"\"MFVec4f\" expected"); break;
			case 61: s = coco_string_create(L"\"SFBool\" expected"); break;
			case 62: s = coco_string_create(L"\"SFColor\" expected"); break;
			case 63: s = coco_string_create(L"\"SFColorRGBA\" expected"); break;
			case 64: s = coco_string_create(L"\"SFDouble\" expected"); break;
			case 65: s = coco_string_create(L"\"SFFloat\" expected"); break;
			case 66: s = coco_string_create(L"\"SFImage\" expected"); break;
			case 67: s = coco_string_create(L"\"SFInt32\" expected"); break;
			case 68: s = coco_string_create(L"\"SFMatrix3d\" expected"); break;
			case 69: s = coco_string_create(L"\"SFMatrix3f\" expected"); break;
			case 70: s = coco_string_create(L"\"SFMatrix4d\" expected"); break;
			case 71: s = coco_string_create(L"\"SFMatrix4f\" expected"); break;
			case 72: s = coco_string_create(L"\"SFNode\" expected"); break;
			case 73: s = coco_string_create(L"\"SFRotation\" expected"); break;
			case 74: s = coco_string_create(L"\"SFString\" expected"); break;
			case 75: s = coco_string_create(L"\"SFTime\" expected"); break;
			case 76: s = coco_string_create(L"\"SFVec2d\" expected"); break;
			case 77: s = coco_string_create(L"\"SFVec2f\" expected"); break;
			case 78: s = coco_string_create(L"\"SFVec3d\" expected"); break;
			case 79: s = coco_string_create(L"\"SFVec3f\" expected"); break;
			case 80: s = coco_string_create(L"\"SFVec4d\" expected"); break;
			case 81: s = coco_string_create(L"\"SFVec4f\" expected"); break;
			case 82: s = coco_string_create(L"\"TRUE\" expected"); break;
			case 83: s = coco_string_create(L"\"FALSE\" expected"); break;
			case 84: s = coco_string_create(L"\"SALVE\" expected"); break;
			case 85: s = coco_string_create(L"??? expected"); break;
			case 86: s = coco_string_create(L"invalid HeaderStatement"); break;
			case 87: s = coco_string_create(L"invalid Statement"); break;
			case 88: s = coco_string_create(L"invalid NodeStatement"); break;
			case 89: s = coco_string_create(L"invalid ProtoStatement"); break;
			case 90: s = coco_string_create(L"invalid Node"); break;
			case 91: s = coco_string_create(L"invalid RootNodeStatement"); break;
			case 92: s = coco_string_create(L"invalid InterfaceDeclaration"); break;
			case 93: s = coco_string_create(L"invalid RestrictedInterfaceDeclaration"); break;
			case 94: s = coco_string_create(L"invalid FieldType"); break;
			case 95: s = coco_string_create(L"invalid FieldValue"); break;
			case 96: s = coco_string_create(L"invalid URLList"); break;
			case 97: s = coco_string_create(L"invalid ExternInterfaceDeclaration"); break;
			case 98: s = coco_string_create(L"invalid NodeBodyElement"); break;
			case 99: s = coco_string_create(L"invalid NodeBodyElement"); break;
			case 100: s = coco_string_create(L"invalid ScriptBodyElement"); break;
			case 101: s = coco_string_create(L"invalid ScriptBodyElement"); break;
			case 102: s = coco_string_create(L"invalid SingleValue"); break;
			case 103: s = coco_string_create(L"invalid MultiValue"); break;
			case 104: s = coco_string_create(L"invalid MultiNumber"); break;
			case 105: s = coco_string_create(L"invalid MultiBool"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	wchar_t str[100];
	coco_swprintf(str, 100, L"-- line %d col %d: %ls\n", line, col, s);
	stringError = coco_string_create_append(stringError, str);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wchar_t str[100];
	coco_swprintf(str, 100, L"-- line %d col %d: %ls\n", line, col, s);
	stringError = coco_string_create_append(stringError, str);
	count++;
}

/*
void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(L"%ls\n", s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(L"%ls", s); 
	exit(1);
}
*/
} // namespace


