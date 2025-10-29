#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <utility>
#include <fstream>
#include <functional> // For hash
#include <mpi.h>
using namespace std;
// Using custom types for clarity
using vertex_t = int;
using count_t = long long;
using edge_t = pair<vertex_t, vertex_t>;
using wedge_t = pair<edge_t, vertex_t>;
using pvc_pair_t = pair<vertex_t, count_t>; // For Per-Vertex Counts

// --- Job 1: Map Edges to Wedges ---
void map_job1(int rank, int world_size, int total_vertices, const vector<vector<vertex_t>> &adj, map<int, vector<wedge_t>> &wedges_to_send)
{
    hash<vertex_t> hasher;
    for (vertex_t center_v = rank; center_v < total_vertices; center_v += world_size)
    {
        const auto &neighbors = adj.at(center_v);
        if (neighbors.size() < 2)
            continue;
        for (size_t i = 0; i < neighbors.size(); ++i)
        {
            for (size_t j = i + 1; j < neighbors.size(); ++j)
            {
                vertex_t v1 = min(neighbors[i], neighbors[j]);
                vertex_t v2 = max(neighbors[i], neighbors[j]);
                int dest_rank = hasher(v1) % world_size;
                wedges_to_send[dest_rank].push_back({{v1, v2}, center_v});
            }
        }
    }
}

// --- Job 2 & 3: Reduce Wedges to Counts ---
void reduce_jobs_2_and_3(const vector<wedge_t> &received_wedges, count_t &local_global_count, map<vertex_t, count_t> &local_per_vertex_counts)
{
    map<edge_t, vector<vertex_t>> grouped_wedges;
    for (const auto &wedge : received_wedges)
    {
        grouped_wedges[wedge.first].push_back(wedge.second);
    }
    for (const auto &pair : grouped_wedges)
    {
        count_t k = pair.second.size();
        if (k < 2)
            continue;
        count_t cycles_found = k * (k - 1) / 2;
        local_global_count += cycles_found;
        const vertex_t v1 = pair.first.first;
        const vertex_t v2 = pair.first.second;
        local_per_vertex_counts[v1] += cycles_found;
        local_per_vertex_counts[v2] += cycles_found;
        for (const auto &center_v : pair.second)
        {
            local_per_vertex_counts[center_v] += (k - 1);
        }
    }
}

int main(int argc, char *argv[])
{
    // freopen("output.txt", "w", stdout); // file output.txt is opened in writing mode i.e "w"
    // freopen("bench.txt", "w", stderr);  // file output.txt is opened in writing mode i.e "w"
    MPI_Init(&argc, &argv);
    int rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 2)
    { /* Error handling */
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    // --- Step 1: Coordinator reads and broadcasts initial data ---
    vector<edge_t> all_edges;
    map<vertex_t, string> id_to_name;
    int total_vertices = 0;

    if (rank == 0)
    {
        // ... (File reading and string-to-int mapping logic is unchanged) ...
        map<string, vertex_t> name_to_id;
        vertex_t next_id = 0;
        ifstream input_file("input.txt");
        if (!input_file.is_open())
        {
            cerr << "Error: Rank 0 could not open input.txt." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        string line;
        while (getline(input_file, line))
        {
            if (line.empty())
                continue;
            stringstream ss(line);
            string u_name, v_name;
            ss >> u_name >> v_name;
            if (name_to_id.find(u_name) == name_to_id.end())
            {
                name_to_id[u_name] = next_id;
                id_to_name[next_id] = u_name;
                next_id++;
            }
            if (name_to_id.find(v_name) == name_to_id.end())
            {
                name_to_id[v_name] = next_id;
                id_to_name[next_id] = v_name;
                next_id++;
            }
            all_edges.push_back({name_to_id[u_name], name_to_id[v_name]});
        }
        total_vertices = next_id;
    }

    // Broadcast setup data
    MPI_Bcast(&total_vertices, 1, MPI_INT, 0, MPI_COMM_WORLD);
    long long num_edges = all_edges.size();
    MPI_Bcast(&num_edges, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    if (rank != 0)
        all_edges.resize(num_edges);
    MPI_Bcast(all_edges.data(), num_edges * sizeof(edge_t), MPI_BYTE, 0, MPI_COMM_WORLD);

    // --- Steps 2, 3, 4, 5 (Map, Shuffle, Reduce for Wedges) are unchanged ---
    vector<vector<vertex_t>> adj(total_vertices);
    for (const auto &edge : all_edges)
    {
        adj[edge.first].push_back(edge.second);
        adj[edge.second].push_back(edge.first);
    }
    map<int, vector<wedge_t>> wedges_to_send;
    map_job1(rank, world_size, total_vertices, adj, wedges_to_send);

    // Shuffle wedges
    vector<int> send_counts_w(world_size, 0);
    for (auto const &[dest, wedges] : wedges_to_send)
    {
        send_counts_w[dest] = wedges.size() * sizeof(wedge_t);
    }
    vector<int> recv_counts_w(world_size, 0);
    MPI_Alltoall(send_counts_w.data(), 1, MPI_INT, recv_counts_w.data(), 1, MPI_INT, MPI_COMM_WORLD);
    vector<char> send_buffer_w;
    vector<int> send_displs_w(world_size + 1, 0);
    for (int i = 0; i < world_size; ++i)
    {
        send_displs_w[i + 1] = send_displs_w[i] + send_counts_w[i];
        if (wedges_to_send.count(i))
        {
            const auto &wedges = wedges_to_send[i];
            send_buffer_w.insert(send_buffer_w.end(), (char *)wedges.data(), (char *)wedges.data() + wedges.size() * sizeof(wedge_t));
        }
    }
    vector<int> recv_displs_w(world_size + 1, 0);
    for (int i = 0; i < world_size; ++i)
        recv_displs_w[i + 1] = recv_displs_w[i] + recv_counts_w[i];
    vector<char> recv_buffer_w(recv_displs_w[world_size]);
    MPI_Alltoallv(send_buffer_w.data(), send_counts_w.data(), send_displs_w.data(), MPI_BYTE, recv_buffer_w.data(), recv_counts_w.data(), recv_displs_w.data(), MPI_BYTE, MPI_COMM_WORLD);
    vector<wedge_t> received_wedges((wedge_t *)recv_buffer_w.data(), (wedge_t *)(recv_buffer_w.data() + recv_buffer_w.size()));

    count_t local_global_count = 0;
    map<vertex_t, count_t> local_per_vertex_counts;
    reduce_jobs_2_and_3(received_wedges, local_global_count, local_per_vertex_counts);

    // --- Step 6: Shuffle and Aggregate Per-Vertex Counts ---
    map<int, vector<pvc_pair_t>> counts_to_send;
    hash<vertex_t> hasher;
    for (const auto &pair : local_per_vertex_counts)
    {
        int dest_rank = hasher(pair.first) % world_size;
        counts_to_send[dest_rank].push_back({pair.first, pair.second});
    }

    vector<int> send_counts_pvc(world_size, 0);
    for (auto const &[dest, pairs] : counts_to_send)
    {
        send_counts_pvc[dest] = pairs.size() * sizeof(pvc_pair_t);
    }
    vector<int> recv_counts_pvc(world_size, 0);
    MPI_Alltoall(send_counts_pvc.data(), 1, MPI_INT, recv_counts_pvc.data(), 1, MPI_INT, MPI_COMM_WORLD);

    vector<char> send_buffer_pvc;
    vector<int> send_displs_pvc(world_size + 1, 0);
    for (int i = 0; i < world_size; ++i)
    {
        send_displs_pvc[i + 1] = send_displs_pvc[i] + send_counts_pvc[i];
        if (counts_to_send.count(i))
        {
            const auto &pairs = counts_to_send[i];
            send_buffer_pvc.insert(send_buffer_pvc.end(), (char *)pairs.data(), (char *)pairs.data() + pairs.size() * sizeof(pvc_pair_t));
        }
    }
    vector<int> recv_displs_pvc(world_size + 1, 0);
    for (int i = 0; i < world_size; ++i)
        recv_displs_pvc[i + 1] = recv_displs_pvc[i] + recv_counts_pvc[i];
    vector<char> recv_buffer_pvc(recv_displs_pvc[world_size]);

    MPI_Alltoallv(send_buffer_pvc.data(), send_counts_pvc.data(), send_displs_pvc.data(), MPI_BYTE,
                  recv_buffer_pvc.data(), recv_counts_pvc.data(), recv_displs_pvc.data(), MPI_BYTE,
                  MPI_COMM_WORLD);

    vector<pvc_pair_t> received_counts((pvc_pair_t *)recv_buffer_pvc.data(), (pvc_pair_t *)(recv_buffer_pvc.data() + recv_buffer_pvc.size()));
    map<vertex_t, count_t> final_per_vertex_counts;
    for (const auto &pair : received_counts)
    {
        final_per_vertex_counts[pair.first] += pair.second;
    }

    // --- Step 7: Final Aggregation and Reporting ---
    count_t final_global_count = 0;
    MPI_Reduce(&local_global_count, &final_global_count, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();

    if (rank == 0)
    {
        final_global_count /= 2;

        // Gather all per-vertex counts to rank 0 for printing
        // This is a simplified gather. For huge numbers of vertices, another approach is needed.
        // But for typical graphs, this is fine.
        int local_pvc_size = final_per_vertex_counts.size();
        vector<int> all_pvc_sizes(world_size);
        MPI_Gather(&local_pvc_size, 1, MPI_INT, all_pvc_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

        vector<pvc_pair_t> final_pvc_results;
        for (const auto &pair : final_per_vertex_counts)
        {
            final_pvc_results.push_back(pair);
        }

        for (int i = 1; i < world_size; ++i)
        {
            if (all_pvc_sizes[i] > 0)
            {
                vector<pvc_pair_t> temp_buffer(all_pvc_sizes[i]);
                MPI_Recv(temp_buffer.data(), all_pvc_sizes[i] * sizeof(pvc_pair_t), MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                final_pvc_results.insert(final_pvc_results.end(), temp_buffer.begin(), temp_buffer.end());
            }
        }

        // Reporting
        double max_time;
        double elapsed_time = end_time - start_time;
        MPI_Reduce(&elapsed_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        cout << "\n========== FINAL RESULTS ==========" << endl;
        cout << "Global_Count\t4-Cycles\t" << final_global_count << endl;
        for (const auto &pair : final_pvc_results)
        {
            cout << "Per-Vertex_Count\t" << id_to_name[pair.first] << "\t" << (pair.second / 2) << endl;
        }
        cout << "===================================" << endl;

        cerr << "--- BENCHMARK DATA ---" << endl;
        cerr << "CORES: " << world_size << endl;
        cerr << "TOTAL_TIME: " << max_time << endl;
    }
    else
    {
        // Workers participate in reductions and send final PVC to master
        double elapsed_time = end_time - start_time;
        MPI_Reduce(&elapsed_time, nullptr, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        int local_pvc_size = final_per_vertex_counts.size();
        MPI_Gather(&local_pvc_size, 1, MPI_INT, nullptr, 0, MPI_INT, 0, MPI_COMM_WORLD);

        if (local_pvc_size > 0)
        {
            vector<pvc_pair_t> pvc_to_send;
            for (const auto &pair : final_per_vertex_counts)
            {
                pvc_to_send.push_back(pair);
            }
            MPI_Send(pvc_to_send.data(), pvc_to_send.size() * sizeof(pvc_pair_t), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
