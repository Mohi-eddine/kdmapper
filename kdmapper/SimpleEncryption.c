#include "SimpleEncryption.h"


bool IsSSE42Supported() 
{
	int info[4] = { 0 };

#if defined(_MSC_VER)
	__cpuid(info, 1); // Get CPU feature information (EAX=1)
#else
	__cpuid(1, info[0], info[1], info[2], info[3]);
#endif

	// Check if SSE4.2 is supported (bit 20 of ECX register)
	return (info[2] & (1 << 20)) != 0;
}

uint32_t GetKeyHash(__m128i Key[2])
{
	if (!Key)
		return 0;

	uint8_t* KeyPtr = (uint8_t*)Key;
	size_t KeyElementsCount = sizeof(__m128i) * 2;
	uint32_t Crc32 = 0;
	size_t Index = 0;
	while (Index < KeyElementsCount)
	{
		Crc32 = _mm_crc32_u8(Crc32, KeyPtr[Index]);
		++Index;
	}

	return Crc32;
}

uint32_t KeyGenerator(wchar_t* Password, size_t PasswordSize, __m128i Key[2])
{
	if (!Password || !Key || !PasswordSize)
		return 0;

	uint32_t Crc32 = 0, keyCrc32 = 0;
	size_t Index = 0;
	uint8_t* KeyPtr = (uint8_t*)Key;
	size_t KeyElementsCount = sizeof(__m128i) * 2;
	
	for (size_t i = 0; i < PasswordSize; i++)
	{
		Crc32 = _mm_crc32_u16(Crc32, Password[i]);
	}

	uint32_t Temp = 0;
	do
	{
		if (Temp == 0)
		{
			Crc32 = _mm_crc32_u16(Crc32, Password[Index % PasswordSize]);
			Temp = Crc32;
		}
		uint8_t Val = (uint8_t)(Temp);
		Temp >>= 8;
		int ShiftCount = 0;
		while (ShiftCount < (sizeof(uint32_t) - 1) && !(IsValidKeyChar(Val)))
		{
			Val = (uint8_t)(Temp);
			Temp >>= 8;
			ShiftCount++;
		}

		if (Temp)
		{
			Temp = 0;
			keyCrc32 = _mm_crc32_u8(keyCrc32, Val);
			KeyPtr[Index] = Val;
			Index++;
		}
	} while (Index < KeyElementsCount);

	return keyCrc32;
}

uint32_t RandKeyGenerator(__m128i Key[2])
{
	if (!Key)
	{
		return 0;
	}

	uint32_t Crc32 = 0;
	size_t Index = 0;
	uint8_t* KeyPtr = (uint8_t*)Key;
	size_t KeyElementsCount = sizeof(__m128i) * 2;
	do
	{
		srand((uint32_t)(__rdtsc()));
		uint8_t Val = (uint8_t)(rand());
		if (IsValidKeyChar(Val)/*IsPrintable(Val)*/)
		{
			KeyPtr[Index] = Val;
			Crc32 = _mm_crc32_u8(Crc32, Val);
			Index++;
		}
	} while (Index < KeyElementsCount);
	return Crc32;
}

void ShuffleLeft(__m128i Buffer[2],uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash)
{
	int Shift = InitialShiftValue;
	if(!Shift)
	{
		Shift = (Crc32ShiftingHash % 63) + 1;
	}
	Buffer[0] = RotateLeft128_64(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 8) % 63) + 1;
	Buffer[0] = RotateLeft128_64(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 16) % 31) + 1;
	Buffer[0] = RotateLeft128_32(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 24) % 15) + 1;
	Buffer[0] = RotateLeft128_16(Buffer[0], Shift);

	Crc32ShiftingHash = (Crc32ShiftingHash | (Crc32ShiftingHash << 40)) * 8;
	Shift = ((Crc32ShiftingHash >> 32) % 63) + 1;
	Buffer[1] = RotateLeft128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 40) % 63) + 1;
	Buffer[1] = RotateLeft128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 48) % 31) + 1;
	Buffer[1] = RotateLeft128_32(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 56) % 15) + 1;
	Buffer[1] = RotateLeft128_16(Buffer[1], Shift);
}

void ShuffleRight(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash)
{
	int Shift = InitialShiftValue;
	if (!Shift)
	{
		Shift = (Crc32ShiftingHash % 63) + 1;
	}
	Buffer[0] = RotateRight128_64(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 8) % 63) + 1;
	Buffer[0] = RotateRight128_64(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 16) % 31) + 1;
	Buffer[0] = RotateRight128_32(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 24) % 15) + 1;
	Buffer[0] = RotateRight128_16(Buffer[0], Shift);

	Crc32ShiftingHash = (Crc32ShiftingHash | (Crc32ShiftingHash << 40)) * 8;
	Shift = ((Crc32ShiftingHash >> 32) % 63) + 1;
	Buffer[1] = RotateRight128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 40) % 63) + 1;
	Buffer[1] = RotateRight128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 48) % 31) + 1;
	Buffer[1] = RotateRight128_32(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 56) % 15) + 1;
	Buffer[1] = RotateRight128_16(Buffer[1], Shift);
}

void UndoShuffleLeft(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash)
{

	int Shift = ((Crc32ShiftingHash >> 24) % 15) + 1;
	Buffer[0] = RotateRight128_16(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 16) % 31) + 1;
	Buffer[0] = RotateRight128_32(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 8) % 63) + 1;
	Buffer[0] = RotateRight128_64(Buffer[0], Shift);
	Shift = InitialShiftValue;
	if (!Shift)
	{
		Shift = (Crc32ShiftingHash % 63) + 1;
	}
	Buffer[0] = RotateRight128_64(Buffer[0], Shift);

	Crc32ShiftingHash = (Crc32ShiftingHash | (Crc32ShiftingHash << 40)) * 8;
	Shift = ((Crc32ShiftingHash >> 56) % 15) + 1;
	Buffer[1] = RotateRight128_16(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 48) % 31) + 1;
	Buffer[1] = RotateRight128_32(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 40) % 63) + 1;
	Buffer[1] = RotateRight128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 32) % 63) + 1;
	Buffer[1] = RotateRight128_64(Buffer[1], Shift);

}

void UndoShuffleRight(__m128i Buffer[2], uint32_t InitialShiftValue, uint64_t Crc32ShiftingHash)
{
	int Shift = InitialShiftValue;

	Shift = ((Crc32ShiftingHash >> 24) % 15) + 1;
	Buffer[0] = RotateLeft128_16(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 16) % 31) + 1;
	Buffer[0] = RotateLeft128_32(Buffer[0], Shift);
	Shift = ((Crc32ShiftingHash >> 8) % 63) + 1;
	Buffer[0] = RotateLeft128_64(Buffer[0], Shift);
	Shift = InitialShiftValue;
	if (!Shift)
	{
		Shift = (Crc32ShiftingHash % 63) + 1;
	}
	Buffer[0] = RotateLeft128_64(Buffer[0], Shift);
	
	Crc32ShiftingHash = (Crc32ShiftingHash | (Crc32ShiftingHash << 40)) * 8;
	Shift = ((Crc32ShiftingHash >> 56) % 15) + 1;
	Buffer[1] = RotateLeft128_16(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 48) % 31) + 1;
	Buffer[1] = RotateLeft128_32(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 40) % 63) + 1;
	Buffer[1] = RotateLeft128_64(Buffer[1], Shift);
	Shift = ((Crc32ShiftingHash >> 32) % 63) + 1;
	Buffer[1] = RotateLeft128_64(Buffer[1], Shift);
}

#ifdef ENCRYPT_ENABLE
bool EncryptBytes( uint8_t* Bytes, size_t BytesCount, __m128i Key[2])
{
	if (!Bytes || !Key || !BytesCount)
	{
		return FALSE;
	}
	
	if (BytesCount % 32)
	{
		Print("Error: Buffer Bytes are not aligned to 256 bits (32 bytes).\n");
		return FALSE;
	}

	uint8_t* KeyPtr = (uint8_t*)Key;
	size_t KeyElementsCount = sizeof(__m128i) * 2;

	//size_t KeySize = KeyElementsCount * sizeof(wchar_t);
	__m128i LocalKey[2];
	_mm_storeu_si128(&LocalKey[0], Key[0]);
	_mm_storeu_si128(&LocalKey[1], Key[1]);

	//compute initial key sha256 hash 
	__m128i KeySha256Hash[2];
	SHA256_SIMD_CTX Ctx;
	InitSha256(&Ctx);
	ComputeSha256(&Ctx, (const uint8_t*)Key, KeyElementsCount, KeySha256Hash);
	LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
	LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);

	uint32_t Crc32 = 0, KeyHash = 0;
	for (size_t i = 0; i < KeyElementsCount; ++i)
	{
		Crc32 = (uint32_t)(_mm_crc32_u8(Crc32, KeyPtr[i]));
	}
	KeyHash = Crc32;
	uint64_t ShiftingHash = _mm_crc32_u64(KeyHash, (uint64_t)(KeyHash) << (KeyPtr[KeyElementsCount - 1] % 32)) | ((uint64_t)KeyHash << 32);
	uint32_t Shift = (ShiftingHash % 63) + 1;
	
	size_t Sz = 0;
	__m128i Block[2];
	while (TRUE)
	{
		//generate a key based on both hashes, to be used in encryption rounds
		__m128i RoundKey[2] = { _mm_set1_epi64x(ShiftingHash) ,_mm_set1_epi64x(ShiftingHash) };
		RoundKey[0] = _mm_xor_si128(RoundKey[0], KeySha256Hash[1]);
		RoundKey[1] = _mm_xor_si128(RoundKey[1], KeySha256Hash[0]);

		_mm_storeu_si128(&Block[0], *((__m128i*)(Bytes + Sz)));
		_mm_storeu_si128(&Block[1], *((__m128i*)(Bytes + Sz) + 1));
		size_t ShuffleRounds = (ShiftingHash % 32) + 1;
		uint32_t TempShift = 0;
		//for (size_t i = 0; i <= ShuffleRounds; ++i)
		size_t i = 0;
		do
		{	
			__m128i TempKey[2] = { RoundKey[0] ,RoundKey[1] };
			TempShift = ((ShiftingHash * (i + 1)) % 63) + 1;
			//shuffle key
			ShuffleLeft(TempKey, TempShift, ShiftingHash * ((i + 1) * 2));
			//xor block with the new temp key
			Block[0] = _mm_xor_si128(Block[0], TempKey[0]);
			Block[1] = _mm_xor_si128(Block[1], TempKey[1]);

			if(Shift % 2)//keep this so only use one mode per loop
			{
				ShuffleLeft(Block, TempShift, ShiftingHash * (i + 1));
			}
			else
			{
				ShuffleRight(Block, TempShift, ShiftingHash * (i + 1));
			}
			i++;
		} while (i <= ShuffleRounds);

        Block[0] = _mm_xor_si128(Block[0], LocalKey[0]);
		Block[1] = _mm_xor_si128(Block[1], LocalKey[1]);
		//one last shuffle
		if (Shift % 2)
		{
			ShuffleLeft(Block, Shift, ShiftingHash);
		}
		else
		{
			ShuffleRight(Block, Shift, ShiftingHash);
		}
		_mm_storeu_si128(((__m128i*)(Bytes + Sz)), Block[0]);
		_mm_storeu_si128(((__m128i*)(Bytes + Sz) + 1), Block[1]);

		Sz += sizeof(Block);
		if (Sz >= BytesCount)
			break;
		do
		{
			if (Shift % 8)
			{
				ShuffleRight(KeySha256Hash, Shift, ShiftingHash);
				ShuffleLeft(LocalKey, Shift, ShiftingHash);
			}
			else
			{
				ShuffleLeft(KeySha256Hash, Shift, ShiftingHash);
				ShuffleRight(LocalKey, Shift, ShiftingHash);
			}
	
			LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
			LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);

			ComputeSha256(&Ctx, (const uint8_t*)LocalKey, KeyElementsCount, KeySha256Hash);
		
			ShiftingHash =  _mm_crc32_u64(KeyHash, ShiftingHash) | (_mm_crc32_u64(ShiftingHash, KeyHash + ShiftingHash) << 32);
			Shift = ShiftingHash % 64;//keep it like this we want a 0 to have a chance here 
		} while (!Shift);
	}

	_mm_storeu_si128(&LocalKey[0], Key[0]);
	_mm_storeu_si128(&LocalKey[1], Key[1]);
	//rest sha256 hash
	InitSha256(&Ctx);
	size_t Index = 0;
	
	while (Index < BytesCount)
	{
		if (!(Index % KeyElementsCount))
		{
			//compute new key each 256bits 
			ComputeSha256(&Ctx, (const uint8_t*)LocalKey, KeyElementsCount, KeySha256Hash);
			ShiftingHash = Crc32 | ((uint64_t)Crc32 << 32);
			Shift = (ShiftingHash % 63) + 1;
			if (Shift % 16)
			{
				ShuffleRight(KeySha256Hash, Shift, ShiftingHash);
				ShuffleLeft(LocalKey, Shift, ShiftingHash);
			}
			else
			{
				ShuffleLeft(KeySha256Hash, Shift, ShiftingHash);
				ShuffleRight(LocalKey, Shift, ShiftingHash);
			}
			LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
			LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);
		}

		size_t KeyIndex = Index % KeyElementsCount;
		Crc32 = (_mm_crc32_u16(Crc32, ((uint8_t*)LocalKey)[KeyIndex]) ^ (((uint8_t*)LocalKey)[KeyIndex] | Crc32));
		uint8_t Temp = (uint8_t)(Crc32 % 32);
		Temp ^= (Crc32 >> 16);
		Temp %= 31;
		++Temp;
		Temp = (uint8_t)((_mm_crc32_u16(Crc32, Temp) >> (((uint8_t*)LocalKey)[KeyIndex] % 17)) ^ ((uint8_t*)LocalKey)[KeyIndex]);
		Bytes[Index] ^= Temp;
		Bytes[Index] ^= (uint8_t)(_mm_crc32_u16(Crc32 ^ (Crc32 >> 24), ((uint8_t*)LocalKey)[KeyIndex]));//making this related to the whole integrity of the key bytes
		++Index;
	}

	return TRUE;
}
#endif

bool DecryptBytes(uint8_t* Bytes, size_t BytesCount, __m128i Key[2])
{
	if (!Bytes || !Key || !BytesCount)
	{
		return FALSE;
	}

	if (BytesCount % 32)
	{
		Print("Error: Buffer Bytes are not aligned to 256 bits (32 bytes).\n");
		return FALSE;
	}

	uint8_t* KeyPtr = (uint8_t*)Key;
	size_t KeyElementsCount = sizeof(__m128i) * 2;

	__m128i LocalKey[2];
	_mm_storeu_si128(&LocalKey[0], Key[0]);
	_mm_storeu_si128(&LocalKey[1], Key[1]);

	__m128i KeySha256Hash[2];
	SHA256_SIMD_CTX Ctx;

	uint32_t Crc32 = 0,KeyHash = 0;
	for (size_t i = 0; i < KeyElementsCount; ++i)
	{
		Crc32 = (uint32_t)(_mm_crc32_u8(Crc32, KeyPtr[i]));
	}
	KeyHash = Crc32;

	InitSha256(&Ctx);

	uint64_t ShiftingHash = 0;
	uint32_t Shift = 0;
	size_t Index = 0;
	while (Index < BytesCount)
	{
		if (!(Index % KeyElementsCount))
		{
			//compute new key each 256bits 
			ComputeSha256(&Ctx, (const uint8_t*)LocalKey, KeyElementsCount, KeySha256Hash);
			ShiftingHash = Crc32 | ((uint64_t)Crc32 << 32);
			Shift = (ShiftingHash % 63) + 1;
			if (Shift % 16)
			{
				ShuffleRight(KeySha256Hash, Shift, ShiftingHash);
				ShuffleLeft(LocalKey, Shift, ShiftingHash);
			}
			else
			{
				ShuffleLeft(KeySha256Hash, Shift, ShiftingHash);
				ShuffleRight(LocalKey, Shift, ShiftingHash);
			}
			LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
			LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);
		}

		size_t KeyIndex = Index % KeyElementsCount;
		Crc32 = (_mm_crc32_u16(Crc32, ((uint8_t*)LocalKey)[KeyIndex]) ^ (((uint8_t*)LocalKey)[KeyIndex] | Crc32));

		uint8_t Temp = (uint8_t)(Crc32 % 32);
		Temp ^= (Crc32 >> 16);
		Temp %= 31;
		++Temp;
		Temp = (uint8_t)((_mm_crc32_u16(Crc32, Temp) >> (((uint8_t*)LocalKey)[KeyIndex] % 17)) ^ ((uint8_t*)LocalKey)[KeyIndex]);

		Bytes[Index] ^= (uint8_t)(_mm_crc32_u16(Crc32 ^ (Crc32 >> 24), ((uint8_t*)LocalKey)[KeyIndex]));

		Bytes[Index] ^= Temp;
		++Index;
	}

	ShiftingHash = _mm_crc32_u64(KeyHash, (uint64_t)(KeyHash) << (KeyPtr[KeyElementsCount - 1] % 32)) | ((uint64_t)KeyHash << 32);
	Shift = (ShiftingHash % 63) + 1;

	//rebase everything 
	InitSha256(&Ctx);
	ComputeSha256(&Ctx, (const uint8_t*)Key, KeyElementsCount, KeySha256Hash);
	_mm_storeu_si128(&LocalKey[0], Key[0]);
	_mm_storeu_si128(&LocalKey[1], Key[1]);
	LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
	LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);

	size_t Sz = 0;
	__m128i Block[2];
	while (TRUE)
	{
		__m128i RoundKey[2] = { _mm_set1_epi64x(ShiftingHash) ,_mm_set1_epi64x(ShiftingHash) };
		RoundKey[0] = _mm_xor_si128(RoundKey[0], KeySha256Hash[1]);
		RoundKey[1] = _mm_xor_si128(RoundKey[1], KeySha256Hash[0]);

		_mm_storeu_si128(&Block[0], *((__m128i*)(Bytes + Sz)));
		_mm_storeu_si128(&Block[1], *((__m128i*)(Bytes + Sz) + 1));
		//first undo the last shuffle
		if (Shift % 2)
		{
			UndoShuffleLeft(Block, Shift, ShiftingHash);
		}
		else
		{
			UndoShuffleRight(Block, Shift, ShiftingHash);
		}
		//then xor with the local key that was generated with sha256 help
		Block[0] = _mm_xor_si128(Block[0], LocalKey[0]);
		Block[1] = _mm_xor_si128(Block[1], LocalKey[1]);

		size_t ShuffleRounds = (ShiftingHash % 32) + 1;
		uint32_t TempShift = 0;
		size_t i = 0, CountDown = ShuffleRounds;
		do
		{
			TempShift = ((ShiftingHash * (CountDown + 1)) % 63) + 1;
			//then undo the shuffle
			if (Shift % 2)//keep this so only use one mode per loop
			{
				UndoShuffleLeft(Block, TempShift, ShiftingHash * (CountDown + 1));
			}
			else
			{
				UndoShuffleRight(Block, TempShift, ShiftingHash * (CountDown + 1));
			}
			__m128i TempKey[2] = { RoundKey[0] ,RoundKey[1] };
			//then xor with the round key
			//shuffle key
			ShuffleLeft(TempKey, TempShift, ShiftingHash * ((CountDown + 1) * 2));
			Block[0] = _mm_xor_si128(Block[0], TempKey[0]);
			Block[1] = _mm_xor_si128(Block[1], TempKey[1]);
			CountDown--;
			i++;
		}while (i <= ShuffleRounds);
		
		//store back
		_mm_storeu_si128(((__m128i*)(Bytes + Sz)), Block[0]);
		_mm_storeu_si128(((__m128i*)(Bytes + Sz) + 1), Block[1]);

		Sz += sizeof(Block);
		if (Sz >= BytesCount)
			break;
		do
		{
			if (Shift % 8)
			{
				ShuffleRight(KeySha256Hash, Shift, ShiftingHash);
				ShuffleLeft(LocalKey, Shift, ShiftingHash);
			}
			else
			{
				ShuffleLeft(KeySha256Hash, Shift, ShiftingHash);
				ShuffleRight(LocalKey, Shift, ShiftingHash);
			}

			LocalKey[0] = _mm_xor_si128(LocalKey[0], KeySha256Hash[0]);
			LocalKey[1] = _mm_xor_si128(LocalKey[1], KeySha256Hash[1]);

			ComputeSha256(&Ctx, (const uint8_t*)LocalKey, KeyElementsCount, KeySha256Hash);

			ShiftingHash = _mm_crc32_u64(KeyHash, ShiftingHash) | (_mm_crc32_u64(ShiftingHash, KeyHash + ShiftingHash) << 32);
			Shift = ShiftingHash % 64;//keep it like this we want a 0 to have a chance here 
		} while (!Shift);
	}
	return TRUE;
}