__kernel void radius_neighbors(__global float* restrict X,
	      			__global float* restrict Y,
				__global uint* restrict neighbCount,
				__global uint* restrict neighbIdx,
				__global uint* restrict stack,
				__global uint* restrict visited,
				uint N, float eps) {

	uint i = stack[get_global_id(0)];

	if(visited[i] != 0) return;

	//visited[i] = 1;
	uint nNeighb = 0;
	float dist = 0;

	for(uint j = 0; j < N; j++) {
		 dist = (X[i] - X[j]) * (X[i] - X[j]) + (Y[i] - Y[j]) * (Y[i] - Y[j]);
		 if(dist <= eps * eps) {
		 	 neighbIdx[N*i + nNeighb] = j;
		 	 nNeighb++;
		 }
	}

	neighbCount[i] = nNeighb;
}

__kernel void label(__global uint* restrict neighbCount,
		     __global uint* restrict neighbIdx,
		     __global uint* restrict stack,
	      	     __global uint* restrict labels,
		     __global uint* restrict added_to_stack,
		     __global uint* restrict counter, 
		     __global uint* restrict entered_loop, 
		     __global uint* restrict visited,
		     __local uint*  restrict stack_copy,
		     uint label_num, uint N, uint min_samps) {
	

	for(uint i = 0;	i < *counter; i++) {
		 stack_copy[i] = stack[i];
		 printf("Stack: %d\n", stack_copy[i]);
 	}

	uint total_added = 0;
	for(uint i = 0; i < *counter; i++) {
		 uint j = stack_copy[i];
		 if(visited[j]) continue;
		 if(neighbCount[j] >= min_samps) {
		 	labels[j] = label_num;
			*entered_loop = 1;
			for (uint k = 0; k < neighbCount[j]; k++) {
			    uint u_neighbIdx = neighbIdx[j*N+k];
			    if(added_to_stack[u_neighbIdx] == 0) {
				added_to_stack[u_neighbIdx] = 1;
				stack[total_added] = u_neighbIdx;
				total_added++;
				labels[u_neighbIdx] = label_num;
			    }
			}
		 }
		 visited[j] = 1;
	}

	*counter = total_added;

}


__kernel void label_local(__global uint* restrict neighbCount,
                     __global uint* restrict neighbIdx,
                     __global uint* restrict stack,
                     __global uint* restrict labels,
                     __global uint* restrict added_to_stack,
                     __global uint* restrict counter,
                     __global uint* restrict entered_loop,
                     __global uint* restrict visited,
                     __local uint*  restrict stack_copy,
                    // __local uint* restrict stack_copy_write,
                     __local uint*  restrict neighbCount_copy,
                     __local uint*  restrict neighbIdx_copy,
                    // __local uint*  restrict added_to_stack_copy,
                    // __local uint*  restrict visited_copy,
                     uint label_num, uint N, uint min_samps) {
		     
	for(uint i = 0; i < *counter; i++) {
                      stack_copy[i] = stack[i];
                 neighbCount_copy[i] = neighbCount[stack_copy[i]];
        }

        uint total_added = 0;
        for(uint i = 0; i < *counter; i++) {
                 uint j = stack_copy[i];
                 for(uint idx = 0; idx < neighbCount_copy[i]; idx++) {
                        neighbIdx_copy[idx] = neighbIdx[j*N + idx];
                 }
                 if(visited[j]) continue;
                 if(neighbCount_copy[i] >= min_samps) {
                        labels[j] = label_num;
                        *entered_loop = 1;
                        for (uint k = 0; k < neighbCount_copy[i]; k++) {
                            uint u_neighbIdx = neighbIdx_copy[k];
                            if(added_to_stack[u_neighbIdx] == 0) {
                                added_to_stack[u_neighbIdx] = 1;
                                stack[total_added] = u_neighbIdx;
                                total_added++;
                                labels[u_neighbIdx] = label_num;
                            }
                        }
                 }
         visited[j] = 1;
        }

        *counter = total_added;



}
