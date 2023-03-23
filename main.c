#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUFF 100
FILE *resp;

typedef struct process {
  pid_t pid;
  char name[100];
  int priority;
  int startTime;
  int totalTime;
  int burst;
} Process;

typedef struct _queue {
  Process *process_node;
  struct _queue *process_nxt;
} queue;

int cmpfunc(const void *a, const void *b) {
  Process *pa = (Process *)a;
  Process *pb = (Process *)b;
  return (pa->startTime - pb->startTime);
}

queue *ready();
queue *creating(Process *arr, int c);
Process create_process(Process process, char);
queue *node_remover(queue *r_queue);
queue *node_add(queue *r_queue, Process *p);
void schedule(queue *r_queue, queue *c_queue);

int main(int argc, char *argv[]) {
  int c = 0;
  FILE *arq;
  arq = fopen(argv[1], "r");
  resp = fopen("resposta.txt", "w");
  assert(arq);
  Process processArray[BUFF];
  while (fscanf(arq, "exec %[a-zA-Z0-9]", processArray[c].name) == 1) {
    assert(fscanf(arq, ", prioridade=%d", &processArray[c].priority) == 1);
    assert(fscanf(arq, ", inicio_tempo_execucao=%d",
                  &processArray[c].startTime) == 1);
    assert(fscanf(arq, ", tempo_total_ execucao =%d\n",
                  &processArray[c].totalTime) == 1);
    processArray[c].burst = processArray[c].totalTime;
    processArray[c].pid = -1;
    c++;
  }
  schedule(ready(), creating(processArray, c));
  fclose(arq);
  fclose(resp);
  return 0;
}

queue *ready() { return NULL; }
queue *creating(Process *arr, int c) {
  queue *raiz;
  raiz = (queue *)malloc(sizeof(queue));
  queue *aux = raiz;
  qsort(arr, c, sizeof(Process), cmpfunc);
  for (int i = 0; i < c; i++) {
    aux->process_node = &arr[i];
    if (i != c - 1) {
      queue *novo;
      novo = (queue *)malloc(sizeof(queue));
      aux->process_nxt = novo;
      novo->process_nxt = NULL;
      aux = novo;
    }
  }
  return raiz;
}

void schedule(queue *r_queue, queue *c_queue) {
  int time = 0;
  int id;
  Process *aux = NULL;
  int ts = 3;
  while (r_queue || c_queue || aux) {
    while (c_queue && c_queue->process_node->startTime == time) {
      r_queue = node_add(r_queue, c_queue->process_node);
      c_queue = node_remover(c_queue);
    }
    if (r_queue && aux == NULL) {
      aux = r_queue->process_node;
      if (aux->pid == -1) {
        if ((id = fork()) == 0) {
          execl(aux->name, aux->name, NULL);
        }
        aux->pid = id;
      } else {
        kill(aux->pid, SIGCONT);
      }
      r_queue = node_remover(r_queue);
      ts = 3;
    } else if (r_queue && aux &&
               (aux->priority > r_queue->process_node->priority)) {
      r_queue = node_add(r_queue, aux);
      kill(aux->pid, SIGSTOP);
      aux = r_queue->process_node;
      if (aux->pid == -1) {
        if ((id = fork()) == 0) {
          execl(aux->name, aux->name, NULL);
        }
        aux->pid = id;
      } else {
        kill(aux->pid, SIGCONT);
      }
      r_queue = node_remover(r_queue);
      ts = 3;
    } else if (r_queue && aux &&
               (ts <= 0 &&
                (aux->priority == r_queue->process_node->priority))) {
      r_queue = node_add(r_queue, aux);
      kill(aux->pid, SIGSTOP);
      aux = r_queue->process_node;
      if (aux->pid == -1) {
        if ((id = fork()) == 0) {
          execl(aux->name, aux->name, NULL);
        }
        aux->pid = id;
      } else {
        kill(aux->pid, SIGCONT);
      }
      r_queue = node_remover(r_queue);
      ts = 3;
    }
    fprintf(resp, "tempo = %d\n", time);
    if (aux)
      fprintf(resp, "executando = %s\n", aux->name);
    else
      fprintf(resp, "nenhum programa executando\n");
    queue *auxq = r_queue;
    fprintf(resp, "fila de pronto = ");
    while (auxq != NULL) {
      fprintf(resp, " - %s", auxq->process_node->name);
      auxq = auxq->process_nxt;
    }
    fprintf(resp, "\n");
    queue *auxq1 = c_queue;
    fprintf(resp, "fila de criacao = ");
    while (auxq1 != NULL) {
      fprintf(resp, " - %s", auxq1->process_node->name);
      auxq1 = auxq1->process_nxt;
    }
    fprintf(resp, "\n\n");
    if (aux) {
      aux->burst--;
      if (aux->burst == 0) {
        kill(aux->pid, SIGKILL);
        aux = NULL;
      }
    }
    time++;
    ts--;
    sleep(1);
  }
  fprintf(resp, "Todos os processos foram executados com sucesso!");
}

queue *node_add(queue *r_queue, Process *p) {
  queue *novo;
  novo = (queue *)malloc(sizeof(queue));
  novo->process_node = p;
  queue *current = r_queue;
  if (r_queue == NULL || r_queue->process_node->priority > p->priority) {
    novo->process_node = p;
    novo->process_nxt = r_queue;
    return novo;
  }
  while (current->process_nxt != NULL &&
         current->process_nxt->process_node->priority <= p->priority) {
    current = current->process_nxt;
  }
  novo->process_nxt = current->process_nxt;
  current->process_nxt = novo;
  return r_queue;
}

queue *node_remover(queue *r_queue) {
  queue *ret = r_queue->process_nxt;
  free(r_queue);
  return ret;
}