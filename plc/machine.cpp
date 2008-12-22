/** $Id: machine.cpp,v 1.10 2007/12/12 22:05:50 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file machine.cpp
	@addtogroup machine Machine implementation
	@ingroup plc

	The machine object implements the PLC code.  On Windows machines, it compiles the code 
	using \p mingw, which must be installed on your computer and is provided with the
	GridLAB-D distribution for that purpose.  On Linux machines it uses	\p gcc to compile the code.

	The following is a sample of PLC code that implements a simple residential thermostat

	@code
	#define HEAT 0
	#define COOL 1
	#define OFF 2
	#define HYST 1.0
	#define LOCKOUT 300.0

	int state = OFF;
	BEGIN_DATA
		DOUBLE(Tair)
		DOUBLE(heating_setpoint)
		DOUBLE(cooling_setpoint)
	END_DATA

	#define Tair DATA(double,0)
	#define heating_setpoint DATA(double,1)
	#define cooling_setpoint DATA(double,2)

	INIT 
	{
		state=OFF;
		return 0;
	}

	CODE(dt,dev)
	{
		switch (state) {
		case OFF:
			if (Tair<heating_setpoint-HYST)
			{
				state=HEAT; 
				return 0;
			}
			else if (Tair>cooling_setpoint+HYST)
			{
				state=COOL; 
				return 0;
			}
			break;
		case HEAT:
			if (Tair>heating_setpoint+HYST)
			{
				state=OFF;
				return LOCKOUT;
			}
			break;
		case COOL:
			if (Tair<cooling_setpoint-HYST)
			{
				state=OFF;
				return LOCKOUT;
			}
			break;
		default:
			break;
		}
		return 1;
	}
	@endcode
 @{
 **/
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdarg.h>
#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#define putenv _putenv
#else
#include <unistd.h>
#include <dlfcn.h>
#endif
#include "machine.h"
#include "plc.h"
#include "comm.h"

#ifdef WIN32
char path[1024] = "c:/mingw/bin";
char tmpdir[1024] = "c:/temp";
#else
char path[1024] = "/usr/bin";
char tmpdir[1024] = "/tmp";
#endif

int exec(char *format,...)
{
	char cmd[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(cmd,format,ptr);
	va_end(ptr);
	gl_debug("Running '%s'", cmd);
	return system(cmd);
}

void sendx(machine *src, char *to, void *str, unsigned int len)
{
	if (len==0)
		len=(unsigned int)strlen((char*)str);
	OBJECT *obj = gl_get_object(to);
	if (obj)
	{
		machine *dst = OBJECTDATA(obj,plc)->get_machine();
		src->send(new Message(str,len,src,dst));
	}
	else
		gl_error("sendx(machine=%x, to='%s', str='%-.8s%s', len=%d): message recipient '%s' not found", src, to, (char*)str, len>8?"...":"", len, to);
}

int recvx(machine *dst, char *from, void *str, unsigned int len)
{
	Message *msg = dst->receive();
	if (msg!=NULL)
	{
		unsigned int sz = (unsigned int)msg->get_size()+1;
		if (sz>len) sz=len;
		memcpy(str,msg->get_data(),sz);
		strcpy(from,"(unknown)");
		return sz;
	}
	else 
		return 0;
}

machine::machine(void)
: _link()
{
	_code=NULL;
	_init=NULL;
	_data=NULL;
	wait=-1;
}

machine::~machine(void)
{
}

// COMPILE
// This function load a source file and generates
// a machine that can be executed
// RETURN
//   -1 - failed
//   0 - succeeded
int machine::compile(char *source)
{
	extern char libpath[1024], incpath[1024];
	char cfile[1024];
	char ofile[1024];
	char lfile[1024];
	char name[64], *basename;
	char *pSlash, *pDot;
	char oldpath[1024]="", newpath[1024];
	char buffer[1024];
	FILE *fp;

	/* path */
	if (getenv("PATH")) strcpy(oldpath, getenv("PATH"));
	if (strcmp(oldpath,"")==0)
		sprintf(newpath,"PATH=%s",path);
	else
		sprintf(newpath,"PATH=%s;%s",path,oldpath);
	putenv(newpath);

	/* build the basename */
	strcpy(name,source);
	pSlash = strrchr(name,'/');
	pDot = strrchr(name,'.');
	if (pDot!=NULL && pDot>pSlash)
		*pDot='\0';
	basename = (pSlash==NULL ? name : pSlash+1);
	sprintf(cfile,"%s/%s.c", tmpdir,basename);
	sprintf(ofile,"%s/%s.o", tmpdir,basename);

#ifdef WIN32
	sprintf(lfile,"%s/%s.dll", tmpdir,basename);
#else
	sprintf(lfile,"%s/lib%s.so", tmpdir,basename);
#endif

	/* build source code */
	gl_verbose("converting %s to %s...", source, cfile);
	fp=fopen(cfile,"w");
	if (fp==NULL)
	{
		gl_error("%s: %s", cfile, strerror(errno));
		return -1;
	}
	fprintf(fp,"/* this code automatically generated by gridlab-d " __FILE__ "*/\n");
	fprintf(fp,"/* wd=%s */\n", getcwd(buffer,sizeof(buffer)));
	fprintf(fp,"#include <plc.h>\n");
	fprintf(fp,"#include \"%s/%s\"\n", buffer,source);
	fprintf(fp,"/* END */\n");
	fclose(fp);

	/* compile source */
	gl_verbose("compiling %s from %s using incpath '%s'...", ofile, cfile,incpath);
	unlink(ofile);
	if (exec("gcc -I%s -c %s -o %s",incpath,cfile,ofile)!=0)
		return -1;

	/* link */
	gl_verbose("converting %s to dynamic link library...", ofile);
	unlink(lfile);
	if (exec("gcc -export-all-symbols -shared -o %s -Wl,%s", lfile,ofile)!=0)
		return -1;

	/* load the code (loader is in main.cpp) */
	gl_verbose("loading dynamic link library %s...", ofile);
	wait = load_library(lfile,&_code,&_init,&_data);
	if (wait<0)
	{
#ifdef WIN32
		gl_error("%s: %s", lfile, strerror(errno));
#else
		gl_error("%s: %s", lfile, dlerror());
#endif
		return -1;
	}
	if (_code==NULL)
	{
		gl_error("%s: CODE block not found", source);
		return -1;
	}
	if (_init==NULL)
	{
		gl_error("%s: INIT block not found", source);
		return -1;
	}
	if (_data==NULL)
	{
		gl_error("%s: DATA block not found", source);
		return -1;
	}
	return 0;
}

// INIT
// Initializes the machine
// RETURN
//   -1 - failed
//    0 - succeeded
//    n - succeeded, wait n seconds before calling run
int machine::init(OBJECT *parent)
{
	gl_debug("Connecting PLC to %s:%d...", parent->oclass->name, parent->id);
	PLCDATA item;
	for (item=_data; item->name!=NULL; item++)
	{
		PROPERTY *p = gl_get_property(parent,item->name);
		if (p==NULL)
		{
			gl_error("PCL data item %s is not found in object %s:%d", item->name, parent->oclass->name, parent->id);
			return -1;
		}
		else if (item->type!=p->ptype)
		{
			gl_error("PLC data item %s does not match parent's type", item->name);
			return -1;
		}
		else
			item->addr = (void*)((char*)(parent+1)+(unsigned int64)p->addr);
	}
	gl_debug("Initializing %s:%d PLC...", parent->oclass->name, parent->id);
	return (*_init)();
}

// RUN
// Runs the machine once through
// RETURN
//   -1 - failed
//    0 - succeeded, call again asap
//    n - succeeded, call again after n seconds
int machine::run(double dt)
{
	extern TIMESTAMP *pGlobalClock;
	static PLCDEV dev = {(unsigned int)((*pGlobalClock)/TS_SECOND),{&sendx,&recvx}};
	return (*_code)(this,dt,&dev);
}

void machine::connect(comm *ptr)
{
	net = ptr;
}

void machine::deliver(Message *msg)
{
	_link.add(msg);
}

Message *machine::receive(void)
{
	return _link.take();
}

void machine::send(Message *msg)
{
	net->route(msg);
}

void machine::send(char *to, void *str, size_t len)
{
	if (len==0)
		len=strlen((char*)str);
	OBJECT *obj = gl_get_object(to);
	machine *dst = OBJECTDATA(obj,plc)->get_machine();
	send(new Message(str,len,this,dst));
}

/**@}*/
