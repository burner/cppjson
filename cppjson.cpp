#include <string>
#include <assert.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm> 
#include <functional> 
#include <stdexcept> 

#include "cppjson.hpp"
#include "unit.hpp"

static bool isString(const std::string& toTest) {
	return toTest.size() >= 1;
}

UNITTEST(isStringTest) {
	AS_T(isString("h"));
	AS_T(isString("hello"));
	AS_F(isString(""));
}

static bool convertsToFloat(const std::string& s, double& ret) {
	for(auto it : s) {
		if((it < '0' || it > '9') && it != '.' && it != 'E' && it != 'e' && it != '-' && it != '+') {
			return false;
		}
	}
	sscanf(s.c_str(), "%lf", &ret);
	return true;
}

UNITTEST(convToFloat1) {
	double d;
	AS_T(convertsToFloat("123.3", d));
	AS_EQ(d, 123.3);
}

static bool convertsToInt(const std::string& s, long& ret) {
	for(auto it : s) {
		if((it < '0' || it > '9') && it != '-' && it != '+') {
			return false;
		}
	}
	sscanf(s.c_str(), "%ld", &ret);
	return true;
}

UNITTEST(convToInt1) {
	long d;
	AS_F(convertsToInt("123.3", d));
	AS_NEQ(d, 123);

	AS_T(convertsToInt("123", d));
	AS_EQ(d, 123);
}

using namespace cppjson;

std::string lexString(size_t& it, const std::string& str) {
	it++; // eat the "
	auto be = it;
	it++;
	for(; it < str.size(); it++) {
		if(str[it] == '"' && it > 0 && str[it-1] != '\\') {
			return str.substr(be, it-be);
		} else if(str[it] == '"' && it == 0) {
			return str.substr(be, it-be+1);
		}
	}
	return str.substr(be);
}

void tokenize(const std::string& str, std::vector<std::string>& tokens) {
	size_t it = 0;
	for(; it < str.size(); ++it) {
		if(str[it] == ' ' || str[it] == '\t') {
			continue;
		} else if(str[it] == '}') {
			tokens.push_back(std::string("}"));
		} else if(str[it] == ']') {
			tokens.push_back(std::string("]"));
		} else if(str[it] == '[') {
			tokens.push_back(std::string("["));
		} else if(str[it] == ':') {
			tokens.push_back(std::string(":"));
		} else if(str[it] == '{') {
			tokens.push_back(std::string("{"));
		} else if(str[it] == ',') {
			tokens.push_back(std::string(","));
		} else if(str[it] == '"') {
			tokens.push_back(lexString(it, str));
		}
	}
}

std::vector<std::string> getJsonTokenFromFile(const std::string& filename) {
	std::vector<std::string> strs;
	std::string line;
	std::ifstream infile(filename);
	while(std::getline(infile, line)) {
		//std::cout<<line<<std::endl;
		tokenize(line, strs);
		for(auto it : strs) {
			if(it.size() == 0) {
				std::cout<<"problem"<<std::endl;
			}
		}
	}
	return strs;
}

jsonparser cppjson::parseFile(const std::string& filename) {
	auto tokens(getJsonTokenFromFile(filename));
	jsonparser parser(tokens);
	parser.parse();
	return parser;
}

jsonparser cppjson::parseString(const std::string& inputString) {
	std::vector<std::string> tokens;
	tokenize(inputString, tokens);
	jsonparser p(tokens);
	p.parse();
	return p;
}

/* object impl */
std::unordered_map<std::string,std::shared_ptr<value>>& object::getMappings() {
	return this->mappings;
}

jsonparser::jsonparser(std::vector<std::string> tok) : tokens(tok), idx(0) {
	this->tokens.push_back(std::string("EOI"));
	this->cur = this->tokens[idx];
}

jsonparser::jsonparser() : idx(0) {
	this->cur = this->tokens[idx];
}

std::shared_ptr<object> jsonparser::getRoot() {
	return this->root;
}

std::shared_ptr<value> object::access(const std::string& path) const {
	size_t pos = path.find('.');
	auto pathSubString(path.substr(0,pos));
	auto ret = this->mappings.find(pathSubString);
	if(ret == this->mappings.end()) {
		throw std::logic_error(std::string("Path not present ") + path);
	} else if(pos <= path.size()) {
		if(ret->second->getType() != value::type_object) {
			throw std::logic_error(std::string("Path ") + path + std::string(
				" did not lead to an object"));
		} else {
			return ret->second->getObject()->access(
				path.substr(pos+1,path.size())
			);
		}
	} else {
		return ret->second;
	}
}

bool object::pathExists(const std::string& path) const {
	try {
		auto ret = this->access(path);
	} catch(...) {
		return false;
	}
	return true;
}

UNITTEST(accessTest1) {
	std::string input = "{ \"o\" : { \"b\" : { \"c\" : \"2\" } } } ";
	std::vector<std::string> tokens;
	tokenize(input, tokens);
	jsonparser p(tokens);
	auto v = p.parseObject();
	AS_T(v->pathExists("o.b.c"));
	auto two = v->access("o.b.c");
	AS_EQ(two->getString(), "2");
}

UNITTEST(accessTest2) {
	std::string input = "{ \"o\" : { \"b\" : { \"c\" : \"2\" } } } ";
	std::vector<std::string> tokens;
	tokenize(input, tokens);
	jsonparser p(tokens);
	auto v = p.parseObject();
	AS_F(v->pathExists("o.b.z"));
	bool hasThrown = false;
	try {
		auto two = v->access("o.b.z");
	} catch(std::logic_error& e) {
		hasThrown = true;
	}
	AS_T(hasThrown);
}

void object::print(std::ostream& s, size_t in) {
	s<<"{";
	s<<std::endl;
	for(auto it : mappings) {
		for(size_t i = 0; i < in; i++) {
			s<<"    ";
		}
		s<<'"';
		s<<it.first;
		s<<'"';
		s<<" : ";
		it.second->print(s, in+1);
		s<<std::endl;
	}
	for(size_t i = 0; i < in; i++) {
		s<<"    ";
	}

	s<<std::endl;
}

const std::string& jsonparser::nextToken() {
	cur = this->tokens[++this->idx];
	//return this->tokens[+this->idx];
	return cur;
}

const std::string& jsonparser::curToken() const {
	return this->tokens[this->idx];
}

/* value impl */
value::value() : type(value::type_null) {
}

value::value(const std::string& s) : str(s) {
	if(s == "true" || s == "false") {
		this->type = value::type_boolean;
	
	} else if(s == "null") {
		this->type = value::type_null;
	} else {
		long i;
		double d;
		if(convertsToInt(s, i)) {
			this->setType(value::type_number_int);
		} else if(convertsToFloat(s, d)) {
			this->setType(value::type_number_float);
		} else if(s == "null") {
			this->setType(value::type_null);
		} else if(isString(s)) {
			this->setType(value::type_string);
		} else {
			throw std::logic_error(std::string("unexcepted input while"
				"parsing value input was ") + s);
		}
	}
}

value::types value::getType() const {
	return this->type;
}

void value::setType(types t) {
	this->type = t;
}

bool value::getBool() const {
	return this->str == "true";
}

long value::getInt() const {
	return std::stol(this->str);
}

double value::getFloat() const {
	return std::stod(this->str);
}

void value::setString(std::string s) {
	this->str = s;
}

std::string value::getString() const {
	return this->str;
}

std::vector<std::shared_ptr<value>> value::getArray() {
	return this->array;
}

void value::setArray(std::vector<std::shared_ptr<value>> a) {
	this->array = a;
}

std::shared_ptr<object> value::getObject() {
	if(!this->obj || this->type != value::type_object) {
		throw std::logic_error("value is not of type object");
	}
	return this->obj;
}

void value::setObject(std::shared_ptr<object> o) {
	this->obj = o;
}

bool value::operator==(const value& v) const {
	return this->str == v.getString();
}

bool value::operator<(const value& v) const {
	return this->str < v.getString();
}

void value::print(std::ostream& s, size_t in) {
	if(this->obj) {
		for(size_t i = 0; i < in; i++) {
			s<<"    ";
		}
		this->obj->print(s, in+1);
		//s<<std::endl;
	} else if(!this->array.empty()) {
		s<<"["<<std::endl;
		for(auto it : this->array) {
			/*for(size_t i = 0; i < in; i++) {
				std::cout<<"    ";
			}*/
			it->print(s, in+1);
			//s<<std::endl;
		}
		for(size_t i = 0; i < in; i++) {
			s<<"    ";
		}
		s<<"]";
	} else {
		s<<'"';
		s<<this->str;
		s<<'"';
		//s<<std::endl;
	}
}

/* here comes the parsing stuff */

void jsonparser::print(std::ostream& s) {
	this->root->print(s, 0);
}

void jsonparser::parse() {
	this->root = this->parseObject();
}

std::shared_ptr<object> jsonparser::parseObject() {
	auto ret(std::make_shared<object>());
	if(this->curToken() != "{") {
		throw std::logic_error("excepted a \"{\" while parsing an object");
	} else {
		this->nextToken(); // eat the {
	}
	while(this->curToken() != "}") {
		std::string name(this->curToken());
		if(!isString(name)) {
			throw std::logic_error(std::string(
				"excepted a string while parsing an object not ") + name);
		}

		if(this->nextToken() != ":") {
			throw std::logic_error(std::string(
				"excepted a colon while parsing an object not ") + name);
		} else {
			this->nextToken(); // eat the colon
		}

		std::shared_ptr<value> v = this->parseValue();
		ret->getMappings().insert(std::make_pair(name,v));

		if(this->curToken() != "}" && this->curToken() != ",") {
			throw std::logic_error(std::string(
				"excepted a comma or \"}\" while parsing an object not ") + name);
		} else if(this->curToken() == ",") {
			this->nextToken(); // eat the ,
		}
	}
	this->nextToken(); // eat the }
	return ret;
}

UNITTEST(object1) {
	std::vector<std::string> in = { "{", "}" };
	jsonparser p(in);
	auto v = p.parseObject();
	AS_EQ(v->getMappings().size(), 0u);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(object2) {
	std::vector<std::string> in = { "{","key",":","value","}" };
	std::unordered_map<std::string,std::shared_ptr<value>> cmp = {
		{ "key", std::make_shared<value>("value") }
	};
	jsonparser p(in);
	auto v = p.parseObject();
	AS_EQ(v->getMappings().size(), 1u);
	AS_EQ(p.curToken(), "EOI");
	for(auto it : v->getMappings()) {
		auto f = cmp.find(it.first);
		if(AS_T(f != v->getMappings().end())) {
			AS_T(*it.second == *f->second);
		}
	}
}

UNITTEST(object3) {
	std::vector<std::string> in = { "{",
			"key" ,":","value",",",
			"key2",":","value2",
		"}" };
	jsonparser p(in);
	auto v = p.parseObject();
	AS_EQ(v->getMappings().size(), 2u);
	AS_EQ(p.curToken(), "EOI");
	std::unordered_map<std::string,std::shared_ptr<value>> cmp = {
		{ "key", std::make_shared<value>("value") },
		{ "key2", std::make_shared<value>("value2") }
	};
	for(auto it : v->getMappings()) {
		auto f = cmp.find(it.first);
		if(AS_T(f != v->getMappings().end())) {
			AS_T(*it.second == *f->second);
		}
	}
}

std::shared_ptr<value> jsonparser::parseValue() {
	auto ret(std::make_shared<value>());
	if(this->curToken() == "{") {
		ret->setObject(parseObject());
		ret->setType(value::type_object);
	}  else if(this->curToken() == "[") {
		ret->setArray(parseArray());
		ret->setType(value::type_array);
	} else if(this->curToken() == "true") {
		ret->setString(this->curToken());
		ret->setType(value::type_boolean);
		this->nextToken();
	} else if(this->curToken() == "false") {
		ret->setString(this->curToken());
		ret->setType(value::type_boolean);
		this->nextToken();
	} else {
		long i;
		double d;
		if(convertsToInt(this->curToken(), i)) {
			ret->setString(this->curToken());
			ret->setType(value::type_number_int);
		} else if(convertsToFloat(this->curToken(), d)) {
			ret->setString(this->curToken());
			ret->setType(value::type_number_float);
		} else if(this->curToken() == "null") {
			ret->setType(value::type_null);
		} else if(isString(this->curToken())) {
			ret->setString(this->curToken());
			ret->setType(value::type_string);
		} else {
			throw std::logic_error(std::string("unexcepted input while"
				"parsing value input was ") + this->curToken());
		}
		this->nextToken(); // eat whatever we found
	}

	return ret;
}

UNITTEST(valuetest1) {
	std::vector<std::string> in = { "true" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_boolean);
	AS_EQ(v->getBool(), true);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(valuetest2) {
	std::vector<std::string> in = { "false" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_boolean);
	AS_EQ(v->getBool(), false);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(valuetest3) {
	std::vector<std::string> in = { "123" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_number_int);
	AS_EQ(v->getInt(), 123l);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(valuetest4) {
	std::vector<std::string> in = { "123.0" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_number_float);
	AS_EQ(v->getFloat(), 123.0);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(valuetest5) {
	std::vector<std::string> in = { "{", "}" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_object);
	AS_EQ(p.curToken(), "EOI");
}

UNITTEST(valuetest6) {
	std::vector<std::string> in = { "[", "]" };
	jsonparser p(in);
	auto v = p.parseValue();
	AS_EQ(v->getType(), value::type_array);
	AS_EQ(p.curToken(), "EOI");
}

std::vector<std::shared_ptr<value>> jsonparser::parseArray() {
	std::vector<std::shared_ptr<value>> ret;
	if(this->curToken() != "[") {
		throw std::logic_error(std::string("expected a ] not ") +
			this->curToken()); // array begin with a [
	} else {
		this->nextToken(); // eat the [
	}
	while(this->curToken() != "]") {
		ret.push_back(parseValue());
		if(this->curToken() != "," && this->curToken() != "]") {
			throw std::logic_error(std::string("expected a comma not a ") +
					this->curToken());
		}
		if(this->curToken() == ",") {
			this->nextToken();
		}
	}
	this->nextToken(); // eat the ]
	return ret;
}

UNITTEST(arraytest1) {
	std::vector<std::string> in = { "[","hello","]" };
	jsonparser p(in);
	auto v = p.parseArray();
	AS_EQ(v.size(), 1u);
	AS_EQ(p.curToken(), "EOI");
}
