# hotel optimizer

To run the program:
Create an amalgamated source and header for the library using the instructions on the website. To do this, you'll need to have JsonCpp downloaded and

1) Download or clone this repository,
 
2) Ensure you have Python 2.6 or later installed. From the top level directory of JsonCpp folder, run python amalgamate.py,

3) Include the source file jsoncpp/dist/jsoncpp.cpp in your project's make file or build system.


Here is how I built mine in terminal
`g++ -std=c++0x -O2 jsoncpp/dist/jsoncpp.cpp  hotelapp.cpp -o hotelapp.out -Wall`
