
#include <libc/component.h>
#include <base/log.h>
 

extern "C" {
	#include "socket_test.h"

	extern int go(void);
}

void Libc::Component::construct( Libc::Env &env)
{
	Libc::with_libc( [&] () {
		socket_test();
	});
}
