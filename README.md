# line_counter
This is an experiment for developing multithreaded solutions for the simple parsing of input files.
As parsing is often quite complicated, this demonstration simply counts the number of non-empty lines.
Parallel processing does not change the "Big O" time complexity of this task, however it does significantly improve the time needed.
"wctest" and "wctest.c" have simple and naive implementations of this task. it provides a comparison to show how much quicker a multithreaded solution is.
"wcmulti" and "wcmulti.c" demonstrates attempts at a multithreaded solution. Even the first implementation is far quicker when handling large input files. As time required increases with size of input, these multithreaded algorithms will eventually overtake single threaded implementations despite the overhead. 
"lc_f" and "lc_final.c" is a possible solution to this question. Switching has a cost but it is small in comparison to the performance gains of a multithreaded solution when the input file is large. The parameters can be adjusted to suit the hardware used.
"lc_test" and "lc_test.c" allow for testing the speed of lc_f and optimizing for hardware.
"generator.c" provides a simple means to create random files to test on. It can easily be adjusted to create files with more blank lines or other situations to test the validity of the implementations. 