/* $Id: load_xml_handle.h 1182 2008-12-22 22:08:36Z dchassin $
 	Copyright (C) 2008 Battelle Memorial Institute
 * 
 * Authors:
 *	Matthew Hauer <matthew.hauer@pnl.gov>, 6 Nov 07 -
 *
 * Versions:
 *	1.0 - MH - initial version
 *
 * Credits:
 *	adapted from SAX2PrintHndl.h
 *
 *	@file load_xml_handle.h
 *	@addtogroup load XML file loader
 *	@ingroup core
 *
 */

#include    <xercesc/sax2/DefaultHandler.hpp>
#include    <xercesc/framework/XMLFormatter.hpp>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <vector>

extern "C" {
	#include "class.h"
	#include "load.h"
	#include "globals.h"
	#include "module.h"
	#include "convert.h"
	#include "random.h"
	#include "output.h"
	#include "unit.h"
}
#include "gridlabd.h"

XERCES_CPP_NAMESPACE_USE

using std::vector;

typedef enum {
	EMPTY = 0,
	LOAD,			//	got load tag & not in a module
	MODULE_STATE,	//	in a module
	MODULE_PROP,	//	setting a module property
	OBJECT_STATE,	//	setting up an object
	OBJECT_PROP,	//	setting an object property
	GLOBAL_STATE,	//	setting up for global variables
	GLOBAL_PROP,	//	setting a global variable
	CLOCK_STATE,
	CLOCK_PROP
} gld_state;

class gldStack{
public:
	gldStack(){next = NULL; clear();}
	gldStack(gldStack *ptr){next = ptr; clear();}
	~gldStack(){if(next != NULL) delete next;}

	void clear(){object_type[0] = 0; object_id[0] = 0; object_name[0] = 0; keyword[0] = 0;}
	char object_type[64];
	char object_id[64];
	char object_name[64];
	char keyword[64];
	gldStack *next;
};

class gld_loadHndl : public DefaultHandler, public XMLFormatTarget {
public:
	gld_loadHndl();
    gld_loadHndl(const char* const, const XMLFormatter::UnRepFlags, const bool);
    ~gld_loadHndl();

	bool did_load(){return load_state;}

	void writeChars(const XMLByte* const toWrite);
	void writeChars(const XMLByte* const toWrite, const unsigned int count, XMLFormatter* const formatter);

	void setDocumentLocator(const Locator *const locator);
	void endDocument();
    void endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname);
    void characters(const XMLCh* const chars, const unsigned int length);
    void ignorableWhitespace(const XMLCh* const chars, const unsigned int length);
    void processingInstruction(const XMLCh* const target, const XMLCh* const data);
    void startDocument();
    void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const Attributes& attributes);

    void warning(const SAXParseException& exc);
    void error(const SAXParseException& exc);
    void fatalError(const SAXParseException& exc);

    void notationDecl(const XMLCh* const name, const XMLCh* const publicId, const XMLCh* const systemId);
    /* void unparsedEntityDecl(const XMLCh* const name, const XMLCh* const publicId, const XMLCh* const systemId, const XMLCh* const notationName); */

	char *build_object_vect(int start, int end);
	void parse_property(char *buffer);
private :
    XMLFormatter    fFormatter;
	bool			fExpandNS ;

	gld_state	stack_state;
	Locator const *locator;
	char	errmsg[1024];
	MODULE *module;
	CLASS *oclass;
	OBJECT *obj;
	PROPERTY *prop;
	char propname[256];

	char *read_module_prop(char *buffer, size_t len);
	char *read_global_prop(char *buffer, size_t len);
	char *read_object_prop(char *buffer, size_t len);
	char *read_clock_prop(char *buffer, size_t len);

	char *start_element_empty(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_load(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_module(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_module_prop(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_module_build_object(const Attributes &attributes);
	char *start_element_object(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_object_prop(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_global(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_global_prop(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_clock(char *buffer, size_t len, const Attributes& attributes);
	char *start_element_clock_prop(char *buffer, size_t len, const Attributes& attributes);

	char *end_element_empty(char *buffer, size_t len);
	char *end_element_load(char *buffer, size_t len);
	char *end_element_module(char *buffer, size_t len);
	char *end_element_module_prop(char *buffer, size_t len);
	char *end_element_object(char *buffer, size_t len);
	char *end_element_object_prop(char *buffer, size_t len);
	char *end_element_global(char *buffer, size_t len);
	char *end_element_global_prop(char *buffer, size_t len);
	char *end_element_clock(char *buffer, size_t len);
	char *end_element_clock_prop(char *buffer, size_t len);

	int depth;

	char module_name[64];
	/* char obj_id[64];
	char obj_type[64];
	char keyword[64]; */

	int first, last;
	bool load_ready;
	bool load_state;
	int object_count, class_count;

	vector<OBJECT *> obj_vect;
	gldStack *stack_ptr;
};
