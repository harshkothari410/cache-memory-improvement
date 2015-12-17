/* sim-cache.c - sample cache simulator implementation */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "cache.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "sim.h"

/*
 * This file implements a functional cache simulator.  Cache statistics are
 * generated for a user-selected cache and TLB configuration, which may include
 * up to two levels of instruction and data cache (with any levels unified),
 * and one level of instruction and data TLBs.  No timing information is
 * generated (hence the distinction, "functional" simulator).
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of insn and refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* level 1 instruction cache, entry level instruction cache */
static struct cache_t *cache_il1 = NULL;

/* level 1 instruction cache */
static struct cache_t *cache_il2 = NULL;

/* level 1 data cache, entry level data cache */
static struct cache_t *cache_dl1 = NULL;

/* level 2 data cache */
static struct cache_t *cache_dl2 = NULL;


// vicitm cache
/* victim cache for instruction */
static struct cache_t *victim_il1 = NULL;

/* victim cache for data */
static struct cache_t *victim_dl1 = NULL;
// victim cache end

// miss cache 
/* miss cache for instruction */
static struct cache_t *miss_il1 = NULL;

/* miss cache for data */
static struct cache_t *miss_dl1 = NULL;

// miss cache end

/* selective cache */
static struct cache_t *selective_dl1 = NULL;

static struct cache_t *selective_il1 = NULL;
/* selective cache end */

/* instruction TLB */
static struct cache_t *itlb = NULL;

/* data TLB */
static struct cache_t *dtlb = NULL;

/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
static struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
static counter_t pcstat_lastvals[MAX_PCSTAT_VARS];
static struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];

/* wedge all stat values into a counter_t */
#define STATVAL(STAT)							\
  ((STAT)->sc == sc_int							\
   ? (counter_t)*((STAT)->variant.for_int.var)				\
   : ((STAT)->sc == sc_uint						\
      ? (counter_t)*((STAT)->variant.for_uint.var)			\
      : ((STAT)->sc == sc_counter					\
	 ? *((STAT)->variant.for_counter.var)				\
	 : (panic("bad stat class"), 0))))

/* l1 data cache l1 block miss handler function */
static unsigned int			/* latency of block access */
dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */{

	// victim cache

  	md_addr_t *repl_addr = (md_addr_t *)malloc(sizeof(md_addr_t));

  	if (selective_dl1){
  		if (cache_dl1->last_blk_addr != 0){
  			if (cache_probe(selective_dl1, baddr) != 0){


  				int h1, h2, s1, s2;
  				// h1 = get_hit(  )

  				s1 = get_sticky(cache_dl1, cache_dl1->last_blk_addr );
  				
  				cache_dl1->misses--;
				cache_dl1->hits++;
				cache_dl1->sel_hits++;

  				// if sticky of l1 block address 0
  				if( !s1 ){

  					// flush for place in selective
  					cache_flush_addr(selective_dl1, baddr, now); 

  					// place block to selective cache from l1
  					cache_access(selective_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  					// set sticky of replaced block to 1
  					set_sticky(cache_dl1, baddr, 1);

  					// set hit of replaced block to 1
  					set_hit(cache_dl2, baddr, 1);
  				}
  				else{

  					// get hit value address
  					h1 = get_hit( cache_dl2, baddr );
  					
  					// hit value of victim hit 1 and sticky of flushed block l1 1
  					if(h1){

  						// flush for place in selective
  						cache_flush_addr(selective_dl1, baddr, now);

  						// place that replaced block to selective from l1
  						cache_access(selective_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  						// stickey 1 for replaced block
  						set_sticky(cache_dl1, baddr, 1);

  						// set hit 0 in l2 for replaced block
  						set_hit(cache_dl2, baddr, 0);
  					}
  					else{

  						// bring back replaced block from l2
  						cache_access(cache_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  						// set sticky 0 to replaced block
  						set_sticky(cache_dl1, cache_dl1->last_blk_addr, 0);
  					}
  				}
  			}
  			else{

				cache_dl1->sel_misses++;
  				int h1, h2, s1, s2;
  				s1 = get_sticky(cache_dl1, cache_dl1->last_blk_addr );
  				if( !s1 ){
  					cache_flush_addr(selective_dl1, baddr, now); 
  					cache_access(selective_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  					set_sticky(cache_dl1, baddr, 1);
  					set_hit(cache_dl2, baddr, 1);
  				}
  				else{
  					h1 = get_hit( cache_dl2, baddr );
  					// set_hit( cache_dl2, addr, 0 );
  					if(h1){
  						cache_flush_addr(selective_dl1, baddr, now);
  						cache_access(selective_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  						set_sticky(cache_dl1, baddr, 1);
  						set_hit(cache_dl2, baddr, 0);
  					}
  					else{
  						cache_access(cache_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  						// cache_access(selective_dl1, cmd, baddr, NULL, bsize, now, NULL, repl_addr);
  						cache_access(selective_dl1, cmd, baddr, NULL, bsize, now, NULL, NULL);
  						set_sticky(cache_dl1, baddr, 0);
  					}
  				}
  			}
  		}
  		// else{
  		// 	set_sticky(cache_dl1, baddr, 1);
   	// 		set_hit(cache_dl2, baddr, 1);
  		// }
  	}

	if (victim_dl1){

		if (cache_dl1->last_blk_addr != 0){

  			// printf("%d\n",cache_dl1->last_blk_addr );
			if (cache_probe(victim_dl1, baddr) != 0){ // Hit in VC
	
				cache_dl1->misses--;
				cache_dl1->hits++;
				cache_dl1->vc_hits++;

				// time_t t;
				// srand((unsigned) time(&t));
				// int a = rand() % 2;

				cache_flush_addr(victim_dl1, baddr, now); // Swap this "hit" with the replacement
				cache_access(victim_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and return
	
				return 1;
			} 
			else {
				cache_dl1->vc_misses++;
				cache_access(victim_dl1, 0, cache_dl1->last_blk_addr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and continue
			}
		}
	}

    // miss cache
	if (miss_dl1){

		if (cache_dl1->last_blk_addr != 0){

  			// printf("%d\n",cache_dl1->last_blk_addr );
			if (cache_probe(miss_dl1, baddr) != 0){ // Hit in VC
	
				cache_dl1->misses--;
				cache_dl1->hits++;
				cache_dl1->mc_hits++;
				

				// cache_flush_addr(victim_dl1, baddr, now); // Swap this "hit" with the replacement
				cache_access(miss_dl1, 0, baddr, NULL, bsize, now, NULL, NULL); // Add replaced block to miss cache and return
				return 1;
			} 
			else {
				cache_dl1->mc_misses++;
				cache_access(miss_dl1, 0, baddr, NULL, bsize, now, NULL, NULL); // Add replaced block to miss cache and continue
			}
		}
	}

	if (cache_dl2){
 	    // access next level cache 
		// return cache_access(cache_dl2, cmd, baddr, NULL, bsize, /* now */now, /* pudata */NULL, /* replace addr */repl_addr);
		int j = 0;
        j = cache_access(cache_dl2, cmd, baddr, NULL, bsize, /* now */now, /* pudata */NULL, /* replace addr */NULL);
        //if (j != 1) printf("CAT: %d", j);
        //set_hit(cache_dl2, baddr, 1);
        // printf("%s\n", "hello shaktiman");
        return j;
    }
	else{
		/* access main memory, which is always done in the main simulator loop */
		return /* access latency, ignored */1;
    }
}


// vicitim cache handler function
static unsigned int			/* latency of block access */
victim_dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}


static unsigned int			/* latency of block access */
victim_il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}
// victim cache handler end

// miss cache handler 
miss_dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}


static unsigned int			/* latency of block access */
miss_il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}

// miss cache handler end


// selective cache handler
static unsigned int			/* latency of block access */
selective_dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}

static unsigned int			/* latency of block access */
selective_il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
              md_addr_t baddr,		/* block address to access */
              int bsize,		/* size of block to access */
              struct cache_blk_t *blk,	/* ptr to block in upper level */
              tick_t now){		/* time of access */
              return 1;
}
// selective cache handler end

/* l2 data cache block miss handler function */
static unsigned int			/* latency of block access */
dl2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory, which is
     always done in the main simulator loop */
  return /* access latency, ignored */100;
}

/* l1 inst cache l1 block miss handler function */
static unsigned int			/* latency of block access */
il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
	 	md_addr_t *repl_addr = (md_addr_t *)malloc(sizeof(md_addr_t));

  	if (selective_il1){
  		if (cache_il1->last_blk_addr != 0){
  			if (cache_probe(selective_il1, baddr) != 0){


  				int h1, h2, s1, s2;
  				// h1 = get_hit(  )

  				s1 = get_sticky(cache_il1, cache_il1->last_blk_addr );
  				
  				cache_il1->misses--;
				cache_il1->hits++;
				cache_il1->sel_hits++;

  				// if sticky of l1 block address 0
  				if( s1 == 0 ){

  					// flush for place in selective
  					cache_flush_addr(selective_il1, baddr, now); 

  					// place block to selective cache from l1
  					cache_access(selective_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  					// set sticky of replaced block to 1
  					set_sticky(cache_il1, baddr, 1);

  					// set hit of replaced block to 1
  					set_hit(cache_dl2, baddr, 1);
  				}
  				else{

  					// get hit value address
  					h1 = get_hit( cache_dl2, baddr );
  					
  					// hit value of victim hit 1 and sticky of flushed block l1 1
  					if(h1){

  						// flush for place in selective
  						cache_flush_addr(selective_il1, baddr, now);

  						// place that replaced block to selective from l1
  						cache_access(selective_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  						// stickey 1 for replaced block
  						set_sticky(cache_il1, baddr, 1);

  						// set hit 0 in l2 for replaced block
  						set_hit(cache_dl2, baddr, 0);
  					}
  					else{

  						// bring back replaced block from l2
  						cache_access(cache_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);

  						// set sticky 0 to replaced block
  						set_sticky(cache_il1, cache_il1->last_blk_addr, 0);
  					}
  				}
  			}
  			else{

				cache_il1->sel_misses++;
  				int h1, h2, s1, s2;
  				s1 = get_sticky(cache_il1, cache_il1->last_blk_addr );
  				if( s1 == 0 ){
  					cache_flush_addr(selective_il1, baddr, now); 
  					cache_access(selective_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  					set_sticky(cache_il1, baddr, 1);
  					set_hit(cache_dl2, baddr, 1);
  				}
  				else{
  					h1 = get_hit( cache_dl2, baddr );
  					// set_hit( cache_dl2, addr, 0 );
  					if(h1){
  						cache_flush_addr(selective_il1, baddr, now);
  						cache_access(selective_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  						set_sticky(cache_il1, baddr, 1);
  						set_hit(cache_dl2, baddr, 0);
  					}
  					else{
  						cache_access(cache_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL);
  						cache_access(selective_il1, cmd, baddr, NULL, bsize, now, NULL, NULL);
  						set_sticky(cache_il1, baddr, 0);
  					}
  				}
  			}
  		}
  		// else{
  		// 	set_sticky(cache_il1, baddr, 1);
   	// 		set_hit(cache_dl2, baddr, 1);
  		// }
  	}

	if (victim_il1){

		if (cache_il1->last_blk_addr != 0){

  			// printf("%d\n",cache_il1->last_blk_addr );
			if (cache_probe(victim_il1, baddr) != 0){ // Hit in VC
	
				cache_il1->misses--;
				cache_il1->hits++;
				cache_il1->vc_hits++;

				// time_t t;
				// srand((unsigned) time(&t));
				// int a = rand() % 2;

				cache_flush_addr(victim_il1, baddr, now); // Swap this "hit" with the replacement
				cache_access(victim_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and return
	
				return 1;
			} 
			else {
				cache_il1->vc_misses++;
				cache_access(victim_il1, 0, cache_il1->last_blk_addr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and continue
			}
		}
	}

    // miss cache
	if (miss_il1){

		if (cache_il1->last_blk_addr != 0){

  			// printf("%d\n",cache_il1->last_blk_addr );
			if (cache_probe(miss_il1, baddr) != 0){ // Hit in VC
	
				cache_il1->misses--;
				cache_il1->hits++;
				cache_il1->mc_hits++;
				

				// cache_flush_addr(victim_il1, baddr, now); // Swap this "hit" with the replacement
				cache_access(miss_il1, 0, baddr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and return
				return 1;
			} 
			else {
				cache_il1->mc_misses++;
				cache_access(miss_il1, 0, baddr, NULL, bsize, now, NULL, NULL); // Add replaced block to victim cache and continue
			}
		}
	}


	if (cache_il2){
		/* access next level of inst cache hierarchy */
		return cache_access(cache_il2, cmd, baddr, NULL, bsize,
		/* now */now, /* pudata */NULL, /* repl addr */NULL);
    }
	else{
		/* access main memory, which is always done in the main simulator loop */
		return /* access latency, ignored */1;
	}
}



/* l2 inst cache block miss handler function */
static unsigned int			/* latency of block access */
il2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      md_addr_t baddr,		/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk_t *blk,	/* ptr to block in upper level */
	      tick_t now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory, which is
     always done in the main simulator loop */
  return /* access latency, ignored */1;
}

/* inst cache block miss handler function */
static unsigned int			/* latency of block access */
itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,	/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now)		/* time of access */
{
	md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

	/* no real memory access, however, should have user data space attached */
	assert(phy_page_ptr);

	/* fake translation, for now... */
	*phy_page_ptr = 0;

	return /* access latency, ignored */1;
}

/* data cache block miss handler function */
static unsigned int			/* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       md_addr_t baddr,		/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk_t *blk,	/* ptr to block in upper level */
	       tick_t now)		/* time of access */
{
  md_addr_t *phy_page_ptr = (md_addr_t *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  return /* access latency, ignored */1;
}

/* cache/TLB options */
static char *cache_dl1_opt /* = "none" */;
static char *cache_dl2_opt /* = "none" */;
static char *cache_il1_opt /* = "none" */;
static char *cache_il2_opt /* = "none" */;

// victim cache options
static char *victim_dl1_opt /* = "none" */;
static char *victim_il1_opt /* = "none" */;
// vicitim cache end

// miss cache
static char *miss_dl1_opt /* = "none" */;
static char *miss_il1_opt /* = "none" */;
// miss cache end

// selective cache
static char *selective_dl1_opt;
static char *selective_il1_opt;
// selective cache end

static char *itlb_opt /* = "none" */;
static char *dtlb_opt /* = "none" */;
static int flush_on_syscalls /* = FALSE */;
static int compress_icache_addrs /* = FALSE */;

/* text-based stat profiles */
static int pcstat_nelt = 0;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#ifdef TARGET_PISA
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - ld_text_base) >> 1) + ld_text_base) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))
#else /* !TARGET_PISA */
#define IACOMPRESS(A)		(A)
#define ISCOMPRESS(SZ)		(SZ)
#endif /* TARGET_PISA */

/* Registe simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)	/* options database */
{
  opt_reg_header(odb, 
"sim-cache: This simulator implements a functional cache simulator.  Cache\n"
"statistics are generated for a user-selected cache and TLB configuration,\n"
"which may include up to two levels of instruction and data cache (with any\n"
"levels unified), and one level of instruction and data TLBs.  No timing\n"
"information is generated.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-cache:dl1",
		 "l1 data cache config, i.e., {<config>|none}",
		 &cache_dl1_opt, "dl1:256:32:1:l", /* print */TRUE, NULL);
  opt_reg_note(odb,
"  The cache config parameter <config> has the following format:\n"
"\n"
"    <name>:<nsets>:<bsize>:<assoc>:<repl>\n"
"\n"
"    <name>   - name of the cache being defined\n"
"    <nsets>  - number of sets in the cache\n"
"    <bsize>  - block size of the cache\n"
"    <assoc>  - associativity of the cache\n"
"    <repl>   - block replacement strategy, 'l'-LRU, 'f'-FIFO, 'r'-random\n"
"\n"
"    Examples:   -cache:dl1 dl1:4096:32:1:l\n"
"                -dtlb dtlb:128:4096:32:r\n"
	       );
  opt_reg_string(odb, "-cache:dl2",
		 "l2 data cache config, i.e., {<config>|none}",
		 &cache_dl2_opt, "ul2:1024:64:4:l", /* print */TRUE, NULL);
  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
		 &cache_il1_opt, "il1:256:32:1:l", /* print */TRUE, NULL);
  opt_reg_note(odb,
"  Cache levels can be unified by pointing a level of the instruction cache\n"
"  hierarchy at the data cache hiearchy using the \"dl1\" and \"dl2\" cache\n"
"  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
"\n"
"    A unified l2 cache (il2 is pointed at dl2):\n"
"      -cache:il1 il1:128:64:1:l -cache:il2 dl2\n"
"      -cache:dl1 dl1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
"\n"
"    Or, a fully unified cache hierarchy (il1 pointed at dl1):\n"
"      -cache:il1 dl1\n"
"      -cache:dl1 ul1:256:32:1:l -cache:dl2 ul2:1024:64:2:l\n"
"\n"
"    victim cache\n"
"      -cache:dvictim dvictim:8:1\n"
"      -cache:ivictim dvictim:8:1\n"
"\n"
"    miss cache\n"
"      -cache:dmcache dmcache:8:1\n"
"      -cache:imcache dvictim:8:1\n"
"\n"
"    selective cache\n"
"      -cache:dsel dsel:8:1\n"
"      -cache:isel dsel:8:1\n"
	       );
  opt_reg_string(odb, "-cache:il2",
		 "l2 instruction cache config, i.e., {<config>|dl2|none}",
		 &cache_il2_opt, "dl2", /* print */TRUE, NULL);

  // victim cache
  opt_reg_string(odb, "-cache:dvictim",
     "l1 data victim cache config, i.e., {<config>|none}",
     &victim_dl1_opt, "none", /* print */TRUE, NULL);

  opt_reg_string(odb, "-cache:ivictim",
     "l1 instruction victim cache config, i.e., {<config>|none}",
     &victim_il1_opt, "none", /* print */TRUE, NULL);
  // vicitm cache end

  // miss cache
  opt_reg_string(odb, "-cache:dmcache",
     "l1 data miss cache config, i.e., {<config>|none}",
     &miss_dl1_opt, "none", /* print */TRUE, NULL);

  opt_reg_string(odb, "-cache:imcache",
     "l1 instruction miss cache config, i.e., {<config>|none}",
     &miss_il1_opt, "none", /* print */TRUE, NULL);
  // miss cache end

  	// selective cache
     opt_reg_string(odb, "-cache:dsel",
     "l1 data selective cache config, i.e., {<config>|none}",
     &selective_dl1_opt, "none", /* print */TRUE, NULL);

     opt_reg_string(odb, "-cache:isel",
     "l1 instruction selective cache config, i.e., {<config>|none}",
     &selective_il1_opt, "none", /* print */TRUE, NULL);
     // selective cache end

  opt_reg_string(odb, "-tlb:itlb",
		 "instruction TLB config, i.e., {<config>|none}",
		 &itlb_opt, "itlb:16:4096:4:l", /* print */TRUE, NULL);
  opt_reg_string(odb, "-tlb:dtlb",
		 "data TLB config, i.e., {<config>|none}",
		 &dtlb_opt, "dtlb:32:4096:4:l", /* print */TRUE, NULL);
  opt_reg_flag(odb, "-flush", "flush caches on system calls",
	       &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL);
  opt_reg_flag(odb, "-cache:icompress",
	       "convert 64-bit inst addresses to 32-bit inst equivalents",
	       &compress_icache_addrs, /* default */FALSE,
	       /* print */TRUE, NULL);

  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addr's (mult uses ok)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb,	/* options database */
		  int argc, char **argv)	/* command line arguments */
{
  char name[128], c;
  int nsets, bsize, assoc;

  /* use a level 1 D-cache? */
  	if (!mystricmp(cache_dl1_opt, "none")){
    cache_dl1 = NULL;

      /* the level 2 D-cache cannot be defined */
    	if (strcmp(cache_dl2_opt, "none"))
			fatal("the l1 data cache must defined if the l2 cache is defined");
      	cache_dl2 = NULL;
    }
  	else /* dl1 is defined */{
    	if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%c",
			name, &nsets, &bsize, &assoc, &c) != 5)
		
		fatal("bad l1 D-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      	cache_dl1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       dl1_access_fn, /* hit latency */1);

		// victim cache
	    if (!mystricmp(victim_dl1_opt, "none")){
	            victim_dl1 = NULL;
	    } 
	    else{
	    	//sscanf(victim_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  victim_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), victim_dl1_access_fn, /* hit latency */1);
	    	if (sscanf(victim_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad victim D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            victim_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), victim_dl1_access_fn, /* hit latency */1);
	  	}
  		// victim cache end

	  	// miss cache 
	    if (!mystricmp(miss_dl1_opt, "none")){
	            miss_dl1 = NULL;
	    }
	    else{
	    	//sscanf(miss_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  miss_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), miss_dl1_access_fn, /* hit latency */1);
	    	if (sscanf(miss_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad miss D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            miss_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), miss_dl1_access_fn, /* hit latency */1);
	  	}
	  	// miss cache end

	  	// selective cache 
	    if (!mystricmp(selective_dl1_opt, "none")){
	            selective_dl1 = NULL;
	    }
	    else{
	    	//sscanf(selective_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  selective_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), selective_dl1_access_fn, /* hit latency */1);
	    	if (sscanf(selective_dl1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad selective D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            selective_dl1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), selective_dl1_access_fn, /* hit latency */1);
	  	}
	  	// selective cache end


      /* is the level 2 D-cache defined? */
    	if (!mystricmp(cache_dl2_opt, "none"))
			cache_dl2 = NULL;
      	else{
	  		if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c",
		    	name, &nsets, &bsize, &assoc, &c) != 5)
	    	fatal("bad l2 D-cache parms: "
		  		"<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  		cache_dl2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   dl2_access_fn, /* hit latency */1);
		}
    }

  

  /* use a level 1 I-cache? */
  if (!mystricmp(cache_il1_opt, "none"))
    {
      cache_il1 = NULL;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl1"))
    {
      if (!cache_dl1)
	fatal("I-cache l1 cannot access D-cache l1 as it's undefined");
      cache_il1 = cache_dl1;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else if (!mystricmp(cache_il1_opt, "dl2"))
    {
      if (!cache_dl2)
	fatal("I-cache l1 cannot access D-cache l2 as it's undefined");
      cache_il1 = cache_dl2;

      /* the level 2 I-cache cannot be defined */
      if (strcmp(cache_il2_opt, "none"))
	fatal("the l1 inst cache must defined if the l2 cache is defined");
      cache_il2 = NULL;
    }
  else /* il1 is defined */
    {
      if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad l1 I-cache parms: <name>:<nsets>:<bsize>:<assoc>:<repl>");
      cache_il1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			       /* usize */0, assoc, cache_char2policy(c),
			       il1_access_fn, /* hit latency */1);

		// victim cache
	    if (!mystricmp(victim_il1_opt, "none")){
	            victim_il1 = NULL;
	    } 
	    else{
	    	//sscanf(victim_il1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  victim_il1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), victim_il1_access_fn, /* hit latency */1);
	    	if (sscanf(victim_il1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad victim D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            victim_il1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), victim_il1_access_fn, /* hit latency */1);
	  	}
  		// victim cache end

	  	// miss cache 
	    if (!mystricmp(miss_il1_opt, "none")){
	            miss_il1 = NULL;
	    }
	    else{
	    	//sscanf(miss_il1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  miss_il1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), miss_il1_access_fn, /* hit latency */1);
	    	if (sscanf(miss_il1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad miss D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            miss_il1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), miss_il1_access_fn, /* hit latency */1);
	  	}
	  	// miss cache end

	  	// selective cache 
	    if (!mystricmp(selective_il1_opt, "none")){
	            selective_il1 = NULL;
	    }
	    else{
	    	//sscanf(selective_il1_opt, "%[^:]:%d:%c",name, &nsets, &c);
	          //  selective_il1 = cache_create(name, 1, bsize, /* balloc */FALSE,
	            ///* usize */0, nsets, cache_char2policy(c), selective_il1_access_fn, /* hit latency */1);
	    	if (sscanf(selective_il1_opt, "%[^:]:%d:%c",name, &nsets, &c) != 3 )
	    		fatal("bad selective D-cache parms: "
		  		"<name>:<lines>:<repl>");
	            selective_il1 = cache_create(name, 2, bsize, /* balloc */FALSE,
	            /* usize */0, nsets, cache_char2policy(c), selective_il1_access_fn, /* hit latency */1);
	  	}
	  	// selective cache end



      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_il2_opt, "none"))
	cache_il2 = NULL;
      else if (!mystricmp(cache_il2_opt, "dl2"))
	{
	  if (!cache_dl2)
	    fatal("I-cache l2 cannot access D-cache l2 as it's undefined");
	  cache_il2 = cache_dl2;
	}
      else
	{
	  if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c",
		     name, &nsets, &bsize, &assoc, &c) != 5)
	    fatal("bad l2 I-cache parms: "
		  "<name>:<nsets>:<bsize>:<assoc>:<repl>");
	  cache_il2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   il2_access_fn, /* hit latency */1);
	}
    }

  /* use an I-TLB? */
  if (!mystricmp(itlb_opt, "none"))
    itlb = NULL;
  else
    {
      if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      itlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), itlb_access_fn,
			  /* hit latency */1);
    }

  /* use a D-TLB? */
  if (!mystricmp(dtlb_opt, "none"))
    dtlb = NULL;
  else
    {
      if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: <name>:<nsets>:<page_size>:<assoc>:<repl>");
      dtlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(md_addr_t), assoc,
			  cache_char2policy(c), dtlb_access_fn,
			  /* hit latency */1);
    }
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* local machine state accessor */
static char *					/* err str, NULL for no err */
cache_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* register to access */
		 struct mem_t *mem)		/* memory to access */
{
  /* just dump intermediate stats */
  sim_print_stats(stream);

  /* no error */
  return NULL;
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, cache_mstate_obj);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)	/* stats database */
{
  int i;

  /* register baseline stats */
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  /* register cache stats */
  if (cache_il1
      && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
    cache_reg_stats(cache_il1, sdb);
  if (cache_il2
      && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
    cache_reg_stats(cache_il2, sdb);
  if (cache_dl1)
    cache_reg_stats(cache_dl1, sdb);
  if (cache_dl2)
    cache_reg_stats(cache_dl2, sdb);

//	victim cache 
	if (victim_dl1)
    	cache_reg_stats(victim_dl1, sdb);

	if (victim_il1)
  		cache_reg_stats(victim_il1, sdb);
	// victim cache end


  	// miss cache
  	if (miss_dl1)
    	cache_reg_stats(miss_dl1, sdb);

	if (miss_il1)
  		cache_reg_stats(miss_il1, sdb);
  	// miss cache

  	// selective cache
  	if (selective_dl1)
    	cache_reg_stats(selective_dl1, sdb);

	   if (selective_il1)
   		cache_reg_stats(selective_il1, sdb);
  	// selective cache end

  if (itlb)
    cache_reg_stats(itlb, sdb);
  if (dtlb)
    cache_reg_stats(dtlb, sdb);

  for (i=0; i<pcstat_nelt; i++)
    {
      char buf[512], buf1[512];
      struct stat_stat_t *stat;

      /* track the named statistical variable by text address */

      /* find it... */
      stat = stat_find_stat(sdb, pcstat_vars[i]);
      if (!stat)
	fatal("cannot locate any statistic named `%s'", pcstat_vars[i]);

      /* stat must be an integral type */
      if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
	fatal("`-pcstat' statistical variable `%s' is not an integral type",
	      stat->name);

      /* register this stat */
      pcstat_stats[i] = stat;
      pcstat_lastvals[i] = STATVAL(stat);

      /* declare the sparce text distribution */
      sprintf(buf, "%s_by_pc", stat->name);
      sprintf(buf1, "%s (by text address)", stat->desc);
      pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
					/* initial value */0,
					/* print fmt */(PF_COUNT|PF_PDF),
					/* format */"0x%p %u %.2f",
					/* print fn */NULL);
    }
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize the simulator */
void
sim_uninit(void)
{
  /* nada */
}

/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[N])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[N] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define __READ_CACHE(addr, SRC_T)					\
  ((dtlb								\
    ? cache_access(dtlb, Read, (addr), NULL,				\
		   sizeof(SRC_T), 0, NULL, NULL)			\
    : 0),								\
   (cache_dl1								\
    ? cache_access(cache_dl1, Read, (addr), NULL,			\
		   sizeof(SRC_T), 0, NULL, NULL)			\
    : 0))

#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC),				\
   __READ_CACHE(addr, byte_t), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC),				\
   __READ_CACHE(addr, half_t), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC),				\
   __READ_CACHE(addr, word_t), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC),				\
   __READ_CACHE(addr, qword_t), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define __WRITE_CACHE(addr, DST_T)					\
  ((dtlb								\
    ? cache_access(dtlb, Write, (addr), NULL,				\
		   sizeof(DST_T), 0, NULL, NULL)			\
    : 0),								\
   (cache_dl1								\
    ? cache_access(cache_dl1, Write, (addr), NULL,			\
		   sizeof(DST_T), 0, NULL, NULL)			\
    : 0))

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST),				\
   __WRITE_CACHE(addr, byte_t), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST),				\
   __WRITE_CACHE(addr, half_t), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST),				\
   __WRITE_CACHE(addr, word_t), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST),				\
   __WRITE_CACHE(addr, qword_t), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call memory access function */
enum md_fault_type
dcache_access_fn(struct mem_t *mem,	/* memory space to access */
		 enum mem_cmd cmd,	/* memory access cmd, Read or Write */
		 md_addr_t addr,	/* data address to access */
		 void *p,		/* data input/output buffer */
		 int nbytes)		/* number of bytes to access */
{
  if (dtlb)
    cache_access(dtlb, cmd, addr, NULL, nbytes, 0, NULL, NULL);
  if (cache_dl1)
    cache_access(cache_dl1, cmd, addr, NULL, nbytes, 0, NULL, NULL);
  return mem_access(mem, cmd, addr, p, nbytes);
}

/* system call handler macro */
#define SYSCALL(INST)							\
  (flush_on_syscalls							\
   ? ((dtlb ? cache_flush(dtlb, 0) : 0),				\
      (cache_dl1 ? cache_flush(cache_dl1, 0) : 0),			\
      (cache_dl2 ? cache_flush(cache_dl2, 0) : 0),			\
      sys_syscall(&regs, mem_access, mem, INST, TRUE))			\
   : sys_syscall(&regs, dcache_access_fn, mem, INST, TRUE))

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  int i;
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

  fprintf(stderr, "sim: ** starting functional simulation w/ caches **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t), regs.regs_PC,
	       sim_num_insn, &regs, mem);

  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      if (itlb)
	cache_access(itlb, Read, IACOMPRESS(regs.regs_PC),
		     NULL, ISCOMPRESS(sizeof(md_inst_t)), 0, NULL, NULL);
      if (cache_il1)
	cache_access(cache_il1, Read, IACOMPRESS(regs.regs_PC),
		     NULL, ISCOMPRESS(sizeof(md_inst_t)), 0, NULL, NULL);
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
          panic("attempted to execute a bogus opcode");
	}

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* update any stats tracked by PC */
      for (i=0; i < pcstat_nelt; i++)
	{
	  counter_t newval;
	  int delta;

	  /* check if any tracked stats changed */
	  newval = STATVAL(pcstat_stats[i]);
	  delta = newval - pcstat_lastvals[i];
	  if (delta != 0)
	    {
	      stat_add_samples(pcstat_sdists[i], regs.regs_PC, delta);
	      pcstat_lastvals[i] = newval;
	    }

	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
