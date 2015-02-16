// simkernel.c - kernel for CA simulation using shared memory


#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int nrows;
  int ncols;
  int *state;
  int *next_state;
} ca;

void supdate(int *sd, int *ns, int nrows, int ncols)
{
  int h9[9][2] = { {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 0}, {0, 1}, {1, -1}, {1, 0}, {1, 1} };
  int ur[10] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 1};
  int real_nrows = nrows + 2;
  int real_ncols = ncols + 2;
  int *state_data = sd;
  int *next_state = ns;

  int ro, co, r, c, nsum, i;

  //#pragma omp parallel for private(c, nsum, ro, co, i) schedule(guided, 10)
  for(r=0; r<nrows; r++)
    for(c=0; c<ncols; c++) {
      nsum = 0;
      for (i=0; i<9; i++) {
        ro = h9[i][0];
        co = h9[i][1];
        nsum += state_data[(1 + c + co) + (1 + r + ro) * real_ncols];
      }
      next_state[(1 + c) + (1 + r) * real_ncols] = ur[nsum];
    }
}

int ca_get_mem_size(ca this)
{
  /* Calculate CA memory size. We allow an extra row and column surrounding the
     CA block for boundary wrapping/edge communication */

  int size = sizeof(int) * (this.nrows + 2) * (this.ncols + 2);

  return size;
}

void ca_init_random(ca this)
{
  /* Initialise each element of the CA's state array to either 1 or 0 */
  int r, c;

  for ( r=0; r<this.nrows; r++ ) {
    for ( c=0; c<this.ncols; c++ ) {
      this.state[ (c+1) + (r+1) * (this.ncols + 2) ] = rand()%2;
    }
  }
}


int create_and_open_shm(char* shm_name)
{
  /* Open the named shared memory area, creating it if it does not exist */

  int shm = shm_open(shm_name, O_CREAT|O_RDWR, 0666);

  if ( shm == -1 ) {
    fprintf(stderr, "Failed to create/open shared memory\n");
    exit(1);
  }

  return shm;
}


int main()
{
  int shm = create_and_open_shm("/cashm");

  ca ca_vote = {1000, 1000, NULL, NULL};
  int ca_size = ca_get_mem_size(ca_vote);

  ftruncate(shm, ca_size);

  ca_vote.state = (int*) mmap(NULL, ca_size, 
			      PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);

  ca_vote.next_state = (int*) malloc(ca_size);

  close(shm);

  ca_init_random(ca_vote);

  int *tmp;

  while (1) {
    supdate(ca_vote.state, ca_vote.next_state, ca_vote.nrows, ca_vote.ncols);
    tmp = ca_vote.state;
    ca_vote.state = ca_vote.next_state;
    ca_vote.next_state = tmp;
  }

  return 0;
}
