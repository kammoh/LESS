#ifndef SORT_H
#define SORT_H

#include "codes.h"

////////////////////////////////////////////////////////////////////////
///                             Sorting                              ///
////////////////////////////////////////////////////////////////////////
int fqcmp(const void *a,const void *b);


void bitonic_sort_i8(FQ_ELEM *x, const long long n);
void counting_sort_u8(FQ_ELEM *arr, const uint32_t size);

#ifdef USE_AVX2
void sortingnetwork(uint8_t *arr, const size_t size);
#endif

void sort(uint8_t *out, const uint8_t *in, const uint32_t len);

int compare_rows(const FQ_ELEM *row1, const FQ_ELEM *row2);

int SortRows(normalized_IS_t *G, const uint32_t n, const uint8_t *L);
int SortRows_internal(FQ_ELEM *ptr[K],
                            uint32_t P[K],
                            const uint32_t n);

void col_bitonic_sort_transpose(normalized_IS_t *V);
void SortCols(normalized_IS_t *V,
                             const uint32_t z);
void col_quicksort(normalized_IS_t *V,
                   const uint32_t z);
#endif //SORT_H
