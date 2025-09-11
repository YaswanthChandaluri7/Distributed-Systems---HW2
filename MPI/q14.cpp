#include <mpi.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N, M, P, total_nnz = 0;
    vector<vector<int>> A_cols, A_vals, B_cols, B_vals;

    if (rank == 0) {
        cin >> N >> M >> P;
        A_cols.resize(N); A_vals.resize(N);
        B_cols.resize(M); B_vals.resize(M);

        
        for (int i = 0; i < N; i++) {
            int k; cin >> k;
            A_cols[i].resize(k); A_vals[i].resize(k);
            total_nnz += k;
            for (int j = 0; j < k; j++) {
                cin >> A_cols[i][j] >> A_vals[i][j];
            }
        }

        for (int i = 0; i < M; i++) {
            int k; cin >> k;
            B_cols[i].resize(k); B_vals[i].resize(k);
            for (int j = 0; j < k; j++) {
                cin >> B_cols[i][j] >> B_vals[i][j];
            }
        }
    }

    
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&P, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&total_nnz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    
    
    if (rank != 0) {
        A_cols.resize(N); A_vals.resize(N);
        B_cols.resize(M); B_vals.resize(M);
    }

    
    vector<int> A_sizes(N);
    if (rank == 0) {
        for (int i = 0; i < N; i++) A_sizes[i] = A_cols[i].size();
    }
    MPI_Bcast(A_sizes.data(), N, MPI_INT, 0, MPI_COMM_WORLD);

    // Pack and broadcast all A data efficiently
    int total_A_elems = 0;
    for (int sz : A_sizes) total_A_elems += sz;
    
    vector<int> A_cols_packed(total_A_elems), A_vals_packed(total_A_elems);
    
    if (rank == 0) {
        int pos = 0;
        for (int i = 0; i < N; i++) {
            for (int val : A_cols[i]) A_cols_packed[pos++] = val;
        }
        pos = 0;
        for (int i = 0; i < N; i++) {
            for (int val : A_vals[i]) A_vals_packed[pos++] = val;
        }
    }
    
    // SINGLE BATCH BROADCAST
    MPI_Bcast(A_cols_packed.data(), total_A_elems, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(A_vals_packed.data(), total_A_elems, MPI_INT, 0, MPI_COMM_WORLD);

    // Unpack A data
    if (rank != 0) {
        int pos = 0;
        for (int i = 0; i < N; i++) {
            A_cols[i].resize(A_sizes[i]); A_vals[i].resize(A_sizes[i]);
            for (int j = 0; j < A_sizes[i]; j++) {
                A_cols[i][j] = A_cols_packed[pos++];
            }
        }
        pos = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < A_sizes[i]; j++) {
                A_vals[i][j] = A_vals_packed[pos++];
            }
        }
    }

    // Same optimized broadcast for B matrix
    vector<int> B_sizes(M);
    if (rank == 0) {
        for (int i = 0; i < M; i++) B_sizes[i] = B_cols[i].size();
    }
    MPI_Bcast(B_sizes.data(), M, MPI_INT, 0, MPI_COMM_WORLD);

    int total_B_elems = 0;
    for (int sz : B_sizes) total_B_elems += sz;
    
    vector<int> B_cols_packed(total_B_elems), B_vals_packed(total_B_elems);
    
    if (rank == 0) {
        int pos = 0;
        for (int i = 0; i < M; i++) {
            for (int val : B_cols[i]) B_cols_packed[pos++] = val;
        }
        pos = 0;
        for (int i = 0; i < M; i++) {
            for (int val : B_vals[i]) B_vals_packed[pos++] = val;
        }
    }
    
    MPI_Bcast(B_cols_packed.data(), total_B_elems, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B_vals_packed.data(), total_B_elems, MPI_INT, 0, MPI_COMM_WORLD);

    // Unpack B data
    if (rank != 0) {
        int pos = 0;
        for (int i = 0; i < M; i++) {
            B_cols[i].resize(B_sizes[i]); B_vals[i].resize(B_sizes[i]);
            for (int j = 0; j < B_sizes[i]; j++) {
                B_cols[i][j] = B_cols_packed[pos++];
            }
        }
        pos = 0;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < B_sizes[i]; j++) {
                B_vals[i][j] = B_vals_packed[pos++];
            }
        }
    }

    //  Optimal load balancing
    vector<int> my_work_elements;
    int nnz_per_proc = total_nnz / size;
    int nnz_remainder = total_nnz % size;
    
    int my_start_nnz, my_end_nnz;
    if (rank < nnz_remainder) {
        my_start_nnz = rank * (nnz_per_proc + 1);
        my_end_nnz = my_start_nnz + nnz_per_proc + 1;
    } else {
        my_start_nnz = nnz_remainder * (nnz_per_proc + 1) + (rank - nnz_remainder) * nnz_per_proc;
        my_end_nnz = my_start_nnz + nnz_per_proc;
    }

    // Map my non-zero range to actual matrix elements
    int current_nnz = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < (int)A_cols[i].size(); j++) {
            if (current_nnz >= my_start_nnz && current_nnz < my_end_nnz) {
                // This element belongs to me
                my_work_elements.push_back(i);  // Store row index
                my_work_elements.push_back(j);  // Store element index within row
            }
            current_nnz++;
        }
    }

    //  Process my assigned non-zero elements
    // row -> (col -> val)
    unordered_map<int, unordered_map<int, int>> row_results; 

    for (int idx = 0; idx < (int)my_work_elements.size(); idx += 2) {
        int row = my_work_elements[idx];
        int elem_idx = my_work_elements[idx + 1];
        
        int k = A_cols[row][elem_idx];
        int a_val = A_vals[row][elem_idx];
        
        if (k >= 0 && k < M) {
            for (int b_idx = 0; b_idx < (int)B_cols[k].size(); b_idx++) {
                int j = B_cols[k][b_idx];
                int b_val = B_vals[k][b_idx];
                row_results[row][j] += a_val * b_val;
            }
        }
    }

    
    // Collect partial results using efficient collective operations
    
    if (rank == 0) {
        vector<unordered_map<int, int>> final_rows(N);
        
        // Add master's results
        for (const auto& row_pair : row_results) {
            final_rows[row_pair.first] = row_pair.second;
        }
        
        // Collect from all other processes using optimized pattern
        for (int p = 1; p < size; p++) {
            int num_rows;
            MPI_Recv(&num_rows, 1, MPI_INT, p, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            for (int r = 0; r < num_rows; r++) {
                int row_idx, num_cols;
                MPI_Recv(&row_idx, 1, MPI_INT, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&num_cols, 1, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                if (num_cols > 0) {
                    vector<int> cols(num_cols), vals(num_cols);
                    MPI_Recv(cols.data(), num_cols, MPI_INT, p, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(vals.data(), num_cols, MPI_INT, p, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    
                    // Merge results
                    for (int c = 0; c < num_cols; c++) {
                        final_rows[row_idx][cols[c]] += vals[c];
                    }
                }
            }
        }
        
        // Output final result
        for (int i = 0; i < N; i++) {
            vector<pair<int, int>> row_result;
            for (const auto& pair : final_rows[i]) {
                if (pair.second != 0) {
                    row_result.push_back({pair.first, pair.second});
                }
            }
            
            sort(row_result.begin(), row_result.end());
            
            cout << row_result.size();
            for (const auto& p : row_result) {
                cout << " " << p.first << " " << p.second;
            }
            cout << "\n";
        }
    } else {
        // Send results efficiently
        int num_rows = row_results.size();
        MPI_Send(&num_rows, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        
        for (const auto& row_pair : row_results) {
            int row_idx = row_pair.first;
            MPI_Send(&row_idx, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            vector<int> cols, vals;
            for (const auto& col_pair : row_pair.second) {
                if (col_pair.second != 0) {
                    cols.push_back(col_pair.first);
                    vals.push_back(col_pair.second);
                }
            }
            
            int num_cols = cols.size();
            MPI_Send(&num_cols, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            if (num_cols > 0) {
                MPI_Send(cols.data(), num_cols, MPI_INT, 0, 3, MPI_COMM_WORLD);
                MPI_Send(vals.data(), num_cols, MPI_INT, 0, 4, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Finalize();
    return 0;
}
