// LibMath.dll
//
// (C)Kuina-chan
//

#include "main.h"

typedef struct SMat
{
	SClass Class;
	S64 Row;
	S64 Col;
	double* Buf;
} SMat;

typedef struct IntList
{
	S64 Value;
	struct IntList* Next;
} IntList;

static const S64 Primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 };
static const S64 Facts[21] =
{
	1, 1, 2, 6, 24,
	120, 720, 5040, 40320, 362880,
	3628800, 39916800, 479001600, 6227020800, 87178291200,
	1307674368000, 20922789888000, 355687428096000, 6402373705728000, 121645100408832000,
	2432902008176640000
};
static const S64 FibonacciNumbers[93] =
{
	0, 1, 1, 2, 3,
	5, 8, 13, 21, 34,
	55, 89, 144, 233, 377,
	610, 987, 1597, 2584, 4181,
	6765, 10946, 17711, 28657, 46368,
	75025, 121393, 196418, 317811, 514229,
	832040, 1346269, 2178309, 3524578, 5702887,
	9227465, 14930352, 24157817, 39088169, 63245986,
	102334155, 165580141, 267914296, 433494437, 701408733,
	1134903170, 1836311903, 2971215073, 4807526976, 7778742049,
	12586269025, 20365011074, 32951280099, 53316291173, 86267571272,
	139583862445, 225851433717, 365435296162, 591286729879, 956722026041,
	1548008755920, 2504730781961, 4052739537881, 6557470319842, 10610209857723,
	17167680177565, 27777890035288, 44945570212853, 72723460248141, 117669030460994,
	190392490709135, 308061521170129, 498454011879264, 806515533049393, 1304969544928657,
	2111485077978050, 3416454622906707, 5527939700884757, 8944394323791464, 14472334024676221,
	23416728348467685, 37889062373143906, 61305790721611591, 99194853094755497, 160500643816367088,
	259695496911122585, 420196140727489673, 679891637638612258, 1100087778366101931, 1779979416004714189,
	2880067194370816120, 4660046610375530309, 7540113804746346429,
};

static U64 ModPow(U64 value, U64 exponent, U64 modulus);
static U64 ModMul(U64 a, U64 b, U64 modulus);
static S64 FindFactor(S64 n, U32 seed);
static int CmpInt(const void* a, const void* b);

Bool AddAsm(S64* a, S64 b);
Bool SubAsm(S64* a, S64 b);
Bool MulAsm(S64* a, S64 b);
const U8* GetPrimesBin(size_t* size);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;
}

EXPORT S64 _gcd(S64 a, S64 b)
{
	if (a == 0)
	{
		THROWDBG(b == 0, EXCPT_DBG_ARG_OUT_DOMAIN);
		return b;
	}
	if (b == 0)
		return a;
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;
	S64 r;
	if (b > a)
	{
		r = a;
		a = b;
		b = r;
	}
	for (; ; )
	{
		r = a % b;
		if (r == 0)
			return b;
		a = b;
		b = r;
	}
}

EXPORT S64 _lcm(S64 a, S64 b)
{
	if (a == 0)
	{
		THROWDBG(b == 0, EXCPT_DBG_ARG_OUT_DOMAIN);
		return 0;
	}
	if (b == 0)
		return 0;
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;
	return a / _gcd(a, b) * b;
}

EXPORT S64 _modPow(S64 value, S64 exponent, S64 modulus)
{
	THROWDBG(value < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(exponent < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(modulus < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	return (S64)ModPow((U64)value, (U64)exponent, (U64)modulus);
}

EXPORT S64 _modMul(S64 a, S64 b, S64 modulus)
{
	THROWDBG(a < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(b < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(modulus < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	return (S64)ModMul((U64)a, (U64)b, (U64)modulus);
}

EXPORT Bool _prime(S64 n)
{
	if (n <= 1)
		return False;
	if ((n & 1) == 0)
		return n == 2;
	if (n <= 1920000)
	{
		if (n == 3)
			return True;
		if (n % 6 != 1 && n % 6 != 5)
			return False;
		S64 m = n / 6 * 2 + (n % 6 == 1 ? 0 : 1);
		size_t size;
		const U8* primes_bin = GetPrimesBin(&size);
		return (primes_bin[m / 8] & (1 << (m % 8))) != 0;
	}

	// Miller-Rabin primality test.
	U64 enough;
	if (n < 2047)
		enough = 1;
	else if (n < 1373653)
		enough = 2;
	else if (n < 25326001)
		enough = 3;
	else if (n < 3215031751)
		enough = 4;
	else if (n < 2152302898747)
		enough = 5;
	else if (n < 3474749660383)
		enough = 6;
	else if (n < 341550071728321)
		enough = 7;
	else if (n < 3825123056546413051)
		enough = 9;
	else
	{
		// n < 2^64 < 318665857834031151167461
		enough = 12;
	}
	{
		U64 d = (U64)n - 1;
		U64 s = 0;
		while ((d & 1) == 0)
		{
			s++;
			d >>= 1;
		}
		for (U64 i = 0; i < enough; i++)
		{
			U64 x = ModPow(Primes[i], d, (U64)n);
			U64 j;
			if (x == 1 || x == (U64)n - 1)
				continue;
			Bool probablyPrime = False;
			for (j = 0; j < s; j++)
			{
				x = ModPow(x, 2, (U64)n);
				if (x == (U64)n - 1)
				{
					probablyPrime = True;
					break;
				}
			}
			if (!probablyPrime)
				return False;
		}
		return True;
	}
}

EXPORT void* _primeFactors(S64 n)
{
	IntList* top = NULL;
	IntList* bottom = NULL;
	int num = 0;
	while (n > 1)
	{
		S64 factor = FindFactor(n, 1);
		IntList* node = (IntList*)AllocMem(sizeof(IntList));
		node->Value = factor;
		node->Next = NULL;
		if (top == NULL)
		{
			top = node;
			bottom = node;
		}
		else
		{
			bottom->Next = node;
			bottom = node;
		}
		num++;
		n /= factor;
	}
	U8* result = (U8*)AllocMem(0x10 + sizeof(S64) * (size_t)num);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)num;
	{
		IntList* ptr = top;
		S64* dst = (S64*)result + 2;
		while (ptr != NULL)
		{
			IntList* ptr2 = ptr;
			ptr = ptr->Next;
			*dst = ptr2->Value;
			dst++;
			FreeMem(ptr2);
		}
	}
	qsort((S64*)result + 2, (size_t)num, sizeof(S64), CmpInt);
	return result;
}

EXPORT double _gamma(double n)
{
	return tgamma(n);
}

EXPORT double _fact(double n)
{
	return tgamma(n + 1.0);
}

EXPORT S64 _factInt(S64 n)
{
	if (n < 0)
	{
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
		return 0;
	}
	if (n > 20)
	{
		THROWDBG(True, EXCPT_DBG_INT_OVERFLOW);
		return 0;
	}
	return Facts[n];
}

EXPORT S64 _fibonacci(S64 n)
{
	if (n < 0)
	{
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
		return 0;
	}
	if (n > 92)
	{
		THROWDBG(True, EXCPT_DBG_INT_OVERFLOW);
		return 0;
	}
	return FibonacciNumbers[n];
}

EXPORT S64 _knapsack(const void* weights, const void* values, S64 max_weight, Bool reuse)
{
	THROWDBG(weights == NULL || values == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(S64*)((U8*)weights + 0x08) != *(S64*)((U8*)values + 0x08), EXCPT_DBG_ARG_OUT_DOMAIN);
	S64 len = *(S64*)((U8*)weights + 0x08);
	const S64* weights2 = (S64*)((U8*)weights + 0x10);
	const S64* values2 = (S64*)((U8*)values + 0x10);
	S64* dp = (S64*)AllocMem(sizeof(S64) * (size_t)(max_weight + 1));
	S64 i, j;
#if defined(DBG)
	for (i = 0; i < len; i++)
		THROWDBG(weights2[i] <= 0, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif
	memset(dp, 0, sizeof(S64) * (size_t)(max_weight + 1));
	if (reuse)
	{
		for (i = 0; i < len; i++)
		{
			for (j = weights2[i]; j <= max_weight; j++)
			{
				S64 value = dp[j - weights2[i]] + values2[i];
				if (dp[j] < value)
					dp[j] = value;
			}
		}
	}
	else
	{
		for (i = 0; i < len; i++)
		{
			for (j = max_weight; j >= weights2[i]; j--)
			{
				S64 value = dp[j - weights2[i]] + values2[i];
				if (dp[j] < value)
					dp[j] = value;
			}
		}
	}
	S64 result = dp[max_weight];
	FreeMem(dp);
	return result;
}

EXPORT void* _dijkstra(S64 node_num, const void* from_nodes, const void* to_nodes, const void* values, S64 begin_node)
{
	THROWDBG(from_nodes == NULL || to_nodes == NULL || values == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(S64*)((U8*)from_nodes + 0x08) != *(S64*)((U8*)to_nodes + 0x08) || *(S64*)((U8*)to_nodes + 0x08) != *(S64*)((U8*)values + 0x08), EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(node_num <= 0 || begin_node < 0 || node_num <= begin_node, EXCPT_DBG_ARG_OUT_DOMAIN);
	const S64* from_nodes2 = (S64*)((U8*)from_nodes + 0x10);
	const S64* to_nodes2 = (S64*)((U8*)to_nodes + 0x10);
	const S64* values2 = (S64*)((U8*)values + 0x10);
	S64 len = *(S64*)((U8*)from_nodes + 0x08);
	S64 i;
#if defined(DBG)
	for (i = 0; i < len; i++)
		THROWDBG(from_nodes2[i] < 0 || node_num <= from_nodes2[i] || to_nodes2[i] < 0 || node_num <= to_nodes2[i] || values2[i] < 0, EXCPT_DBG_ARG_OUT_DOMAIN);
#endif

	U8* result = (U8*)AllocMem(0x10 + sizeof(S64) * (size_t)node_num);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = node_num;
	S64* distance = (S64*)(result + 0x10);
	for (i = 0; i < node_num; i++)
		distance[i] = LLONG_MAX;
	distance[begin_node] = 0;

	S64* heap = (S64*)AllocMem(sizeof(S64) * (2 * (size_t)(node_num * len) + 2));
	int heap_cnt = 1;
	heap[0] = begin_node;
	heap[1] = 0;
	while (heap_cnt > 0)
	{
		S64 item_node = heap[0];
		S64 item_value = heap[1];
		heap_cnt--;
		heap[0] = heap[heap_cnt * 2];
		heap[1] = heap[heap_cnt * 2 + 1];
		{
			S64 del_idx = 0;
			for (; ; )
			{
				if ((del_idx + 1) * 2 - 1 < heap_cnt && heap[del_idx * 2 + 1] > heap[((del_idx + 1) * 2 - 1) * 2 + 1])
				{
					S64 tmp;
					tmp = heap[del_idx * 2];
					heap[del_idx * 2] = heap[((del_idx + 1) * 2 - 1) * 2];
					heap[((del_idx + 1) * 2 - 1) * 2] = tmp;
					tmp = heap[del_idx * 2 + 1];
					heap[del_idx * 2 + 1] = heap[((del_idx + 1) * 2 - 1) * 2 + 1];
					heap[((del_idx + 1) * 2 - 1) * 2 + 1] = tmp;
					del_idx = (del_idx + 1) * 2 - 1;
				}
				else if ((del_idx + 1) * 2 < heap_cnt && heap[del_idx * 2 + 1] > heap[((del_idx + 1) * 2) * 2 + 1])
				{
					S64 tmp;
					tmp = heap[del_idx * 2];
					heap[del_idx * 2] = heap[((del_idx + 1) * 2) * 2];
					heap[((del_idx + 1) * 2) * 2] = tmp;
					tmp = heap[del_idx * 2 + 1];
					heap[del_idx * 2 + 1] = heap[((del_idx + 1) * 2) * 2 + 1];
					heap[((del_idx + 1) * 2) * 2 + 1] = tmp;
					del_idx = (del_idx + 1) * 2;
				}
				else
					break;
			}
		}
		if (distance[item_node] < item_value)
			continue;
		for (i = 0; i < len; i++)
		{
			if (from_nodes2[i] != item_node)
				continue;
			if (distance[to_nodes2[i]] > distance[item_node] + values2[i])
			{
				distance[to_nodes2[i]] = distance[item_node] + values2[i];
				heap[heap_cnt * 2] = to_nodes2[i];
				heap[heap_cnt * 2 + 1] = distance[to_nodes2[i]];
				{
					S64 ins_idx = heap_cnt;
					for (; ; )
					{
						if (ins_idx > 0 && heap[ins_idx * 2 + 1] < heap[(ins_idx - 1) / 2 * 2 + 1])
						{
							S64 tmp;
							tmp = heap[ins_idx * 2];
							heap[ins_idx * 2] = heap[(ins_idx - 1) / 2 * 2];
							heap[(ins_idx - 1) / 2 * 2] = tmp;
							tmp = heap[ins_idx * 2 + 1];
							heap[ins_idx * 2 + 1] = heap[(ins_idx - 1) / 2 * 2 + 1];
							heap[(ins_idx - 1) / 2 * 2 + 1] = tmp;
							ins_idx = (ins_idx - 1) / 2;
						}
						else
							break;
					}
				}
				heap_cnt++;
			}
		}
	}
	FreeMem(heap);

	return result;
}

EXPORT void* _bellmanFord(S64 node_num, const void* from_nodes, const void* to_nodes, const void* values, S64 begin_node)
{
	THROWDBG(from_nodes == NULL || to_nodes == NULL || values == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(S64*)((U8*)from_nodes + 0x08) != *(S64*)((U8*)to_nodes + 0x08) || *(S64*)((U8*)to_nodes + 0x08) != *(S64*)((U8*)values + 0x08), EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(node_num <= 0 || begin_node < 0 || node_num <= begin_node, EXCPT_DBG_ARG_OUT_DOMAIN);
	const S64* from_nodes2 = (S64*)((U8*)from_nodes + 0x10);
	const S64* to_nodes2 = (S64*)((U8*)to_nodes + 0x10);
	const S64* values2 = (S64*)((U8*)values + 0x10);
	S64 len = *(S64*)((U8*)from_nodes + 0x08);
	S64 i;
#if defined(DBG)
	for (i = 0; i < len; i++)
		THROWDBG(from_nodes2[i] < 0 || node_num <= from_nodes2[i] || to_nodes2[i] < 0 || node_num <= to_nodes2[i], EXCPT_DBG_ARG_OUT_DOMAIN);
#endif

	U8* result = (U8*)AllocMem(0x10 + sizeof(S64) * (size_t)node_num);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = node_num;
	S64* distance = (S64*)(result + 0x10);
	for (i = 0; i < node_num; i++)
		distance[i] = LLONG_MAX;
	distance[begin_node] = 0;

	Bool found;
	do
	{
		found = False;
		for (i = 0; i < len; i++)
		{
			S64 from_distance = distance[from_nodes2[i]];
			if (from_distance != LLONG_MAX && distance[to_nodes2[i]] > from_distance + values2[i])
			{
				distance[to_nodes2[i]] = from_distance + values2[i];
				found = True;
			}
		}
	} while (found);
	return result;
}

EXPORT void* _floydWarshall(S64 node_num, const void* from_nodes, const void* to_nodes, const void* values)
{
	THROWDBG(from_nodes == NULL || to_nodes == NULL || values == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(S64*)((U8*)from_nodes + 0x08) != *(S64*)((U8*)to_nodes + 0x08) || *(S64*)((U8*)to_nodes + 0x08) != *(S64*)((U8*)values + 0x08), EXCPT_DBG_ARG_OUT_DOMAIN);
	THROWDBG(node_num <= 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	const S64* from_nodes2 = (S64*)((U8*)from_nodes + 0x10);
	const S64* to_nodes2 = (S64*)((U8*)to_nodes + 0x10);
	const S64* values2 = (S64*)((U8*)values + 0x10);
	S64 len = *(S64*)((U8*)from_nodes + 0x08);
	S64 i, j, k;
#if defined(DBG)
	for (i = 0; i < len; i++)
		THROWDBG(from_nodes2[i] < 0 || node_num <= from_nodes2[i] || to_nodes2[i] < 0 || node_num <= to_nodes2[i], EXCPT_DBG_ARG_OUT_DOMAIN);
#endif

	U8* result = (U8*)AllocMem(0x10 + sizeof(void**) * (size_t)node_num);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = node_num;
	void** ptr = (void**)(result + 0x10);
	for (i = 0; i < node_num; i++)
	{
		void* item = AllocMem(0x10 + sizeof(S64) * (size_t)node_num);
		((S64*)item)[0] = 1;
		((S64*)item)[1] = node_num;
		S64* ptr2 = (S64*)item + 2;
		for (j = 0; j < node_num; j++)
			ptr2[j] = i == j ? 0 : LLONG_MAX;
		ptr[i] = item;
	}
	for (i = 0; i < len; i++)
		((S64*)ptr[from_nodes2[i]] + 2)[to_nodes2[i]] = values2[i];
	for (i = 0; i < node_num; i++)
	{
		for (j = 0; j < node_num; j++)
		{
			for (k = 0; k < node_num; k++)
			{
				S64 a = ((S64*)ptr[j] + 2)[i];
				S64 b = ((S64*)ptr[i] + 2)[k];
				if (a == LLONG_MAX || b == LLONG_MAX)
					continue;
				S64 value = a + b;
				if (((S64*)ptr[j] + 2)[k] > value)
					((S64*)ptr[j] + 2)[k] = value;
			}
		}
	}

	return result;
}

EXPORT SClass* _makeMat(SClass* me_, S64 row, S64 col)
{
	THROWDBG(row <= 0 || col <= 0, EXCPT_DBG_ARG_OUT_DOMAIN);
	SMat* me2 = (SMat*)me_;
	me2->Row = row;
	me2->Col = col;
	me2->Buf = (double*)AllocMem(sizeof(double) * (size_t)row * (size_t)col);
	S64 i;
	S64 j;
	for (i = 0; i < row; i++)
	{
		for (j = 0; j < col; j++)
			me2->Buf[i * col + j] = 0.0;
	}
	return me_;
}

EXPORT void _matDtor(SClass* me_)
{
	SMat* me2 = (SMat*)me_;
	FreeMem(me2->Buf);
}

static U64 ModPow(U64 value, U64 exponent, U64 modulus)
{
	U64 w = 1;
	while (exponent > 0)
	{
		if ((exponent & 1) != 0)
			w = ModMul(w, value, modulus);
		value = ModMul(value, value, modulus);
		exponent >>= 1;
	}
	return w;
}

static U64 ModMul(U64 a, U64 b, U64 modulus)
{
	{
		S64 a2 = (S64)a;
		if (a2 >= 0)
		{
			S64 b2 = (S64)b;
			if (b2 >= 0)
			{
				if (!MulAsm(&a2, b2))
					return (U64)a2 % modulus;
			}
		}
	}
	U64 result = 0;
	while (a != 0)
	{
		if ((a & 1) != 0)
			result = (result + b) % modulus;
		a >>= 1;
		b = (b << 1) % modulus;
	}
	return result;
}

static S64 FindFactor(S64 n, U32 seed)
{
	// Pollard's rho algorithm.
	U64 n2 = (S64)n;
	for (; ; )
	{
		if (n2 % 2 == 0)
			return 2;
		if (_prime((S64)n2))
			return (S64)n2;
		U64 a = (U64)XorShift(&seed) << 32;
		a |= (U64)XorShift(&seed);
		U64 y = a % (n2 + 1);
		U64 c = (U64)XorShift(&seed) + 1;
		U64 m = (U64)XorShift(&seed) + 1;
		U64 g;
		U64 r = 1;
		U64 q = 1;
		U64 ys = 0;
		U64 x;
		U64 i;
		do
		{
			x = y;
			for (i = 0; i < r; i++)
				y = (ModMul(y, y, n2) + c) % n2;
			U64 k = 0;
			do
			{
				ys = y;
				U64 min_value = m < r - k ? m : r - k;
				for (i = 0; i < min_value; i++)
				{
					y = (ModMul(y, y, n2) + c) % n2;
					q = ModMul(q, x > y ? x - y : y - x, n2);
				}
				g = _gcd((S64)q, (S64)n2);
				k += m;
			} while (k < r && g <= 1);
			r *= 2;
		} while (g <= 1);
		if (g == n2)
		{
			do
			{
				ys = (ModMul(ys, ys, n2) + c) % n2;
				g = _gcd((S64)(x > ys ? x - ys : ys - x), (S64)n2);
			} while (g <= 1);
		}
		if (g == n2)
			seed++;
		else
			n2 = g;
	}
}

static int CmpInt(const void* a, const void* b)
{
	return *(S64*)a > *(S64*)b ? 1 : -1;
}
