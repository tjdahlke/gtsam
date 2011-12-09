/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file Module.ccp
 * @author Frank Dellaert
 **/

#include "Module.h"
#include "utilities.h"
#include "pop_actor.h"

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_clear_actor.hpp>
#include <boost/foreach.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace wrap;
using namespace BOOST_SPIRIT_CLASSIC_NS;

typedef rule<BOOST_SPIRIT_CLASSIC_NS::phrase_scanner_t> Rule;

/* ************************************************************************* */
// We parse an interface file into a Module object.
// The grammar is defined using the boost/spirit combinatorial parser.
// For example, str_p("const") parses the string "const", and the >>
// operator creates a sequence parser. The grammar below, composed of rules
// and with start rule [class_p], doubles as the specs for our interface files.
/* ************************************************************************* */

Module::Module(const string& interfacePath,
	       const string& moduleName, bool enable_verbose) : name(moduleName), verbose(enable_verbose)
{
  // these variables will be imperatively updated to gradually build [cls]
  // The one with postfix 0 are used to reset the variables after parse.
	ReturnValue retVal0, retVal;
  Argument arg0, arg;
  ArgumentList args0, args;
  Constructor constructor0(enable_verbose), constructor(enable_verbose);
  Method method0(enable_verbose), method(enable_verbose);
  StaticMethod static_method0(enable_verbose), static_method(enable_verbose);
  Class cls0(enable_verbose),cls(enable_verbose);
  vector<string> namespaces, namespaces_return;

  //----------------------------------------------------------------------------
  // Grammar with actions that build the Class object. Actions are
  // defined within the square brackets [] and are executed whenever a
  // rule is successfully parsed. Define BOOST_SPIRIT_DEBUG to debug.
  // The grammar is allows a very restricted C++ header
  // ----------------------------------------------------------------------------

  Rule comments_p =  comment_p("/*", "*/") |	comment_p("//", eol_p);

  // lexeme_d turns off white space skipping
  // http://www.boost.org/doc/libs/1_37_0/libs/spirit/classic/doc/directives.html

  Rule basisType_p =
    (str_p("string") | "bool" | "size_t" | "int" | "double");

  Rule keywords_p =
  	(str_p("const") | "static" | "namespace" | basisType_p);

  Rule eigenType_p =
    (str_p("Vector") | "Matrix");

  Rule className_p  = lexeme_d[upper_p >> *(alnum_p | '_')] - eigenType_p - keywords_p;

  Rule namespace_name_p = lexeme_d[lower_p >> *(alnum_p | '_')] - keywords_p;

  Rule namespace_arg_p = namespace_name_p[push_back_a(arg.namespaces)] >> str_p("::");

  Rule classPtr_p =
  	*namespace_arg_p >>
    className_p     [assign_a(arg.type)] >> 
    ch_p('*')       [assign_a(arg.is_ptr,true)];

  Rule classRef_p =
    !str_p("const") [assign_a(arg.is_const,true)] >> 
  	*namespace_arg_p >>
    className_p     [assign_a(arg.type)] >> 
    ch_p('&')       [assign_a(arg.is_ref,true)];

  Rule argEigenType_p =
  	eigenType_p[assign_a(arg.type)] >>
  	!ch_p('*')[assign_a(arg.is_ptr,true)];

  Rule eigenRef_p =
      !str_p("const") [assign_a(arg.is_const,true)] >>
      eigenType_p     [assign_a(arg.type)] >>
      ch_p('&')       [assign_a(arg.is_ref,true)];

  Rule name_p = lexeme_d[alpha_p >> *(alnum_p | '_')];

  Rule argument_p = 
    ((basisType_p[assign_a(arg.type)] | argEigenType_p | classRef_p | eigenRef_p | classPtr_p)
    		>> name_p[assign_a(arg.name)])
    [push_back_a(args, arg)]
    [assign_a(arg,arg0)];

  Rule argumentList_p = !argument_p >> * (',' >> argument_p);

  Rule constructor_p = 
    (className_p >> '(' >> argumentList_p >> ')' >> ';' >> !comments_p)
    [assign_a(constructor.args,args)]
    [assign_a(constructor.name,cls.name)]
    [assign_a(args,args0)]
    [push_back_a(cls.constructors, constructor)]
    [assign_a(constructor,constructor0)];

  Rule namespace_ret_p = namespace_name_p[push_back_a(namespaces_return)] >> str_p("::");

  Rule returnType1_p =
		(basisType_p[assign_a(retVal.type1)][assign_a(retVal.category1, ReturnValue::BASIS)]) |
		((*namespace_ret_p)[assign_a(retVal.namespaces1, namespaces_return)][clear_a(namespaces_return)]
				>> (className_p[assign_a(retVal.type1)][assign_a(retVal.category1, ReturnValue::CLASS)]) >>
				!ch_p('*')[assign_a(retVal.isPtr1,true)]) |
		(eigenType_p[assign_a(retVal.type1)][assign_a(retVal.category1, ReturnValue::EIGEN)]);

  Rule returnType2_p =
		(basisType_p[assign_a(retVal.type2)][assign_a(retVal.category2, ReturnValue::BASIS)]) |
		((*namespace_ret_p)[assign_a(retVal.namespaces2, namespaces_return)][clear_a(namespaces_return)]
				>> (className_p[assign_a(retVal.type2)][assign_a(retVal.category2, ReturnValue::CLASS)]) >>
				!ch_p('*')  [assign_a(retVal.isPtr2,true)]) |
		(eigenType_p[assign_a(retVal.type2)][assign_a(retVal.category2, ReturnValue::EIGEN)]);

  Rule pair_p = 
    (str_p("pair") >> '<' >> returnType1_p >> ',' >> returnType2_p >> '>')
    [assign_a(retVal.isPair,true)];

  Rule void_p = str_p("void")[assign_a(retVal.type1)];

  Rule returnType_p = void_p | returnType1_p | pair_p;

  Rule methodName_p = lexeme_d[lower_p >> *(alnum_p | '_')];

  Rule method_p = 
    (returnType_p >> methodName_p[assign_a(method.name)] >>
     '(' >> argumentList_p >> ')' >> 
     !str_p("const")[assign_a(method.is_const_,true)] >> ';' >> *comments_p)
    [assign_a(method.args,args)]
    [assign_a(args,args0)]
    [assign_a(method.returnVal,retVal)]
    [assign_a(retVal,retVal0)]
    [push_back_a(cls.methods, method)]
    [assign_a(method,method0)];

  Rule staticMethodName_p = lexeme_d[(upper_p | lower_p) >> *(alnum_p | '_')];

  Rule static_method_p =
    (str_p("static") >> returnType_p >> staticMethodName_p[assign_a(static_method.name)] >>
     '(' >> argumentList_p >> ')' >> ';' >> *comments_p)
    [assign_a(static_method.args,args)]
    [assign_a(args,args0)]
    [assign_a(static_method.returnVal,retVal)]
    [assign_a(retVal,retVal0)]
    [push_back_a(cls.static_methods, static_method)]
    [assign_a(static_method,static_method0)];

  Rule includes_p = str_p("#include") >> ch_p('<') >> (*(anychar_p - '>'))[push_back_a(cls.includes)] >> ch_p('>');

  Rule functions_p = includes_p | constructor_p | method_p | static_method_p;

  Rule class_p = (str_p("class") >> className_p[assign_a(cls.name)] >> '{' >>
    *(functions_p | comments_p) >>
    str_p("};"))[assign_a(cls.namespaces, namespaces)][push_back_a(classes,cls)][assign_a(cls,cls0)];

	Rule namespace_def_p = str_p("namespace") >>
			namespace_name_p[push_back_a(namespaces)]
			>> ch_p('{') >>
					*(class_p | namespace_def_p | comments_p) >>
					str_p("}///\\namespace") >> !namespace_name_p // end namespace, avoid confusion with classes
					[pop_a(namespaces)];

  Rule module_content_p =	 comments_p | class_p | namespace_def_p ;

  Rule module_p = *module_content_p >> !end_p;

  //----------------------------------------------------------------------------
  // for debugging, define BOOST_SPIRIT_DEBUG
# ifdef BOOST_SPIRIT_DEBUG
  BOOST_SPIRIT_DEBUG_NODE(className_p);
  BOOST_SPIRIT_DEBUG_NODE(classPtr_p);
  BOOST_SPIRIT_DEBUG_NODE(classRef_p);
  BOOST_SPIRIT_DEBUG_NODE(basisType_p);
  BOOST_SPIRIT_DEBUG_NODE(name_p);
  BOOST_SPIRIT_DEBUG_NODE(argument_p);
  BOOST_SPIRIT_DEBUG_NODE(argumentList_p);
  BOOST_SPIRIT_DEBUG_NODE(constructor_p);
  BOOST_SPIRIT_DEBUG_NODE(returnType1_p);
  BOOST_SPIRIT_DEBUG_NODE(returnType2_p);
  BOOST_SPIRIT_DEBUG_NODE(pair_p);
  BOOST_SPIRIT_DEBUG_NODE(void_p);
  BOOST_SPIRIT_DEBUG_NODE(returnType_p);
  BOOST_SPIRIT_DEBUG_NODE(methodName_p);
  BOOST_SPIRIT_DEBUG_NODE(method_p);
  BOOST_SPIRIT_DEBUG_NODE(class_p);
  BOOST_SPIRIT_DEBUG_NODE(module_p);
# endif
  //----------------------------------------------------------------------------

  // read interface file
  string interfaceFile = interfacePath + "/" + moduleName + ".h";
  string contents = file_contents(interfaceFile);

  // and parse contents
  parse_info<const char*> info = parse(contents.c_str(), module_p, space_p);
  if(!info.full) {
    printf("parsing stopped at \n%.20s\n",info.stop);
    throw ParseFailed(info.length);
  }
}

/* ************************************************************************* */
template<class T>
void verifyArguments(const vector<string>& validArgs, const vector<T>& vt) {
	BOOST_FOREACH(const T& t, vt) {
		BOOST_FOREACH(Argument arg, t.args) {
			string fullType = arg.qualifiedType("::");
			if(std::find(validArgs.begin(), validArgs.end(), fullType)
			== validArgs.end())
				throw DependencyMissing(fullType, t.name);
		}
	}
}

/* ************************************************************************* */
void Module::matlab_code(const string& toolboxPath, 
			 const string& nameSpace, 
			 const string& mexExt,
			 const string& mexFlags)
{
    string installCmd = "install -d " + toolboxPath;
    system(installCmd.c_str());

    // create make m-file
    string matlabMakeFile = toolboxPath + "/make_" + name + ".m";
    ofstream ofs(matlabMakeFile.c_str());
    if(!ofs) throw CantOpenFile(matlabMakeFile);

    // create the (actual) make file
    string makeFile = toolboxPath + "/Makefile";
    ofstream make_ofs(makeFile.c_str());
    if(!make_ofs) throw CantOpenFile(makeFile);

    if (verbose) cerr << "generating " << matlabMakeFile << endl;
    emit_header_comment(ofs,"%");
    ofs << "echo on" << endl << endl;
    ofs << "toolboxpath = mfilename('fullpath');" << endl;
    ofs << "delims = find(toolboxpath == '/');" << endl;
    ofs << "toolboxpath = toolboxpath(1:(delims(end)-1));" << endl;
    ofs << "clear delims" << endl;
    ofs << "addpath(toolboxpath);" << endl << endl;

    if (verbose) cerr << "generating " << makeFile << endl;
    emit_header_comment(make_ofs,"#");
    make_ofs << "\nMEX = mex\n";
    make_ofs << "MEXENDING = " << mexExt << "\n";
    make_ofs << "mex_flags = " << mexFlags << "\n\n";

    //Dependency check list
    std::vector<string> validArgs;
    validArgs.push_back("string");
    validArgs.push_back("int");
    validArgs.push_back("bool");
    validArgs.push_back("size_t");
    validArgs.push_back("double");
    validArgs.push_back("Vector");
    validArgs.push_back("Matrix");

    // add 'all' to Makefile
    make_ofs << "all: ";
    BOOST_FOREACH(Class cls, classes) {
    	make_ofs << cls.qualifiedName() << " ";
			//Create a list of parsed classes for dependency checking
			validArgs.push_back(cls.qualifiedName("::"));
    }
    make_ofs << "\n\n";

    // generate proxy classes and wrappers
    BOOST_FOREACH(Class cls, classes) {
      // create directory if needed
      string classPath = toolboxPath + "/@" + cls.qualifiedName();
      string installCmd = "install -d " + classPath;
      system(installCmd.c_str());

      // create proxy class
      string classFile = classPath + "/" + cls.qualifiedName() + ".m";
      cls.matlab_proxy(classFile);

      // verify all of the function arguments
      verifyArguments<Constructor>(validArgs, cls.constructors);
      verifyArguments<StaticMethod>(validArgs, cls.static_methods);
      verifyArguments<Method>(validArgs, cls.methods);

      // create constructor and method wrappers
      cls.matlab_constructors(toolboxPath,nameSpace);
      cls.matlab_static_methods(toolboxPath,nameSpace);
      cls.matlab_methods(classPath,nameSpace);

      // add lines to make m-file
      ofs << "%% " << cls.qualifiedName() << endl;
      ofs << "cd(toolboxpath)" << endl;
      cls.matlab_make_fragment(ofs, toolboxPath, mexFlags);

      // add section to the (actual) make file
      make_ofs << "# " << cls.qualifiedName() << endl;
      cls.makefile_fragment(make_ofs);
    }  

    // finish make m-file
    ofs << "cd(toolboxpath)" << endl << endl;
    ofs << "echo off" << endl;
    ofs.close();

    // make clean at end of Makefile
    make_ofs << "\n\nclean: \n";
    make_ofs << "\trm -rf *.$(MEXENDING)\n";
    BOOST_FOREACH(Class cls, classes)
    	make_ofs << "\trm -rf @" << cls.qualifiedName() << "/*.$(MEXENDING)\n";

    // finish Makefile
    make_ofs << "\n" << endl;
    make_ofs.close();
  }

/* ************************************************************************* */
