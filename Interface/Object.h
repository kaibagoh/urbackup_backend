#ifndef IOBJECT_H
#define IOBJECT_H

#include "Types.h"

class IObject 
{
public:
	virtual ~IObject(void)
	{
	}

	virtual void Remove(void)
	{
		delete this;
	}
};

class ObjectScope
{
public:
	ObjectScope(IObject *obj)
		: obj(obj) {}
	~ObjectScope(void){
		del();
	}
	void clear(){
		del();
	}
	void reset(IObject *pobj) {
		del();
		obj=pobj;
	}
	void release() {
		obj=NULL;
	}
private:
	void del() {
		if(obj!=NULL) obj->Remove();
		obj=NULL;
	}
	IObject *obj;
};

#endif //IOBJECT_H
