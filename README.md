# Binary-Tree-of-Processes

The goal of this project is to search for a certain pattern in a file, which is in binary format, using multiple processes.

The image ExampleTree.png shows an example of the hierarchy of processes that is created by the program.  

A segment of the file is given to each leaf node. Each leaf node searches for the pattern in its segment, and then returns the records that contain the pattern to its parent node.
The splitter/merger nodes are between the root and the leaf nodes. Their main purpose is to merge the records that they receive from their child processes and send them to their parent process. The root node, collects all the records from its child processes and sorts them.  

As a result, the records flow from the leaf nodes, to the root. Additionaly, each node passes some statistics to the root.  

The communication and data flow between the nodes is carried out by unnamed pipes.

Compilation:
make

Execution:
./myfind -h Height –d Datafile -p Pattern -s  
,where Height is an integer between 2 and 5, Datafile the binary file which will be searched and Pattern a string.
The -s argument is optional. It implies that the segments that the file is divided into, are of different sizes.  
e.x.: ./myfind -h 3 -d Records1000.bin -p ae -s

--------------
This project was an assignments in the Operating Systems course in 2018.
