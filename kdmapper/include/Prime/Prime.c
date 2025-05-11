#include "Prime.h"


uint64_t generate_random_with_crc32(uint64_t init_value)
{
	// Generate two 32-bit random numbers

	srand((uint32_t)(__rdtsc()));
	uint64_t random_part1 = rand() | __rdtsc();
	srand((uint32_t)(__rdtsc()));
	uint64_t random_part2 = rand() | __rdtsc();

	uint64_t combined_random = random_part1 + random_part2;

	uint64_t crc32_value = _mm_crc32_u64(init_value, combined_random);
    crc32_value |= (_mm_crc32_u64(crc32_value, combined_random ^ crc32_value) << 32);
	return combined_random ^ crc32_value;
}

DWORD WINAPI find_prime_number_thread(LPVOID Data)//local function
{
    PDATA_TYPE _Data = (PDATA_TYPE)Data;
    srand((uint32_t)(__rdtsc()));
    
    gint Prime;
    ggint_rand(Prime);
    ggint_shr(Prime, _Data->ShiftValue);

    uint32_t Add = 1;
    size_t Count = 0;
    while (!_Data->IsFound && !ggint_is_prime_fermat(Prime, 10))
    {   
        if (!(Count % 40))
        {
            srand((uint32_t)(__rdtsc()));   
            ggint_rand(Prime);
            ggint_shr(Prime, _Data->ShiftValue);
            ((uint64_t*)Prime)[0] += 1;
            Prime[0] |= 1;
        }
        else
        {
            Add = rand();
            ((uint64_t*)Prime)[0] += Add;
            Prime[0] |= 1;
        }
        ++Count;
    }
    
    if (!_interlockedbittestandset(&_Data->IsFound, 0))
    {  
        memcpy(_Data->data, Prime, sizeof(gint));
        //_Data->IsFound = true;
    }

    InterlockedAdd64((volatile int64_t*) &_Data->ncheck, Count);

    _InterlockedIncrement64((volatile int64_t*)&_Data->DoneThreadCount);
    return 0;
}

void generate_prime(size_t NBits, gint Prime)
{
    size_t shift = (sizeof(gint))-(NBits / 8);//((NBits / 8) % (GInt_Size / 2));
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    size_t ncheck = 0;
    ggint_rand(Prime);
    ggint_shr(Prime, shift);
    DATA_TYPE Data = {
        .data = Prime,
        .NBits = NBits,
        .ShiftValue = shift,
        .ncheck = 0,
        .DoneThreadCount = 0,
        .IsFound = false,
    };
    
    //find_prime_number_thread(&Data);
    size_t thread_div = 64;
    for (int i = 0; i < max(5, NBits / thread_div); i++)
    {
        HANDLE hThread = CreateThread(NULL, 0, find_prime_number_thread, &Data, 0, NULL);
        if (hThread)
            CloseHandle(hThread);
    }

    while (!Data.IsFound || (Data.DoneThreadCount != max(5, NBits / thread_div)))
    {
        Sleep(0);
    }
    ggint_reset_cash();
    ncheck = Data.ncheck;

    printf("\n\n>> %zu-Bits Prime: ", NBits); 
    ggint_print(Prime);
    printf("\n\n");

    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Checked %d numbers in %d ms: %g num/sec\n", (int)ncheck, (int)cpu_time_used, 1000.0 * ((double)(ncheck)) / cpu_time_used);
}
