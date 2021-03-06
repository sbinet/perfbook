/*
 * rcu_nest.c: user-level implementation of RCU permitting nesting.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (c) 2008 Paul E. McKenney, IBM Corporation.
 */

#include "../api.h"
#include "rcu_nest.h"

void synchronize_rcu(void)
{
	int t;

	/* Memory barrier ensures mutation seen before grace period. */

	smp_mb();

	/* Only one synchronize_rcu() at a time. */

	spin_lock(&rcu_gp_lock);

	/* Advance to a new grace-period number, enforce ordering. */

	rcu_gp_ctr += RCU_GP_CTR_BOTTOM_BIT;
	smp_mb();

	/*
	 * Wait until all threads are either out of their RCU read-side
	 * critical sections or are aware of the new grace period.
	 */

	for_each_thread(t) {
		while (rcu_gp_ongoing(t) &&
		       ((per_thread(rcu_reader_gp, t) - rcu_gp_ctr) < 0)) {
			/*@@@ poll(NULL, 0, 10); */
			barrier();
		}
	}

	/* Let other synchronize_rcu() instances move ahead. */

	spin_unlock(&rcu_gp_lock);

	/* Ensure that any subsequent free()s happen -after- above checks. */

	smp_mb();
}

#ifdef TEST
#define RCU_READ_NESTABLE
#include "rcutorture.h"
#endif /* #ifdef TEST */
