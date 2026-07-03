/*
 * File: LinearAlgebra.h
 * Author: W. Kowal
 * Description: Basic and advanced linear algebra routines.
 */

#ifndef LINEAR_ALGEBRA_H
#define LINEAR_ALGEBRA_H

#include <stdint.h>
#include <stdbool.h>
#include "../../State/CalculatorState.h"

#define MAX_MATRIX_DIM 8

// Matrix addition: A + B = C (dims must match: rows x cols)
// Returns true on success, false if dimensions are out of bounds or mismatch.
bool matrix_add(CalculatorState* state,
                const double* A, uint32_t a_rows, uint32_t a_cols,
                const double* B, uint32_t b_rows, uint32_t b_cols,
                double* C);

// Matrix multiplication: A * B = C (A is a_rows x a_cols, B is b_rows x b_cols, C is a_rows x b_cols)
// Returns true on success, false if dimensions are out of bounds or mismatch.
bool matrix_mul(CalculatorState* state,
                const double* A, uint32_t a_rows, uint32_t a_cols,
                const double* B, uint32_t b_rows, uint32_t b_cols,
                double* C);

// Matrix transpose: A^T = C (A is rows x cols, C is cols x rows)
// Returns true on success, false if dimensions are out of bounds.
bool matrix_transpose(CalculatorState* state,
                      const double* A, uint32_t rows, uint32_t cols,
                      double* C);

// Matrix determinant: returns det(A) for square matrix A (dim x dim)
// Returns true on success, false if not square or out of bounds.
bool matrix_determinant(CalculatorState* state,
                        const double* A, uint32_t dim,
                        double* result);

// Matrix inverse: A^-1 = C for square matrix A (dim x dim)
// Returns true on success, false if not square, singular, or out of bounds.
bool matrix_inverse(CalculatorState* state,
                    const double* A, uint32_t dim,
                    double* C);

// Reduced Row Echelon Form (RREF): computes RREF of A (rows x cols) and stores in C
// Returns true on success, false if out of bounds.
bool matrix_rref(CalculatorState* state,
                 const double* A, uint32_t rows, uint32_t cols,
                 double* C);

// LU decomposition with partial pivoting: PA = LU
// A: dim x dim input matrix, L: dim x dim lower triangular, U: dim x dim upper triangular, P: dim-sized permutation vector
// Returns true on success, false if dimensions exceed MAX_MATRIX_DIM, matrix is singular, or arguments are invalid.
bool calc_matrix_lu(const double* A, uint32_t dim, double* L, double* U, uint32_t* P);

// QR decomposition using Modified Gram-Schmidt: A = QR
// A: rows x cols input matrix, Q: rows x cols matrix with orthonormal columns, R: cols x cols upper triangular matrix
// Returns true on success, false if dimensions exceed MAX_MATRIX_DIM, columns are linearly dependent, or arguments are invalid.
bool calc_matrix_qr(const double* A, uint32_t rows, uint32_t cols, double* Q, double* R);

// Solves Ax = b using LU decomposition
// A: dim x dim matrix, b: dim-sized vector, x: dim-sized solution vector
// Returns true on success, false on singularity, invalid dimensions, or failure.
bool calc_solve_linear(const double* A, const double* b, uint32_t dim, double* x);

// Solves least squares problem (minimizes ||Ax - b||_2) using QR decomposition
// A: rows x cols matrix, b: rows-sized vector, x: cols-sized solution vector
// Returns true on success, false on failure or linearly dependent columns.
bool calc_least_squares(const double* A, const double* b, uint32_t rows, uint32_t cols, double* x);

#endif // LINEAR_ALGEBRA_H
