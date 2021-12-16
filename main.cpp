#include <math.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <chrono>

#include "utility.h"
#include "CL/cl.hpp"

using namespace std;

static const cl_uint vectorSize = 50000; 
//static const cl_uint workSize = 50;
//static const cl_uint dimSize = 2;

int main(void) {

  cl_int err;

  // Setting up Platform Information 
  std::vector<cl::Platform> PlatformList;
  err = cl::Platform::get(&PlatformList);
  checkErr(err, "Get Platform List");
  checkErr(PlatformList.size()>=1 ? CL_SUCCESS : -1, "cl::Platform::get");
  print_platform_info(&PlatformList);
  
  //Setup Device
  std::vector<cl::Device> DeviceList;
  err = PlatformList[0].getDevices(CL_DEVICE_TYPE_ALL, &DeviceList);
  checkErr(err, "Get Devices");
  print_device_info(&DeviceList);

  //Setting up Context
  cl::Context myContext(DeviceList, NULL, NULL, NULL, &err);
  checkErr(err, "Context Constructor");

  //Create Command Queue
  cl::CommandQueue myqueue(myContext, DeviceList[0], CL_QUEUE_PROFILING_ENABLE, &err);
  checkErr(err, "Queue Constructor");

  // Create buffers for input
  cl::Buffer bufX(myContext, CL_MEM_READ_ONLY, vectorSize * sizeof(cl_float));
  cl::Buffer bufY(myContext, CL_MEM_READ_ONLY, vectorSize * sizeof(cl_float));


  //Create buffers for intermediate values
  cl::Buffer bufNeighbCount(myContext, CL_MEM_READ_WRITE, vectorSize * sizeof(cl_uint));
  cl::Buffer bufNeighbIdx(myContext, CL_MEM_READ_WRITE, vectorSize * vectorSize * sizeof(cl_uint));
  cl::Buffer bufStack(myContext, CL_MEM_READ_WRITE, vectorSize * sizeof(cl_uint));
  cl::Buffer bufAddedtoStack(myContext, CL_MEM_READ_WRITE, vectorSize * sizeof(cl_uint));
  cl::Buffer bufLabels(myContext, CL_MEM_READ_WRITE, vectorSize * sizeof(cl_int));
  cl::Buffer bufCounter(myContext, CL_MEM_READ_WRITE, sizeof(cl_uint));
  cl::Buffer bufEnteredLoop(myContext, CL_MEM_READ_WRITE, sizeof(cl_uint));
  cl::Buffer bufVisited(myContext, CL_MEM_READ_WRITE, vectorSize * sizeof(cl_uint));


  cl_float eps = 0.3;
  cl_uint min_samps = 10;
  cl_uint init_value  = 0;
  cl_int noise = -1;

  cl_float X[vectorSize]  __attribute__ ((aligned (64)));
  cl_float Y[vectorSize]  __attribute__ ((aligned (64)));

  cl_uint neighbCount[vectorSize]  __attribute__ ((aligned (64)));
  cl_uint neighbIdx[vectorSize*vectorSize]  __attribute__ ((aligned (64)));
  cl_int labels[vectorSize]  __attribute__ ((aligned (64)));
  cl_uint stack[vectorSize]  __attribute__ ((aligned (64)));
  cl_uint added_to_stack[vectorSize]  __attribute__ ((aligned (64)));
  cl_uint counter __attribute__ ((aligned (64))) = 1;
  cl_uint entered_loop __attribute__ ((aligned (64))) = 0;
  cl_uint visited[vectorSize]  __attribute__ ((aligned (64)));


  //Reading in data for comparison
  //  float **true_labels = new float*[999];
  //for(int i = 0; i < 999; i++) {
  //true_labels[i] = new float[50000];
  //}

  // string inFileName_true = "ClusterData/scikitLabels.txt";
  // ifstream inFile_true;
  // inFile_true.open(inFileName_true.c_str());
  // if (inFile_true.is_open()) {
  //for (int i = 0; i < 999; i++){
  //  for (int j = 0; j < 50000; j++) {
  //	inFile_true >> true_labels[i][j];
	//cout << true_labels[i][j] << " ";
  //  }
  //  }
  //inFile_true.close(); // CLose input file
  // }
  //else { //Error message
  //cerr << "Can't find input file " << inFileName_true << endl;
  //}

  // cout << "READ TRUTH" << endl;



  for(int c = 499; c <= 499; c++) {
    
    // Read input data 
    string inFileName = "ClusterData/blobsC" + to_string(c) + ".txt"; 
    ifstream inFile;
    inFile.open(inFileName.c_str());
    if (inFile.is_open()) {
      for (int i = 0; i < vectorSize; i++){
        labels[i] = -1;
        neighbCount[i] = 0;
        stack[i] = 0;
	added_to_stack[i] = 0;
	visited[i] = 0;
        inFile >> X[i];
        inFile >> Y[i];
        for (int j = 0; j < vectorSize; j++) {
          neighbIdx[i*vectorSize + j] = 0;
        }
      }
      inFile.close(); // Close input file
    }
    else { //Error message
      cerr << "Can't find input file " << inFileName << endl;
    }
    cout << "Read Data" << endl;

    // Write to buffers
    err = myqueue.enqueueWriteBuffer(bufX, CL_TRUE, 0, vectorSize * sizeof(cl_float),X); checkErr(err, "WriteBuffer");
    err = myqueue.enqueueWriteBuffer(bufY, CL_TRUE, 0, vectorSize * sizeof(cl_float),Y); checkErr(err, "WriteBuffer 2");
    err = myqueue.enqueueWriteBuffer(bufNeighbCount, CL_TRUE, 0, vectorSize * sizeof(cl_uint),neighbCount); checkErr(err, "WriteBuffer 3");
    err = myqueue.enqueueWriteBuffer(bufLabels, CL_TRUE, 0, vectorSize * sizeof(cl_int),labels); checkErr(err, "WriteBuffer 5");
    err = myqueue.enqueueWriteBuffer(bufNeighbIdx, CL_TRUE, 0, vectorSize * vectorSize * sizeof(cl_uint),neighbIdx); checkErr(err, "WriteBuffer 6");
    err = myqueue.enqueueWriteBuffer(bufStack, CL_TRUE, 0, vectorSize * sizeof(cl_uint),stack); checkErr(err, "WriteBuffer 7");
    err = myqueue.enqueueWriteBuffer(bufAddedtoStack, CL_TRUE, 0, vectorSize * sizeof(cl_uint),added_to_stack); checkErr(err, "WriteBuffer 8");
    err = myqueue.enqueueWriteBuffer(bufCounter, CL_TRUE, 0, sizeof(cl_uint),&counter); checkErr(err, "WriteBuffer 9");
    err = myqueue.enqueueWriteBuffer(bufEnteredLoop, CL_TRUE, 0, sizeof(cl_uint),&entered_loop); checkErr(err, "WriteBuffer 10");
    err = myqueue.enqueueWriteBuffer(bufVisited, CL_TRUE, 0, vectorSize * sizeof(cl_uint),visited); checkErr(err, "WriteBuffer 11");

    cout << "Wrote to buffers" << endl;

    //Creating kernels
    const char *kernel_name_range = "radius_neighbors";
    const char *kernel_name_label = "label_local";

    // Creating Binaries
    std::ifstream aocx_stream("dbscan.aocx", std::ios::in|std::ios::binary);
    checkErr(aocx_stream.is_open() ? CL_SUCCESS:-1, "dbscan.aocx");
    std::string prog(std::istreambuf_iterator<char>(aocx_stream), (std::istreambuf_iterator<char>()));
    cl::Program::Binaries mybinaries(DeviceList.size(), std::make_pair(prog.c_str(), prog.length()));
    cout << "Created Binaries" << endl;

    // Create the Program from the AOCX file.
    cl::Program program(myContext, DeviceList, mybinaries, NULL, &err);
    checkErr(err, "Program Constructor");
    cout << "Created Program" << endl;


    // Build the program
    err= program.build(DeviceList);
    checkErr(err, "Build Program");

    // create the kernel 
    cl::Kernel kernelNeighbors(program, kernel_name_range, &err);
    checkErr(err, "Kernel Creation");

    cl::Kernel kernelLabel(program, kernel_name_label, &err);
    checkErr(err, "Kernel Creation");

    cout << "Created Kernels" << endl;

    //cl::Event event;
    cl_uint label_num = 0;
    
    err = kernelNeighbors.setArg(0, bufX); checkErr(err, "Arg 0");
    err = kernelNeighbors.setArg(1, bufY);checkErr(err, "Arg 1");
    err = kernelNeighbors.setArg(2, bufNeighbCount); checkErr(err, "Arg 2");
    err = kernelNeighbors.setArg(3, bufNeighbIdx); checkErr(err, "Arg 3");
    err = kernelNeighbors.setArg(4, bufStack); checkErr(err, "Arg 4");
    err = kernelNeighbors.setArg(5, bufVisited); checkErr(err, "Arg 5");
    err = kernelNeighbors.setArg(6, vectorSize); checkErr(err, "Arg 6");
    err = kernelNeighbors.setArg(7, eps); checkErr(err, "Arg 7");

    err = kernelLabel.setArg(0, bufNeighbCount); checkErr(err, "Arg 0");
    err = kernelLabel.setArg(1, bufNeighbIdx); checkErr(err, "Arg 1");
    err = kernelLabel.setArg(2, bufStack); checkErr(err, "Arg 2");
    err = kernelLabel.setArg(3, bufLabels); checkErr(err, "Arg 3");
    err = kernelLabel.setArg(4, bufAddedtoStack); checkErr(err, "Arg 4");
    err = kernelLabel.setArg(5, bufCounter); checkErr(err, "Arg 5");
    err = kernelLabel.setArg(6, bufEnteredLoop); checkErr(err, "Arg 6");
    err = kernelLabel.setArg(7, bufVisited); checkErr(err, "Arg 7");
    err = kernelLabel.setArg(8, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 8");
    err = kernelLabel.setArg(9, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 9");
    //err = kernelLabel.setArg(10, counter * counter *  sizeof(cl_uint), NULL); checkErr(err, "Arg 10");
    err = kernelLabel.setArg(10, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 11");
    //err = kernelLabel.setArg(12, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 12");
    err = kernelLabel.setArg(11, label_num); checkErr(err, "Arg 13");
    err = kernelLabel.setArg(12, vectorSize); checkErr(err, "Arg 14");
    err = kernelLabel.setArg(13, min_samps); checkErr(err, "Arg 15");

    chrono::time_point<std::chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();

    //Starting the loop
    for(int i = 0; i< vectorSize; i++) {
      counter = 1;
      entered_loop = 0;
      stack[0] = i;

      err= myqueue.enqueueReadBuffer(bufVisited, CL_TRUE, 0, vectorSize * sizeof(cl_uint),visited); checkErr(err, "Read Buffer 1"); 
      if(visited[i]) continue;
 
      //cout << "Visited: " << visited[i] << endl;
     
      err = myqueue.enqueueWriteBuffer(bufCounter, CL_TRUE, 0, sizeof(cl_uint),&counter); checkErr(err, "WriteBuffer 9");
      err = myqueue.enqueueWriteBuffer(bufStack, CL_TRUE, 0, counter * sizeof(cl_uint),stack); checkErr(err, "WriteBuffer 7");
      err = myqueue.enqueueWriteBuffer(bufEnteredLoop, CL_TRUE, 0, sizeof(cl_uint),&entered_loop); checkErr(err, "WriteBuffer 10");

      while(1) {
	
	//chrono::time_point<std::chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();
	//Launching Kernel
	cl::Event event;
	err = myqueue.enqueueNDRangeKernel(kernelNeighbors, cl::NullRange, cl::NDRange(counter), cl::NullRange, NULL, &event);
	event.wait();
	checkErr(err, "Kernel Execution");
	//event.release_event();
	//chrono::time_point<std::chrono::high_resolution_clock> stop = chrono::high_resolution_clock::now();
	//auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start);

	//cout << "Step 1: " << duration.count() << endl;
	
	//myqueue.finish();

	//cout << "Counter 1: " << counter << endl;
    
	//Kernel Arguments for labelling
	err = kernelLabel.setArg(8, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 8");
	err = kernelLabel.setArg(9, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 9");
	err = kernelLabel.setArg(10, counter * sizeof(cl_uint), NULL); checkErr(err, "Arg 10");
	err = kernelLabel.setArg(11, label_num); checkErr(err, "Arg 13");


	//Launching Kernel
	err = myqueue.enqueueNDRangeKernel(kernelLabel, cl::NullRange, cl::NDRange(1), cl::NullRange, NULL, &event);
	event.wait();
	checkErr(err, "Kernel Execution 2");
	myqueue.finish();

	//chrono::time_point<std::chrono::high_resolution_clock> stop2 = chrono::high_resolution_clock::now();
	//auto duration2 = chrono::duration_cast<chrono::nanoseconds>(stop2 - stop);
        //cout << "Step 2: " << duration2.count() << endl;
	
	//Reading counter from kernel
	err= myqueue.enqueueReadBuffer(bufCounter, CL_TRUE, 0, sizeof(cl_uint),&counter); checkErr(err, "Read Buffer 1");
	err= myqueue.enqueueReadBuffer(bufEnteredLoop, CL_TRUE, 0, sizeof(cl_uint),&entered_loop); checkErr(err, "Read Buffer 2");
      
	//chrono::time_point<std::chrono::high_resolution_clock> stop3 = chrono::high_resolution_clock::now();
	//auto duration3 = chrono::duration_cast<chrono::nanoseconds>(stop3 - stop2);
        //cout << "Step 3: " << duration3.count() << endl;


	//cout << "Counter: " << counter << endl;
	if (counter == 0) break;
      }
      if(entered_loop) label_num++;
    }

    chrono::time_point<std::chrono::high_resolution_clock> stop = chrono::high_resolution_clock::now();                                                                     
    auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start);  
    //cout << "TIME! " << duration.count() << endl;


    //Reading final values
    err= myqueue.enqueueReadBuffer(bufLabels, CL_TRUE, 0, vectorSize * sizeof(cl_int),labels);
    err= myqueue.enqueueReadBuffer(bufNeighbCount, CL_TRUE, 0, vectorSize * sizeof(cl_uint),neighbCount);
    err=myqueue.finish();
    checkErr(err, "Finish Queue");

    //Comparison to truth values
    //  int match = 1;
    // for (int j = 0; j < vectorSize; j++){
      //   //cout << true_labels[c-2][j] << " ";
    //  if (true_labels[c-2][j] != labels[j]) {
    //	match = 0;
    //	cout << j << " " << true_labels[c-2][j] << " " << labels[j] << " " <<  neighbCount[j] << endl;
    //  }
    // }    
    // cout << "MATCH: " << match << endl;

  }

  

  return 1;
}
