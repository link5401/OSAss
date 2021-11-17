#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */
	int index = 0;
	/*finding the right place for new proc*/
	while(index != q->size
		&& q->proc[index]->priority > proc->priority) index++;
	/*shift right 1 slot to make place to new proc if proc is not
	 * in the end of the arry*/
	if(index != q->size){
		for(int i = q->size;i != index; i--)
			q->proc[i] = q->proc[i-1];
	}
	/*add proc, increase size*/
	q->proc[index] = proc;
	q->size++;

}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	struct pcb_t * head = NULL;
	if(q->size > 0){
		/*saving head*/
		head = q->proc[0];
		/*shift left the array by 1*/
		for(int i = 0; i < q->size - 1; i++) q->proc[i] = q->proc[i+1];
		/*delete the last index, decreases the size*/
		q->proc[q->size - 1] = NULL;
		free(q->proc[q->size - 1]);
		q->size--;
	}
	return head;

	
}

