#include "ulti.h"

int exists_in_arr(int *arr, int size, int k)
{
	for (int i = 0; i < size; ++i) {
		if (arr[i] == k)
			return 1;
	}
	return 0;
}