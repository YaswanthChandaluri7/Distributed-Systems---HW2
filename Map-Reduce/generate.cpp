#include <bits/stdc++.h>
using namespace std;

int main() {
  int inf=500;
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Random seed
    random_device rd;
    mt19937 gen(rd());

    // Choose n and m randomly
    uniform_int_distribution<int> distN(50, 600);   // change range as needed
    int n = distN(gen);
    long long maxi=(long)(n);
	maxi*=n;
	maxi*=n;

    // Max edges possible in undirected graph with n nodes
    int maxEdges = n * (n - 1) / 2;
    int  tot=powl(10,9);
    tot/=n;
   maxEdges=min((int)maxEdges,(int)tot);
    long long two=maxEdges;
    uniform_int_distribution<int> distM(n-1, maxEdges);
    int m = distM(gen);

    cout << "n = " << n << ", m = " << m << "\n";

    set<pair<int,int>> edges;

    uniform_int_distribution<int> distNode(1, n);

    // Generate unique edges
    while ((int)edges.size() < m) {
        int u = distNode(gen);
        int v = distNode(gen);
        if (u == v) continue;
        if (u > v) swap(u, v);
        edges.insert({u, v});
    }

    // Write edges to file
    ofstream fout("input.txt");
      fout<<n<<' '<<m<<endl;
    for (auto &e : edges) {
        fout << e.first << " " << e.second << "\n";
    }
    fout.close();

    cout << "Edges written to input.txt\n";
    return 0;
}
