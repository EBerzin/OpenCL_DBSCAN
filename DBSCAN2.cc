#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <set>

using namespace std;

// C++ version of algorithm

int main(int argc, char *argv[]) {

    //Processing inputs
    int nsamples = 0;
    int ndims = 2;
    double eps = 0;
    int min_samps = 0;
    bool precomputed = false;
    string dist_matrix_fname = "";
    string data_fname = "";

    for(int i_arg = 1; i_arg < argc; i_arg++){
      string arg = argv[i_arg];
      if(arg == "--nsamples" && i_arg+1 < argc)        nsamples = atoi(argv[i_arg+1]);
      if(arg == "--ndims" && i_arg+1 < argc)           ndims    = atoi(argv[i_arg+1]);
      if(arg == "--eps" && i_arg+1 < argc)             eps    = atof(argv[i_arg+1]);
      if(arg == "--min_samps" && i_arg+1 < argc)       min_samps   = atoi(argv[i_arg+1]);
      if(arg == "--precomputed")                       precomputed  = true;
      if(arg == "-dist" && i_arg+1 < argc)                dist_matrix_fname    = argv[i_arg+1];
      if(arg == "-data" && i_arg+1 < argc)                data_fname    = argv[i_arg+1];
    }

    // Validating inputs
    if(nsamples == 0) {
      cout << "ERROR: Enter the number of samples to process" << endl;
      return 1;
    }
    if(eps == 0) {
      cout << "ERROR: Enter a value of epsilon" << endl;
      return 1;
    }
    if(min_samps == 0) {
      cout << "ERROR: Enter a value of min_samps" << endl;
      return 1;
    }
    if(precomputed && dist_matrix_fname == "") {
      cout << "ERROR: Enter a distance matrix filename to run precomputed metric" << endl;
      return 1;
    }
    if(!precomputed && data_fname == "") {
      cout << "ERROR: Enter a data filename" << endl;
      return 1;
    }

    double **data = new double*[nsamples];
    double **dist_matrix = new double*[nsamples];
    for(int i = 0; i < nsamples; i++) {
      data[i] = new double[ndims];
      dist_matrix[i] = new double[nsamples];
    }

    int *labels = new int[nsamples];
    int *visited= new int[nsamples];
    int *imm_neighbors = new int[nsamples];
    int *added_to_queue = new int[nsamples];


    //Reading in true labels to compare to python
    //float **true_labels = new float*[999];
    //for(int i = 0; i < 999; i++) {
    //true_labels[i] = new float[50000];
    //}

    //string inFileName_true = "ClusterData/scikitLabels.txt";
    //ifstream inFile_true;
    //inFile_true.open(inFileName_true.c_str());
    //if (inFile_true.is_open()) {
    //for (int i = 0; i < 999; i++){
    //  for (int j = 0; j < 50000; j++) {
    //    inFile_true >> true_labels[i][j];
    //    //cout << true_labels[i][j] << " ";
      //  }
    //  }
    //  inFile_true.close(); // CLose input file
    // }
    // else { //Error message
  //  cerr << "Can't find input file " << inFileName_true << endl;
    // }

    // cout << "READ TRUTH" << endl;


    vector<float> total_times;
    for(int c = 499; c <= 499; c++) {

      // Reading data from file

      string inFileName = "";
      if(!precomputed) { inFileName = data_fname;}
      else {inFileName = dist_matrix_fname;}


      //string inFileName = "../ClusterData/blobsC" + to_string(c) + ".txt";
      //string inFileName = "blobsN10000.txt";
      ifstream inFile;
      inFile.open(inFileName.c_str());
      if (inFile.is_open()) {
        for (int i = 0; i < nsamples; i++){
          labels[i] = -1;
          visited[i] = 0;
          imm_neighbors[i] = 0;
          added_to_queue[i] = 0;
          if(!precomputed) {
            for (int j = 0; j < ndims; j++) {
              inFile >> data[i][j];
            }
          }
          else {
            for (int j = 0; j < nsamples; j++) {
              inFile >> dist_matrix[i][j];
            }
          }
        }
        inFile.close(); // CLose input file
      }
      else { //Error message
          cerr << "Can't find input file " << inFileName << endl;
      }


      chrono::time_point<std::chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();


      // Okay now implementing this algorithm. The problem is the size
      int label_num = 0;
      int *neighbors = new int[nsamples]; // Counting the number of neighbors

      for(int i = 0; i < nsamples; i++) {

        int counter = 0; // keeps track of the number of elements in the queue
        int entered_loop = 0;


        while(1) {

          if(visited[i] == 0) {

            visited[i] = 1; // Mark as visited, because this point will have a range_query performed on it
            int nNeighb = 0; // number of neighbors of point i
            int *found_neighbors = new int[nsamples];


            for(int j = 0; j < nsamples; j++) {
              double dist = 0;
              if (!precomputed) {dist = (data[i][0] - data[j][0]) * (data[i][0] - data[j][0]) + (data[i][1] - data[j][1]) * (data[i][1] - data[j][1]);}
              else {dist = dist_matrix[i][j] * dist_matrix[i][j];}

              // Counting the number of neighbors
              if (dist <= eps * eps) {
                found_neighbors[nNeighb] = j;
                nNeighb++;
              }
            }
            neighbors[i] = nNeighb;

            // If point is a core point, labels it and it's neighbors as part of a cluster
            // and add them to the neighbors queue
            //cout << "Neighbors: "  << nNeighb << endl;
            if (nNeighb >= min_samps) {
              entered_loop = 1;
              labels[i] = label_num;
              for (int k = 0; k < nNeighb; k++) {
		//  cout << "entered_loop" << endl;
                // Only add to queue if points have not previously been added
                if (added_to_queue[found_neighbors[k]] == 0) {
                  added_to_queue[found_neighbors[k]] = 1;
                  imm_neighbors[counter] = found_neighbors[k];
                  counter++;
                  labels[found_neighbors[k]] = label_num;
                }
              }
            }
            delete[] found_neighbors;

	    // cout << "Counter " << counter << endl;
          }
          if (counter == 0) { break;}
          // This is where we launch may parallel processes, each of which return the indices of neighboring clusters
          i = imm_neighbors[counter-1];
          imm_neighbors[counter-1] = 0;
          counter--;

          // Check if imm_neighbors is unique
          // int* queue_end = imm_neighbors + counter;
          // std::sort(imm_neighbors, queue_end);
          // bool containsDuplicates = (std::unique(imm_neighbors, queue_end) != queue_end);
          // if (containsDuplicates) cout << "Duplicates? " << containsDuplicates << endl;


        }
        if(entered_loop) label_num++;
      }

      chrono::time_point<std::chrono::high_resolution_clock> stop = chrono::high_resolution_clock::now();

        //Comparison to true values
      // int match = 1;
      //int count_zeros = 0;
      // for (int j = 0; j < nsamples; j++){
         //cout << labels[j] << endl;
      //  if (true_labels[c-2][j] != labels[j]) {
      //    match = 0;
      //    cout << j << " " << true_labels[c-2][j] << " " << labels[j] << " " << neighbors[j] << endl;
      //  }
      //  }

       //cout << "TOTAL ZEROS: " << count_zeros << endl;

      //  cout << "MATCH: " << match << endl;


       auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);

       cout << "TIME!" << endl;
       cout << duration.count() << endl;


       cout << "nClusters" << endl;
       set<int> n_clusters(labels, labels + nsamples);
       cout << n_clusters.size() << std::endl;

       //total_times.push_back(duration.count());

     }

     // ofstream output_file2("total_times_1000_v2.txt");
     // ostream_iterator<float> output_iterator2(output_file2, "\n");
     // std::copy(total_times.begin(), total_times.end(), output_iterator2);


     delete[] labels;
     delete[] visited;
     delete[] imm_neighbors;
     delete[] added_to_queue;
     for(int i = 0; i < nsamples; i++) {
       delete[] data[i];
     }
     delete[] data;


    return 1;

}
