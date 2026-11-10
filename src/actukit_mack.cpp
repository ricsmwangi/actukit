// src/actukit_mack.cpp
// Mack (1993) chain-ladder with prediction errors.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

struct Triangle {
    int n = 0;
    std::vector<std::vector<double>> C;   // C[i][j] valid iff i+j < n
    double at(int i, int j) const { return C[i][j]; }
};

struct MackResult {
    std::vector<double> f;          // development factors
    std::vector<double> sigma2;     // Mack variance estimates
    std::vector<double> ultimate;
    std::vector<double> reserve;
    std::vector<double> mse_ult;    // per-origin MSE of ultimate
    double total_reserve = 0.0;
    double mse_total     = 0.0;     // aggregate prediction-error variance
    double se_total      = 0.0;
};

MackResult mack(const Triangle& T) {
    int n = T.n;
    MackResult R;
    R.f.assign(n - 1, 0.0);
    R.sigma2.assign(n - 1, 0.0);
    R.ultimate.assign(n, 0.0);
    R.reserve.assign(n, 0.0);
    R.mse_ult.assign(n, 0.0);

    // Column totals: full (known) and "valid" range (excludes last row) for f_j
    std::vector<double> colSum(n, 0.0), colValid(n, 0.0);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n - j; ++i)     colSum[j]   += T.at(i, j);
        for (int i = 0; i < n - j - 1; ++i) colValid[j] += T.at(i, j);
    }

    // f_j = Σ C_{i,j+1} / Σ C_{i,j}
    for (int j = 0; j < n - 1; ++j)
        R.f[j] = colSum[j + 1] / colValid[j];

    // σ²_j  (Mack 1993 eq 4); last one (j=n-2) via extrapolation
    for (int j = 0; j < n - 2; ++j) {
        int cnt = n - j - 2;
        double s = 0.0;
        for (int i = 0; i < n - j - 1; ++i) {
            double r = T.at(i, j + 1) / T.at(i, j);
            double d = r - R.f[j];
            s += T.at(i, j) * d * d;
        }
        R.sigma2[j] = (cnt > 0) ? s / cnt : 0.0;
    }
    // Mack 1993 sec 4.3 (matches R ChainLadder):
    //   sigma^2[n-2] = min( sigma^2[n-3]^2 / sigma^2[n-4], sigma^2[n-3], sigma^2[n-4] )
    if (n >= 3) {
        double s_prev2 = (n >= 4) ? R.sigma2[n - 4] : 0.0;
        double s_prev1 = R.sigma2[n - 3];
        R.sigma2[n - 2] = (s_prev2 > 1e-12)
            ? std::min({ s_prev1 * s_prev1 / s_prev2, s_prev2, s_prev1 })
            : s_prev1;
    }

    // Projections & reserves
    std::vector<std::vector<double>> Chat(n, std::vector<double>(n, 0.0));
    for (int i = 0; i < n; ++i) {
        int j0 = n - 1 - i;
        for (int j = 0; j <= j0; ++j) Chat[i][j] = T.at(i, j);
        for (int j = j0 + 1; j < n; ++j) Chat[i][j] = Chat[i][j - 1] * R.f[j - 1];
        R.ultimate[i] = Chat[i][n - 1];
        R.reserve[i]  = R.ultimate[i] - T.at(i, j0);
        R.total_reserve += R.reserve[i];
    }

    // Per-origin MSE of ultimate (process + parameter variance)
    for (int i = 0; i < n; ++i) {
        int j0 = n - 1 - i;
        if (j0 == n - 1) continue;   // fully developed
        double s = 0.0;
        for (int j = j0; j < n - 1; ++j) {
            double v = R.sigma2[j] / (R.f[j] * R.f[j]);
            s += v * (1.0 / Chat[i][j] + 1.0 / colValid[j]);
        }
        R.mse_ult[i] = R.ultimate[i] * R.ultimate[i] * s;
    }

    // Aggregate prediction-error variance (diagonal + pairwise cross terms)
    double totalVar = 0.0;
    for (int i = 0; i < n; ++i) totalVar += R.mse_ult[i];
    for (int i = 0; i < n; ++i) {
        for (int k = i + 1; k < n; ++k) {
            int jStart = n - 1 - i;   // = max(j0_i, j0_k)
            double s = 0.0;
            for (int j = jStart; j < n - 1; ++j)
                s += R.sigma2[j] / (R.f[j] * R.f[j]) / colValid[j];
            totalVar += 2.0 * R.ultimate[i] * R.ultimate[k] * s;
        }
    }
    R.mse_total = totalVar;
    R.se_total  = std::sqrt(totalVar);
    return R;
}

Triangle load_csv(const char* path) {
    FILE* fp = std::fopen(path, "r");
    if (!fp) { std::perror(path); std::exit(1); }

    char line[1024];
    if (!std::fgets(line, sizeof(line), fp)) { std::fclose(fp); std::exit(1); }

    std::vector<std::tuple<int, int, double>> data;
    int maxO = 0, maxD = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        int oy, dv; double paid;
        if (std::sscanf(line, "%d,%d,%lf", &oy, &dv, &paid) == 3) {
            data.emplace_back(oy, dv, paid);
            maxO = std::max(maxO, oy);
            maxD = std::max(maxD, dv);
        }
    }
    std::fclose(fp);

    Triangle T;
    T.n = std::max(maxO, maxD) + 1;
    T.C.assign(T.n, std::vector<double>(T.n, 0.0));
    for (auto& [oy, dv, paid] : data) T.C[oy][dv] = paid;
    return T;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s triangle.csv\n", argv[0]);
        return 1;
    }
    Triangle T = load_csv(argv[1]);
    MackResult R = mack(T);

    int n = T.n;
    std::printf("Development factors\n");
    for (int j = 0; j < n - 1; ++j)
        std::printf("  f[%d] = %.6f    sigma2[%d] = %.2f\n",
                    j, R.f[j], j, R.sigma2[j]);

    std::printf("\nOrigin | Ultimate    | Reserve     | S.E.(Ult)\n");
    std::printf("-------+-------------+-------------+-----------\n");
    for (int i = 0; i < n; ++i)
        std::printf("  %2d   | %11.2f | %11.2f | %9.2f\n",
                    i, R.ultimate[i], R.reserve[i], std::sqrt(R.mse_ult[i]));

    std::printf("\nTotal reserve = %.2f   S.E. = %.2f   (CV = %.2f%%)\n",
                R.total_reserve, R.se_total,
                100.0 * R.se_total / R.total_reserve);
    return 0;
}
