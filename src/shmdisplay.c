#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <stdio.h>
#include <stdlib.h>

Uint32 WHITE = 255 + (255 << 8) + (255 << 16);

typedef struct {
  int nrows;
  int ncols;
  int *state;
  int *next_state;
} ca;

int ca_get_mem_size(ca this)
{
  /* Calculate CA memory size. We allow an extra row and column surrounding the
     CA block for boundary wrapping/edge communication */

  int size = sizeof(int) * (this.nrows + 2) * (this.ncols + 2);

  return size;
}

void dump_surface_info(SDL_Surface *surface)
{   
    SDL_PixelFormat* fmt;

    fmt = surface->format;

    printf("%d\n", fmt->BitsPerPixel);
}

void update_pixels(Uint32* my_pixels, int* sd, int nrows, int ncols)
{
    int r, c, x, y;

     for (r=0; r<nrows; r++)
        for (c=0; c<ncols; c++) {
                my_pixels[c + r * ncols] = WHITE * sd[(c+1) + (r+1) * (ncols + 2)];
        }   
}

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

int main(int argc, char *argv[])
{
    SDL_Window *window;
    int xdim = 1000, ydim = 1000;
    int nrows = xdim;
    int ncols = ydim;
    int r, c;
    int x, y;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL couldn't initialise: %s.\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
            "Image",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            xdim,
            ydim,
            SDL_WINDOW_BORDERLESS);

    int data_size = sizeof(int) * (xdim + 2) * (ydim + 2);
    //int* sd = (int*) malloc(data_size);

    int shm = shm_open("/cashm", O_RDONLY, 0666);

    if (shm == -1) {
        fprintf(stderr, "Failed to open existing shared memory\n");
        exit(2);
    }
    ca ca_vote = {1000, 1000, NULL, NULL};
    int ca_size = ca_get_mem_size(ca_vote);
    int* sd = (int*) mmap(NULL, ca_size, PROT_READ, MAP_SHARED, shm, 0);
    close(shm);
 
    
    int* ns = (int*) malloc(data_size);
    int* tmp;
    //memset(sd, 0, data_size);
    /* for (r=1; r<nrows; r++) */
    /*     for (c=1; c<ncols; c++) { */
    /*         sd[c + r * ncols] = rand() % 2; */
    /*     } */

    Uint32 *my_pixels = (Uint32*) malloc(sizeof(Uint32) * xdim * ydim);
    memset(my_pixels, 0, sizeof(Uint32) * xdim * ydim);

    update_pixels(my_pixels, sd, nrows, ncols);


    SDL_Renderer *renderer;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if ( renderer == NULL ) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Texture *sdlTexture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               xdim, ydim);

    int done = 0;
    SDL_Event e;
    int t;
    int frame = 0;
    Uint32 ticks;

    while ( !done ) {
      frame++;
        ticks = SDL_GetTicks();

        while ( SDL_PollEvent( &e ) != 0 ) {
            if ( e.type == SDL_QUIT ) {
                done = 1;
            }

            if ( e.type == SDL_KEYDOWN ) {
                done = 1;
            }
        }

        update_pixels(my_pixels, sd, nrows, ncols);
        SDL_UpdateTexture(sdlTexture, NULL, my_pixels, ydim * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
    // SDL_RenderDrawPoint(renderer, 400, 300);
        SDL_RenderPresent(renderer);

        //supdate(sd, ns, nrows, ncols);
        /* tmp = ns; */
        /* ns = sd; */
        /* sd = tmp; */
        SDL_Delay(10);
	/* if ( frame%50 == 0 ) { */
	/*   printf("%d fps\n", (1000 * frame) / ticks); */
	/* } */
    }

    free(my_pixels);

    return 0;
}
