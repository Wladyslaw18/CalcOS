/*
 * File: LinearAlgebra.c
 * Author: W. Kowal
 * Description: Basic and advanced linear algebra routines implementation.
 * 
 * Refactored to eliminate duplicate math helpers. Now strictly routes
 * through the unified /Infrastructure/Utils/MathUtils.h primitives.
 */

#include "LinearAlgebra.h"
#include "Infrastructure/Utils/MemoryUtils.h"
#include "Infrastructure/Utils/MathUtils.h"

bool matrix_add(CalculatorState* state,
                const double* A, uint32_t a_rows, uint32_t a_cols,
                const double* B, uint32_t b_rows, uint32_t b_cols,
                double* C) {
    if (a_rows > MAX_MATRIX_DIM || a_cols > MAX_MATRIX_DIM ||
        b_rows > MAX_MATRIX_DIM || b_cols > MAX_MATRIX_DIM) {
        if (state) state->flags |= 4; // Out of bounds or invalid
        return false;
    }
    if (a_rows != b_rows || a_cols != b_cols) {
        if (state) state->flags |= 4; // Dimension mismatch
        return false;
    }

    uint32_t total = a_rows * a_cols;
    for (uint32_t i = 0; i < total; ++i) {
        C[i] = A[i] + B[i];
    }
    return true;
}

bool matrix_mul(CalculatorState* state,
                const double* A, uint32_t a_rows, uint32_t a_cols,
                const double* B, uint32_t b_rows, uint32_t b_cols,
                double* C) {
    if (a_rows > MAX_MATRIX_DIM || a_cols > MAX_MATRIX_DIM ||
        b_rows > MAX_MATRIX_DIM || b_cols > MAX_MATRIX_DIM) {
        if (state) state->flags |= 4;
        return false;
    }
    if (a_cols != b_rows) {
        if (state) state->flags |= 4;
        return false;
    }

    // Transpose B to improve cache locality (sequential read-friendly)
    double B_T[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    for (uint32_t r = 0; r < b_rows; ++r) {
        for (uint32_t c = 0; c < b_cols; ++c) {
            B_T[c * b_rows + r] = B[r * b_cols + c];
        }
    }

    for (uint32_t r = 0; r < a_rows; ++r) {
        for (uint32_t c = 0; c < b_cols; ++c) {
            double sum = 0.0;
            uint32_t a_offset = r * a_cols;
            uint32_t b_offset = c * b_rows; // since b_rows == a_cols
            for (uint32_t k = 0; k < a_cols; ++k) {
                sum += A[a_offset + k] * B_T[b_offset + k];
            }
            C[r * b_cols + c] = sum;
        }
    }
    return true;
}

bool matrix_transpose(CalculatorState* state,
                      const double* A, uint32_t rows, uint32_t cols,
                      double* C) {
    if (rows > MAX_MATRIX_DIM || cols > MAX_MATRIX_DIM) {
        if (state) state->flags |= 4;
        return false;
    }

    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols; ++c) {
            C[c * rows + r] = A[r * cols + c];
        }
    }
    return true;
}

// Computes determinant using Gaussian elimination with partial pivoting to be O(N^3)
bool matrix_determinant(CalculatorState* state,
                        const double* A, uint32_t dim,
                        double* result) {
    if (dim > MAX_MATRIX_DIM || dim == 0) {
        if (state) state->flags |= 4;
        return false;
    }

    double mat[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    fast_memcpy(mat, A, sizeof(double) * dim * dim);

    double det = 1.0;
    for (uint32_t i = 0; i < dim; ++i) {
        // Find pivot
        uint32_t pivot = i;
        double max_val = math_abs(mat[i * dim + i]);
        for (uint32_t r = i + 1; r < dim; ++r) {
            double val = math_abs(mat[r * dim + i]);
            if (val > max_val) {
                max_val = val;
                pivot = r;
            }
        }

        if (max_val < 1e-12) {
            *result = 0.0;
            return true; // Singular matrix
        }

        if (pivot != i) {
            // Swap rows
            for (uint32_t c = 0; c < dim; ++c) {
                double temp = mat[i * dim + c];
                mat[i * dim + c] = mat[pivot * dim + c];
                mat[pivot * dim + c] = temp;
            }
            det = -det;
        }

        det *= mat[i * dim + i];

        for (uint32_t r = i + 1; r < dim; ++r) {
            double factor = mat[r * dim + i] / mat[i * dim + i];
            for (uint32_t c = i + 1; c < dim; ++c) {
                mat[r * dim + c] -= factor * mat[i * dim + c];
            }
        }
    }

    *result = det;
    return true;
}

// Matrix inverse using Gauss-Jordan elimination with partial pivoting
bool matrix_inverse(CalculatorState* state,
                    const double* A, uint32_t dim,
                    double* C) {
    if (dim > MAX_MATRIX_DIM || dim == 0) {
        if (state) state->flags |= 4;
        return false;
    }

    // Augmented matrix [A | I]
    double aug[MAX_MATRIX_DIM * (MAX_MATRIX_DIM * 2)];
    uint32_t aug_cols = dim * 2;

    for (uint32_t r = 0; r < dim; ++r) {
        for (uint32_t c = 0; c < dim; ++c) {
            aug[r * aug_cols + c] = A[r * dim + c];
            aug[r * aug_cols + (c + dim)] = (r == c) ? 1.0 : 0.0;
        }
    }

    for (uint32_t i = 0; i < dim; ++i) {
        // Find pivot
        uint32_t pivot = i;
        double max_val = math_abs(aug[i * aug_cols + i]);
        for (uint32_t r = i + 1; r < dim; ++r) {
            double val = math_abs(aug[r * aug_cols + i]);
            if (val > max_val) {
                max_val = val;
                pivot = r;
            }
        }

        if (max_val < 1e-12) {
            if (state) state->flags |= 2; // division by zero / singular matrix
            return false;
        }

        if (pivot != i) {
            for (uint32_t c = 0; c < aug_cols; ++c) {
                double temp = aug[i * aug_cols + c];
                aug[i * aug_cols + c] = aug[pivot * aug_cols + c];
                aug[pivot * aug_cols + c] = temp;
            }
        }

        // Scale pivot row to 1
        double pivot_val = aug[i * aug_cols + i];
        for (uint32_t c = i; c < aug_cols; ++c) {
            aug[i * aug_cols + c] /= pivot_val;
        }

        // Eliminate other columns
        for (uint32_t r = 0; r < dim; ++r) {
            if (r != i) {
                double factor = aug[r * aug_cols + i];
                for (uint32_t c = i; c < aug_cols; ++c) {
                    aug[r * aug_cols + c] -= factor * aug[i * aug_cols + c];
                }
            }
        }
    }

    // Extract inverse from augmented matrix
    for (uint32_t r = 0; r < dim; ++r) {
        for (uint32_t c = 0; c < dim; ++c) {
            C[r * dim + c] = aug[r * aug_cols + (c + dim)];
        }
    }

    return true;
}

// Reduced Row Echelon Form (RREF)
bool matrix_rref(CalculatorState* state,
                 const double* A, uint32_t rows, uint32_t cols,
                 double* C) {
    if (rows > MAX_MATRIX_DIM || cols > MAX_MATRIX_DIM) {
        if (state) state->flags |= 4;
        return false;
    }

    // Copy A to C
    fast_memcpy(C, A, sizeof(double) * rows * cols);

    uint32_t lead = 0;
    for (uint32_t r = 0; r < rows; ++r) {
        if (lead >= cols) {
            break;
        }
        uint32_t i = r;
        while (math_abs(C[i * cols + lead]) < 1e-12) {
            i++;
            if (i == rows) {
                i = r;
                lead++;
                if (lead == cols) {
                    return true;
                }
            }
        }

        // Swap rows i and r
        if (i != r) {
            for (uint32_t c = 0; c < cols; ++c) {
                double temp = C[r * cols + c];
                C[r * cols + c] = C[i * cols + c];
                C[i * cols + c] = temp;
            }
        }

        // Divide row r by C[r, lead]
        double val = C[r * cols + lead];
        if (math_abs(val) > 1e-12) {
            for (uint32_t c = 0; c < cols; ++c) {
                C[r * cols + c] /= val;
            }
        }

        // Subtract from other rows
        for (uint32_t i_row = 0; i_row < rows; ++i_row) {
            if (i_row != r) {
                double factor = C[i_row * cols + lead];
                for (uint32_t c = 0; c < cols; ++c) {
                    C[i_row * cols + c] -= factor * C[r * cols + c];
                }
            }
        }
        lead++;
    }

    return true;
}

bool calc_matrix_lu(const double* A, uint32_t dim, double* L, double* U, uint32_t* P) {
    if (dim > MAX_MATRIX_DIM || dim == 0 || !A || !L || !U || !P) {
        return false;
    }

    for (uint32_t r = 0; r < dim; ++r) {
        P[r] = r;
        for (uint32_t c = 0; c < dim; ++c) {
            L[r * dim + c] = (r == c) ? 1.0 : 0.0;
            U[r * dim + c] = A[r * dim + c];
        }
    }

    for (uint32_t i = 0; i < dim; ++i) {
        uint32_t pivot = i;
        double max_val = math_abs(U[i * dim + i]);
        for (uint32_t r = i + 1; r < dim; ++r) {
            double val = math_abs(U[r * dim + i]);
            if (val > max_val) {
                max_val = val;
                pivot = r;
            }
        }

        if (max_val < 1e-12) {
            return false;
        }

        if (pivot != i) {
            for (uint32_t c = 0; c < dim; ++c) {
                double temp = U[i * dim + c];
                U[i * dim + c] = U[pivot * dim + c];
                U[pivot * dim + c] = temp;
            }
            for (uint32_t c = 0; c < i; ++c) {
                double temp = L[i * dim + c];
                L[i * dim + c] = L[pivot * dim + c];
                L[pivot * dim + c] = temp;
            }
            uint32_t temp_p = P[i];
            P[i] = P[pivot];
            P[pivot] = temp_p;
        }

        for (uint32_t r = i + 1; r < dim; ++r) {
            double factor = U[r * dim + i] / U[i * dim + i];
            L[r * dim + i] = factor;
            U[r * dim + i] = 0.0;
            for (uint32_t c = i + 1; c < dim; ++c) {
                U[r * dim + c] -= factor * U[i * dim + c];
            }
        }
    }

    return true;
}

bool calc_matrix_qr(const double* A, uint32_t rows, uint32_t cols, double* Q, double* R) {
    if (rows > MAX_MATRIX_DIM || cols > MAX_MATRIX_DIM || rows == 0 || cols == 0 || !A || !Q || !R) {
        return false;
    }

    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols; ++c) {
            Q[r * cols + c] = A[r * cols + c];
        }
    }
    fast_memset(R, 0, sizeof(double) * cols * cols);

    for (uint32_t i = 0; i < cols; ++i) {
        double sum_sq = 0.0;
        for (uint32_t r = 0; r < rows; ++r) {
            double val = Q[r * cols + i];
            sum_sq += val * val;
        }
        double norm = math_sqrt(sum_sq);

        if (norm < 1e-12) {
            return false;
        }

        R[i * cols + i] = norm;

        for (uint32_t r = 0; r < rows; ++r) {
            Q[r * cols + i] /= norm;
        }

        for (uint32_t j = i + 1; j < cols; ++j) {
            double dot = 0.0;
            for (uint32_t r = 0; r < rows; ++r) {
                dot += Q[r * cols + i] * Q[r * cols + j];
            }
            R[i * cols + j] = dot;
            for (uint32_t r = 0; r < rows; ++r) {
                Q[r * cols + j] -= dot * Q[r * cols + i];
            }
        }
    }

    return true;
}

bool calc_solve_linear(const double* A, const double* b, uint32_t dim, double* x) {
    if (dim > MAX_MATRIX_DIM || dim == 0 || !A || !b || !x) {
        return false;
    }

    double L[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    double U[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    uint32_t P[MAX_MATRIX_DIM];

    if (!calc_matrix_lu(A, dim, L, U, P)) {
        return false;
    }

    double Pb[MAX_MATRIX_DIM];
    for (uint32_t i = 0; i < dim; ++i) {
        Pb[i] = b[P[i]];
    }

    double y[MAX_MATRIX_DIM];
    for (uint32_t i = 0; i < dim; ++i) {
        double sum = 0.0;
        for (uint32_t j = 0; j < i; ++j) {
            sum += L[i * dim + j] * y[j];
        }
        y[i] = Pb[i] - sum;
    }

    for (int32_t i = (int32_t)dim - 1; i >= 0; --i) {
        double sum = 0.0;
        for (uint32_t j = (uint32_t)i + 1; j < dim; ++j) {
            sum += U[i * dim + j] * x[j];
        }
        if (math_abs(U[i * dim + i]) < 1e-12) {
            return false;
        }
        x[i] = (y[i] - sum) / U[i * dim + i];
    }

    return true;
}

bool calc_least_squares(const double* A, const double* b, uint32_t rows, uint32_t cols, double* x) {
    if (rows > MAX_MATRIX_DIM || cols > MAX_MATRIX_DIM || rows == 0 || cols == 0 || !A || !b || !x) {
        return false;
    }

    double Q[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    double R[MAX_MATRIX_DIM * MAX_MATRIX_DIM];

    if (!calc_matrix_qr(A, rows, cols, Q, R)) {
        return false;
    }

    double d[MAX_MATRIX_DIM];
    for (uint32_t i = 0; i < cols; ++i) {
        double sum = 0.0;
        for (uint32_t r = 0; r < rows; ++r) {
            sum += Q[r * cols + i] * b[r];
        }
        d[i] = sum;
    }

    for (int32_t i = (int32_t)cols - 1; i >= 0; --i) {
        double sum = 0.0;
        for (uint32_t j = (uint32_t)i + 1; j < cols; ++j) {
            sum += R[i * cols + j] * x[j];
        }
        if (math_abs(R[i * cols + i]) < 1e-12) {
            return false;
        }
        x[i] = (d[i] - sum) / R[i * cols + i];
    }

    return true;
}

bool calc_matrix_trace(const double* A, uint32_t dim, double* result) {
    if (dim > MAX_MATRIX_DIM || dim == 0 || !A || !result) {
        return false;
    }

    double tr = 0.0;
    for (uint32_t i = 0; i < dim; ++i) {
        tr += A[i * dim + i];
    }
    *result = tr;
    return true;
}

bool calc_matrix_power_iteration(const double* A, uint32_t dim,
                                 double* out_eigenvec, double* out_eigenval,
                                 uint32_t max_iter) {
    if (dim > MAX_MATRIX_DIM || dim == 0 || !A || !out_eigenvec || !out_eigenval) {
        return false;
    }

    if (max_iter == 0) {
        max_iter = 200;
    }

    double v[MAX_MATRIX_DIM];
    double w[MAX_MATRIX_DIM];

    for (uint32_t j = 0; j < dim; ++j) {
        double col_sum = 0.0;
        for (uint32_t i = 0; i < dim; ++i) {
            col_sum += A[i * dim + j];
        }
        v[j] = col_sum;
    }

    double seed_norm_sq = 0.0;
    for (uint32_t j = 0; j < dim; ++j) {
        seed_norm_sq += v[j] * v[j];
    }
    double seed_norm = math_sqrt(seed_norm_sq);

    if (seed_norm < 1e-12) {
        for (uint32_t j = 0; j < dim; ++j) {
            v[j] = (j == 0) ? 1.0 : 0.0;
        }
    } else {
        for (uint32_t j = 0; j < dim; ++j) {
            v[j] /= seed_norm;
        }
    }

    double eigenval_old = 0.0;

    for (uint32_t iter = 0; iter < max_iter; ++iter) {
        for (uint32_t i = 0; i < dim; ++i) {
            double sum = 0.0;
            for (uint32_t j = 0; j < dim; ++j) {
                sum += A[i * dim + j] * v[j];
            }
            w[i] = sum;
        }

        double norm_sq = 0.0;
        for (uint32_t i = 0; i < dim; ++i) {
            size_t idx = i;
            norm_sq += w[idx] * w[idx];
        }
        double norm = math_sqrt(norm_sq);

        if (norm < 1e-15) {
            for (uint32_t j = 0; j < dim; ++j) {
                out_eigenvec[j] = v[j];
            }
            *out_eigenval = 0.0;
            return true;
        }

        for (uint32_t i = 0; i < dim; ++i) {
            v[i] = w[i] / norm;
        }

        double lambda = 0.0;
        for (uint32_t i = 0; i < dim; ++i) {
            double av_i = 0.0;
            for (uint32_t j = 0; j < dim; ++j) {
                av_i += A[i * dim + j] * v[j];
            }
            lambda += v[i] * av_i;
        }

        if (math_abs(lambda - eigenval_old) < 1e-9) {
            for (uint32_t j = 0; j < dim; ++j) {
                out_eigenvec[j] = v[j];
            }
            *out_eigenval = lambda;
            return true;
        }

        eigenval_old = lambda;
    }

    for (uint32_t j = 0; j < dim; ++j) {
        out_eigenvec[j] = v[j];
    }
    *out_eigenval = eigenval_old;
    return true;
}

bool calc_matrix_eigenvalues_symmetric(const double* A, uint32_t dim, double* eigenvalues) {
    if (dim > MAX_MATRIX_DIM || dim == 0 || !A || !eigenvalues) {
        return false;
    }

    double Ak[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    double Q[MAX_MATRIX_DIM * MAX_MATRIX_DIM];
    double R[MAX_MATRIX_DIM * MAX_MATRIX_DIM];

    fast_memcpy(Ak, A, sizeof(double) * dim * dim);

    for (uint32_t outer = 0; outer < 100; ++outer) {
        double mu = Ak[(dim - 1) * dim + (dim - 1)];

        for (uint32_t i = 0; i < dim; ++i) {
            Ak[i * dim + i] -= mu;
        }

        if (!calc_matrix_qr(Ak, dim, dim, Q, R)) {
            for (uint32_t i = 0; i < dim; ++i) {
                Ak[i * dim + i] += mu;
            }
            break;
        }

        for (uint32_t r = 0; r < dim; ++r) {
            for (uint32_t c = 0; c < dim; ++c) {
                double sum = 0.0;
                for (uint32_t k = 0; k < dim; ++k) {
                    sum += R[r * dim + k] * Q[k * dim + c];
                }
                Ak[r * dim + c] = sum + ((r == c) ? mu : 0.0);
            }
        }

        int converged = 1;
        for (uint32_t i = 0; i < dim - 1; ++i) {
            if (math_abs(Ak[(i + 1) * dim + i]) >= 1e-9) {
                converged = 0;
                break;
            }
        }
        if (converged) {
            break;
        }
    }

    for (uint32_t i = 0; i < dim; ++i) {
        eigenvalues[i] = Ak[i * dim + i];
    }
    return true;
}
