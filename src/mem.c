
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct
{
	uint32_t proc; // ID of process currently uses this page
	int index;	   // Index of the page in the list of pages allocated
				   // to the process.
	int next;	   // The next page in the list. -1 if it is the last
				   // page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;
static pthread_mutex_t ram_lock;

void init_mem(void)
{
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr)
{
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr)
{
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr)
{
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t *get_page_table(
	addr_t index, // Segment level index
	struct seg_table_t *seg_table)
{ // first level table

	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < seg_table->size; i++)
	{
		// Enter your code here
		if (index == seg_table->table[i].v_index)
			return seg_table->table[i].pages;
	}
	return NULL;
}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
	addr_t virtual_addr,   // Given virtual address
	addr_t *physical_addr, // Physical address to be returned
	struct pcb_t *proc)
{ // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);

	/* Search in the first level */
	struct page_table_t *page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL)
	{
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++)
	{
		if (page_table->table[i].v_index == second_lv)
		{
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			*physical_addr = offset + (page_table->table[i].p_index << OFFSET_LEN);
			return 1;
		}
	}
	return 0;
}

addr_t alloc_mem(uint32_t size, struct pcb_t *proc)
{
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0;																   // We could allocate new memory region or not?
	int free_page = 0;
	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
			free_page++;
	}
	if (free_page >= num_pages && (proc->bp + num_pages * PAGE_SIZE <= RAM_SIZE))
		mem_avail = 1;
	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	if (mem_avail == 1)
	{
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		//printf("This is the break pointer || the num_pages || size of process: %d || %d || %d \n", proc->bp, num_pages, b);
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		addr_t vir_addr = ret_mem;
		int current_seg_index = -1;
		int alloc_count = 0;
		int page_index = -1;
		int previous = -1;
		for (int i = 0; i < NUM_PAGES; i++)
		{
			/*check for free mem_stat*/
			if (_mem_stat[i].proc == 0)
			{
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = alloc_count++;
				if (previous != -1)
				{
					_mem_stat[previous].next = i;
				}
				previous = i;

				current_seg_index = get_first_lv(vir_addr);
				int seg_pos = 0;
				int found = 0;
				/*find first level layer*/
				/*if not found however -> increase size by 1 because this is
             * a new segment to the table*/
				for (seg_pos = 0; seg_pos < proc->seg_table->size; seg_pos++)
				{
					if (proc->seg_table->table[seg_pos].v_index == current_seg_index)
					{
						found = 1;
						break;
					}
				}
				/*increasing size*/
				if (found == 0)
					proc->seg_table->size++;
				/*allocate pages if not alrd there*/
				if (proc->seg_table->table[seg_pos].pages == NULL)
				{
					proc->seg_table->table[seg_pos].pages = (struct page_table_t *)malloc(sizeof(struct page_table_t));
					proc->seg_table->table[seg_pos].pages->size = 0;
				}
				/*assigns first level*/
				proc->seg_table->table[seg_pos].v_index = current_seg_index;
				/*assign for page table*/
				page_index = proc->seg_table->table[seg_pos].pages->size++;
				proc->seg_table->table[seg_pos].pages->table[page_index].v_index = get_second_lv(vir_addr);
				proc->seg_table->table[seg_pos].pages->table[page_index].p_index = i;

				vir_addr += PAGE_SIZE;

				if (alloc_count >= num_pages)
				{
					_mem_stat[i].next = -1;
					break;
				}
			}
		}
	}
	/*FOR DEBUG PURPOSES*/
	// printf("---------------ALLOCATION-----------\n");
	// dump();
	// printf("------------------------------------\n");
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t *proc)
{
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);
	addr_t vir_addr = address; // Take the beginning virtual address to start freeing
	addr_t phy_addr;		   // Physical address to calculate the physcial page
	addr_t phy_page;		   // Physical page to modify the _mem_stat
	int i;
	int return_var = 0;
	int n;

	// Check if the address is there and can be free
	int flag = translate(address, &phy_addr, proc);
	if (flag == 1)
	{
		phy_page = phy_addr >> OFFSET_LEN;
		//printf("The address and the bp of the process: %d || %d\n", phy_page, proc->bp);
		addr_t seg_idx; // Segment index of the segment table
		while (phy_page != -1)
		{
			_mem_stat[phy_page].proc = 0;

			seg_idx = get_first_lv(vir_addr);
			// update bp
			//proc->bp = proc->bp - PAGE_SIZE;
			struct page_table_t *page_table = NULL;
			page_table = get_page_table(seg_idx, proc->seg_table);
			if (page_table != NULL)
			{
				for (i = 0; i < page_table->size; i++)
				{
					if (page_table->table[i].p_index == phy_page)
					{
						n = --page_table->size;
						if (i != n)
						{
							page_table->table[i].v_index = page_table->table[n].v_index;
							page_table->table[i].p_index = page_table->table[n].p_index;
						}
						break;
					}
				}
				if (page_table->size == 0)
				{
					if (page_table != NULL)
					{
						free(page_table);
						int k;
						for (k = 0; k < proc->seg_table->size; k++)
						{
							// Enter your code here
							if (proc->seg_table->table[k].v_index == seg_idx)
							{
								proc->seg_table->table[k].pages = NULL;
								break;
							}
						}
						if (k < proc->seg_table->size)
						{
							int j = k;
							for (j = k; j < proc->seg_table->size - 1; j++)
							{
								proc->seg_table->table[j] = proc->seg_table->table[j + 1];
							}
							proc->seg_table->size--;
						}
					}
				}
			}

			vir_addr += PAGE_SIZE;

			phy_page = _mem_stat[phy_page].next;
		}
		return_var = 1;
	}
	// debug and check the result after deleting
	// printf("---------------DEALLOCATION-----------\n");
	// dump();
	// printf("--------------------------------------\n");
	pthread_mutex_unlock(&mem_lock);
	return return_var;
}

int read_mem(addr_t address, struct pcb_t *proc, BYTE *data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		*data = _ram[physical_addr];
		return 0;
	}
	else
	{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t *proc, BYTE data)
{
	pthread_mutex_lock(&ram_lock);
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		_ram[physical_addr] = data;
		pthread_mutex_unlock(&ram_lock);
		return 0;
	}
	else
	{
		pthread_mutex_unlock(&ram_lock);
		return 1;
	}
}

void dump(void)
{
	int i;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc != 0)
		{
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				   i << OFFSET_LEN,
				   ((i + 1) << OFFSET_LEN) - 1,
				   _mem_stat[i].proc,
				   _mem_stat[i].index,
				   _mem_stat[i].next);
			int j;
			for (j = i << OFFSET_LEN;
				 j < ((i + 1) << OFFSET_LEN) - 1;
				 j++)
			{

				if (_ram[j] != 0)
				{
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
			}
		}
	}
}
