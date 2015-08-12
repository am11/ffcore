#include "pch.h"
#include "Dict/JsonPersist.h"
#include "Dict/JsonTokenizer.h"
#include "Dict/Value.h"
#include "String/StringUtil.h"

bool JsonTokenizerTest()
{
	ff::String json(
		L"{\n"
		L"  // Test comment\n"
		L"  /* Another test comment */\n"
		L"  'foo': 'bar',\n"
		L"  'obj' : { 'nested': {}, 'nested2': [] },\n"
		L"  'numbers' : [ -1, 0, 8.5, -98.76e54, 1E-8 ],\n"
		L"  'identifiers' : [ true, false, null ],\n"
		L"  'string' : [ 'Hello', 'a\\'\\r\\u0020z' ],\n"
		L"}\n");
	ff::ReplaceAll(json, '\'', '\"');
	ff::JsonTokenizer tokenizer(json);

	for (ff::JsonToken token = tokenizer.NextToken();
		token._type != ff::JsonTokenType::None;
		token = tokenizer.NextToken())
	{
		assertRetVal(token._type != ff::JsonTokenType::Error, false);

		ff::ValuePtr value;

		switch (token._type)
		{
		case ff::JsonTokenType::Number:
			assertRetVal(token.GetValue(&value), false);
			assertRetVal(value->IsType(ff::Value::Type::Double) || value->IsType(ff::Value::Type::Int), false);
			break;

		case ff::JsonTokenType::String:
			assertRetVal(token.GetValue(&value), false);
			assertRetVal(value->IsType(ff::Value::Type::String), false);
			break;

		case ff::JsonTokenType::Null:
			assertRetVal(token.GetValue(&value), false);
			assertRetVal(value->IsType(ff::Value::Type::Null), false);
			break;

		case ff::JsonTokenType::True:
			assertRetVal(token.GetValue(&value), false);
			assertRetVal(value->IsType(ff::Value::Type::Bool) && value->AsBool(), false);
			break;

		case ff::JsonTokenType::False:
			assertRetVal(token.GetValue(&value), false);
			assertRetVal(value->IsType(ff::Value::Type::Bool) && !value->AsBool(), false);
			break;
		}
	}

	return true;
}

bool JsonParserTest()
{
	ff::String json(
		L"{\n"
		L"  // Test comment\n"
		L"  /* Another test comment */\n"
		L"  'foo': 'bar',\n"
		L"  'obj' : { 'nested': {}, 'nested2': [] },\n"
		L"  'numbers' : [ -1, 0, 8.5, -98.76e54, 1E-8 ],\n"
		L"  'identifiers' : [ true, false, null ],\n"
		L"  'string' : [ 'Hello', 'a\\'\\r\\u0020z' ],\n"
		L"}\n");
	ff::ReplaceAll(json, '\'', '\"');

	size_t errorPos = 0;
	ff::Dict dict = ff::JsonParse(json, &errorPos);

	assertRetVal(dict.Size(true) == 5, false);
	ff::Vector<ff::String> names = dict.GetAllNames(true, true, false);
	assertRetVal(names[0] == L"foo", false);
	assertRetVal(names[1] == L"identifiers", false);
	assertRetVal(names[2] == L"numbers", false);
	assertRetVal(names[3] == L"obj", false);
	assertRetVal(names[4] == L"string", false);

	assertRetVal(dict.GetValue(ff::String(L"foo"), true)->IsType(ff::Value::Type::String), false);
	assertRetVal(dict.GetValue(ff::String(L"foo"), true)->AsString() == L"bar", false);

	return true;
}

bool JsonPrintTest()
{
	ff::String json(
		L"{\n"
		L"  // Test comment\n"
		L"  /* Another test comment */\n"
		L"  'foo': 'bar',\n"
		L"  'obj' : { 'nested': {}, 'nested2': [] },\n"
		L"  'numbers' : [ -1, 0, 8.5, -98.76e54, 1E-8 ],\n"
		L"  'identifiers' : [ true, false, null ],\n"
		L"  'string' : [ 'Hello', 'a\\'\\r\\u0020z' ],\n"
		L"}\n");
	ff::ReplaceAll(json, '\'', '\"');

	ff::Dict dict = ff::JsonParse(json);

	ff::String actual = ff::JsonWrite(dict);
	ff::String expect(
		L"{\r\n"
		L"  'foo': 'bar',\r\n"
		L"  'identifiers':\r\n"
		L"  [\r\n"
		L"     true,\r\n"
		L"     false,\r\n"
		L"     null\r\n"
		L"  ],\r\n"
		L"  'numbers':\r\n"
		L"  [\r\n"
		L"     -1,\r\n"
		L"     0,\r\n"
		L"     8.5,\r\n"
		L"     -9.876e+55,\r\n"
		L"     1e-08\r\n"
		L"  ],\r\n"
		L"  'obj':\r\n"
		L"  {\r\n"
		L"    'nested': {},\r\n"
		L"    'nested2': []\r\n"
		L"  },\r\n"
		L"  'string':\r\n"
		L"  [\r\n"
		L"     'Hello',\r\n"
		L"     'a\\'\\r z'\r\n"
		L"  ]\r\n"
		L"}");
	ff::ReplaceAll(expect, '\'', '\"');

	assertRetVal(actual == expect, false);

	return true;
}
