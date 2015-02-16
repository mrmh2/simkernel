# Simkernel

Proof of concept code to demonstrate the separation of simulation and display logic. The code consists of two components:

1. A simulation kernel that initialises shared memory and simulates a simple cellular automata using that memory.

2. A display based on SDL2 that reads the shared memory and displays the results.

To compile:

    cd src
    make

To start the simulation kernel:

    ./simkernel

and the display:

    ./shmdisplay
