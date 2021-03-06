#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <mmintrin.h>
#include <x86intrin.h>
#include "timer.c"

#define N_ 4096
#define K_ 4096
#define M_ 4096
#define bsize_ 512
#define BLOCk_SIZE 16

typedef double dtype;

int bsize;

void verify(dtype *C, dtype *C_ans, int N, int M)
{
  int i, cnt;
  cnt = 0;
  for(i = 0; i < N * M; i++) {
    if(abs (C[i] - C_ans[i]) > 1e-6) cnt++;
  }
  if(cnt != 0) printf("ERROR\n"); else printf("SUCCESS\n");
}

void mm_serial (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  int i, j, k;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < M; j++) {
      for(int k = 0; k < K; k++) {
        C[i * M + j] += A[i * K + k] * B[k * M + j];
      }
    }
  }
}
void mm_vector (dtype *C, dtype *A, dtype *B, int N)
{
  int i, j, k;
  __m128d a_vec, b_vec, mult_vec;
      double z[2] = {0.0, 0.0};
  double c[2];
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      mult_vec = _mm_load_pd(z);
      for(int k = 0; k < N; k += 2) {
        a_vec = _mm_load_pd(A + (i * N) + k);
        b_vec = _mm_load_pd(B + (j * N) + k);
        mult_vec = _mm_add_pd(_mm_mul_pd(a_vec, b_vec), mult_vec);
      }
      _mm_store_pd(c, mult_vec);
      C[i * N + j] += c[0] + c[1];
    }
  }
}

double min (double a, double b){
  //double c;
  if ( a == b){
    return a;
  }else{
    return (a < b) ? a : b;
  }
}

void mm_cb (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  /* =======================================================+ */
  /* Implement your own cache-blocked matrix-matrix multiply  */
  /* =======================================================+ */
  // Block A : N ROWS and K COLUMNS
  // Block B : K ROWS and M COLUMNS
  // Block C : N ROWS and M COLUMNS
  // will assume that Array C is 0


  double sum;
  int i, j, k, jj, kk;
  for (jj = 0; jj < M; jj += bsize){
    for (kk = 0; kk < K; kk += bsize){
      for (i = 0; i < N; i ++){
        for (j = jj; j < min(jj + bsize, M); j++){
          sum = 0.0;
          for (k = kk; k < min(kk + bsize, K); k++){
            sum += A[i * K + k] * B[k * M + j];
          }
          C[i * M + j] += sum;
        }
      }
    }
  }

}

void mm_sv (dtype *C, dtype *A, dtype *B, int N, int K, int M)
{
  /* =========================================================+ */
  /* Implement your own SIMD-vectorized matrix-matrix multiply  */
  /* =========================================================+ */
  // Block A : N ROWS and K COLUMNS
  // Block B : K ROWS and M COLUMNS
  // Block C : N ROWS and M COLUMNS
  // will assume that Array C is 0

  // dot product?
  // loadh and loadl?
  // loadh sets upper half
  // loadl sets lower half

  //int bsize = 850;
  /*
  __m128d vsum, vx1, vx2;
  double sum;
  int i, j, k, jj, kk;
  for (jj = 0; jj < M; jj += bsize){
    for (kk = 0; kk < K; kk += bsize){
      for (i = 0; i < N; i ++){
        for (j = jj; j < min(jj + bsize, M); j++){
          sum = 0.0;
          vsum = __mm_setzero_pd();
          for (k = kk; k < min(kk + bsize, K); k++){
            vx1 = __mm_load_pd(A[i * K + k]);
            __mm_loadh_pd(vx2, B[k * M + j]);
            __mm_loadl_pd(vx2, B[k+1 * M + j]);
            sum += A[i * K + k] * B[k * M + j];
          }
          C[i * M + j] += sum;
        }
      }
    }
  }*/
{
  int i,j,k;
  int row, column;
  dtype *A_temp = (dtype*) malloc (BLOCk_SIZE * BLOCk_SIZE * sizeof (dtype));
  dtype *B_temp = (dtype*) malloc (BLOCk_SIZE * BLOCk_SIZE * sizeof (dtype));
  dtype *C_temp = (dtype*) malloc (BLOCk_SIZE * BLOCk_SIZE * sizeof (dtype));

  for(int i = 0; i < N; i+=BLOCk_SIZE)
  {
    for(int j = 0; j < M; j+=BLOCk_SIZE)
    {
      for(row = 0; row < BLOCk_SIZE; row++)
      {
        for(column = 0; column < BLOCk_SIZE; column++)
        {
          C_temp[row*BLOCk_SIZE + column] = C[(i+row)*M + j + column];
        }
      }
      for(int k = 0; k < K; k+=BLOCk_SIZE)
      {
        for(row = 0; row < BLOCk_SIZE; row++)
        {
          for(column = 0; column < BLOCk_SIZE; column++)
          {
            A_temp[row*BLOCk_SIZE + column] = A[(i+row)*K + k + column];
          }
        }
        for(row = 0; row < BLOCk_SIZE; row++)
        {
          for(column = 0; column < BLOCk_SIZE; column++)
          {
            B_temp[column*BLOCk_SIZE + row] = B[(k+row)*M + j + column];
          }
        }
        mm_vector(C_temp, A_temp, B_temp, BLOCk_SIZE);
      }
      for(row = 0; row < BLOCk_SIZE; row++)
      {
        for(column = 0; column < BLOCk_SIZE; column++)
        {
          C[(i+row)*M + j + column] = C_temp[row*BLOCk_SIZE + column];
        }
      }
    }
  }
}

}

int main(int argc, char** argv)
{
  int i, j, k;
  int N, K, M;

  if(argc == 5) {
    N = atoi (argv[1]);
    K = atoi (argv[2]);
    M = atoi (argv[3]);
    bsize = atoi (argv[4]);
    printf("N: %d K: %d M: %d\n", N, K, M);
  } else {
    N = N_;
    K = K_;
    M = M_;
    bsize = bsize_;
    printf("N: %d K: %d M: %d\n", N, K, M);
  }

  dtype *A = (dtype*) malloc (N * K * sizeof (dtype));
  dtype *B = (dtype*) malloc (K * M * sizeof (dtype));
  dtype *C = (dtype*) malloc (N * M * sizeof (dtype));
  dtype *C_cb = (dtype*) malloc (N * M * sizeof (dtype));
  dtype *C_sv = (dtype*) malloc (N * M * sizeof (dtype));
  assert (A && B && C);

  /* initialize A, B, C */
  srand48 (time (NULL));
  for(i = 0; i < N; i++) {
    for(j = 0; j < K; j++) {
      A[i * K + j] = drand48 ();
    }
  }
  for(i = 0; i < K; i++) {
    for(j = 0; j < M; j++) {
      B[i * M + j] = drand48 ();
    }
  }
  bzero(C, N * M * sizeof (dtype));
  bzero(C_cb, N * M * sizeof (dtype));
  bzero(C_sv, N * M * sizeof (dtype));

  stopwatch_init ();
  struct stopwatch_t* timer = stopwatch_create ();
  assert (timer);
  long double t;

  printf("Naive matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_serial (C, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for naive implementation: %Lg seconds\n\n", t);


  printf("Cache-blocked matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_cb (C_cb, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for cache-blocked implementation: %Lg seconds\n", t);

  /* verify answer */
  verify (C_cb, C, N, M);

  printf("SIMD-vectorized Cache-blocked matrix multiply\n");
  stopwatch_start (timer);
  /* do C += A * B */
  mm_sv (C_sv, A, B, N, K, M);
  t = stopwatch_stop (timer);
  printf("Done\n");
  printf("time for SIMD-vectorized cache-blocked implementation: %Lg seconds\n", t);

  /* verify answer */
  verify (C_sv, C, N, M);

  return 0;
}
