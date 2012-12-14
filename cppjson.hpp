#ifndef CPPJSON_HPP
#define CPPJSON_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <ostream>
#include <vector>

namespace cppjson {

class value;

class object {
private:
	std::unordered_map<std::string,std::shared_ptr<value>> mappings;
public:
	/** This function returns the mapping contained by the object. */
	std::unordered_map<std::string,std::shared_ptr<value>>& getMappings();
	/** Given a dot separated path this function returns a corresponding
	 * value. */
	std::shared_ptr<value> access(const std::string&);
	void print(std::ostream&, size_t);
};

class value {
public:
	enum types {
		type_string,
		type_number_int,
		type_number_float,
		type_boolean,
		type_array,
		type_object,
		type_null
	};

private:
	// a value is either a string which maps (string,number,bool) or
	// it is object represented by the obj shared_ptr. The last possiblity
	// is an array of value shared_ptr.
	std::string str;
	std::shared_ptr<object> obj;
	std::vector<std::shared_ptr<value>> array;

	types type;

public:
	value();
	value(const std::string&);
	/** Returns the type of the value
	 *
	 * @return The type of the value.
	 */
	types getType() const;
	void setType(types);

	/** If the value holds a bool this function will return true or false. */
	bool getBool() const;

	/** If the value holds a integer this function will return the int value. */
	long getInt() const;

	/** If the value holds a float this function will return the float value. */
	double getFloat() const;

	/** If the value holds a string this function will return it. */
	std::string getString() const;
	void setString(std::string);

	/** If the value holds an array this function will return it. */
	std::vector<std::shared_ptr<value>> getArray();
	void setArray(std::vector<std::shared_ptr<value>>);

	/** If the value holds an object this function will return it. */
	std::shared_ptr<object> getObject();
	void setObject(std::shared_ptr<object>);
	bool operator==(const value&) const;
	bool operator<(const value&) const;
	void print(std::ostream&, size_t);
};

class jsonparser {
public:

	/** Returns the root of the parsed json tree.
	 *
	 * @return The root object of the tree.
	 */
	std::shared_ptr<object> getRoot();

	jsonparser();

	/** The passed vector can contain the tokens for parsing */
	jsonparser(std::vector<std::string>);

	/** This methode starts the parsing process. */
	void parse();

	/** This method parses an json object. The object needs to be enclosed by a 
	 * { * and a }.
	 *
	 * @return A shared_ptr which points to an object.
	 * @throws std::logic error if parsing fails.
	 */
	std::shared_ptr<object> parseObject();

	/** This method parses an json value. A value can be a string, a integer
	 * or float number, an object, an array or a "null" string.
	 *
	 * @return A shared_ptr which points to an value.
	 * @throws std::logic error if parsing fails.
	 */
	std::shared_ptr<value> parseValue();

	/** Arrays consists of values or better said shared_ptr of values. The
	 * array is represented by a vector. An array is enclosed by a [ and a ].
	 *
	 * @return A vector of shared_ptr to values.
	 */
	std::vector<std::shared_ptr<value>> parseArray();
	/** Returns the current Token.
	 *
	 * @return The current Token.
	 */
	const std::string& curToken() const;

	/** As a stream and the parsed json file will be pretty printed to it.
	 *
	 * @param The stream to print to.
	 */
	void print(std::ostream&);

private:
	std::shared_ptr<object> root;
	std::vector<std::string> tokens;
	size_t idx;
	std::string cur;

	const std::string& nextToken();
};

/** Returns a jsonparser who has parsed the file given by the parameter. */
jsonparser parseFile(const std::string&);

/** Returns a jsonparser who has parsed the json object contained in the
 * string. */
jsonparser parseString(const std::string&);

std::vector<std::string> getJsonTokenFormFile(const std::string& filename);

} // cppjson
#endif
