/** $Id: exception.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exception.c
	@addtogroup exception Exception handling
	@ingroup core
	
	Exception handlers are created and caught using the exception module

	Note that the exception handlers are only necessary for C code.  This
	will usually by something like this:

	@code
	TRY {
		// some code...
		if (errno)
			THROW("Error %d: %s", errno, strerror(errno));
		// more code...
	} CATCH(char *msg) {
		output_error("Exception caught: %s", msg);
	} ENDCATCH
	@endcode
	
	In C++ code, you can use THROW() to throw an exception that is
	to be caught by the main system exception handler. 

	The recommended format for exception messages is as follows:

	- <b>Core exceptions</b> should include the offending function
	  call and the specifics of the error in context, as in	
		\code 
	  	throw_exception("object_tree_add(obj='%s:%d', name='%s'): memory allocation failed (%s)", 
			obj->oclass->name, obj->id, name, strerror(errno));
		\endcode
	  For functions that a \p static, you should include the file in which
	  it is implemented, as in
		\code
		throw_exception("object.c/addto_tree(tree.name='%s', item.name='%s'): strcmp() returned unexpected value %d", tree->name, item->name, rel);
		\endcode
	  One important situation to consider are exception that occur during I/O
	  where the context in GridLAB may not be as important the context in the
	  I/O stream, as in
		\code
		throw_exception("%s(%d): unexpected EOF", filename, linenum);
		\endcode

	- <b>Module exception</b> should include the offending object as well
	  as information about the location and nature of the exception, as in
		\code
		GL_THROW("%s:%d circuit %d has an invalid circuit type (%d)", obj->oclass->name, obj->id, c->id, (int)c->type);
		\endcode

 @{
 **/

#include <string.h>
#include "exception.h"
#include "output.h"

EXCEPTIONHANDLER *handlers = NULL;

/** Creates an exception handler for use in a try block 
	@return a pointer to an EXCEPTIONHANDLER structure
 **/
EXCEPTIONHANDLER *create_exception_handler(void)
{
	EXCEPTIONHANDLER *ptr = malloc(sizeof(EXCEPTIONHANDLER));
	ptr->next = handlers;
	ptr->id = (handlers==NULL?0:handlers->id)+1;
	memset(ptr->msg,0,sizeof(ptr->msg));
	handlers = ptr;
	return ptr;
}

/** Deletes an exception handler from the handler list
 **/
void delete_exception_handler(EXCEPTIONHANDLER *ptr) /**< a pointer to the exception handler */
{
	EXCEPTIONHANDLER *target = ptr->next;
	while (handlers!=target)
	{
		ptr = handlers;
		handlers=ptr->next;
		free(ptr);
		/* if(handlers == NULL) break; */
	}
}

/** Throw an exception
 **/
void throw_exception(char *format, /**< the format string */
					 ...) /**< the parameters of the message */
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr);
	va_end(ptr);

	if (handlers)
	{
		strncpy(handlers->msg,buffer,sizeof(handlers->msg));
		longjmp(handlers->buf,handlers->id);
	}
	else
	{
		output_fatal("unhandled exception: %s", buffer);
		exit(-1);
	}
}

/** Retrieves the message of the most recently thrown exception
	@return a \e char* pointer to the message
 **/
char *exception_msg(void)
{
	return handlers->msg;
}

/**@}*/
