/*
 * \brief  Most minimal 
 * \author Mark Vels
 * \date   2015-11-13
 */

/*
 * Copyright (C) 2008-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/env.h>
#include <base/printf.h>

using namespace Genode;

int main(void)
{
	int count = 0;
	while (1) {
		PDBG("I have repeated myself again already for %d times!", count);

		int loopcount = 100000;
		while(loopcount) loopcount--;
		count++;
	}

	return 0;
}
