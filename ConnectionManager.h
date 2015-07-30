#pragma once
#include <memory/Alloc.h>
#include "TubesTypes.h"
#include "Connection.h"

class ConnectionManager {
public:
	

private:
	Connection* Connect( const tString& address, Port port );

};