#include "bitmap.h"

#include <stdio.h>

#define mask 0x01 // 00000001

//restituisco se il bit in pos è 0 o 1
int BitMap_getBit(BitMap* bmp, int pos){
	if(pos >= bmp->num_bits) return -1;
	BitMapEntryKey map = BitMap_blockToIndex(pos);
	return bmp->entries[map.entry_num] >> map.bit_num & mask;
}

BitMapEntryKey BitMap_blockToIndex(int num){
	BitMapEntryKey map;
	int byte = num / 8; // vedi disegno fogli
	map.entry_num = byte;
	char offset = num - (byte * 8);
	map.bit_num = offset;
	return map;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
	if (entry < 0 || bit_num < 0) return -1;
	return (entry * 8) + bit_num;
}

//parto da start, se start ha lo status che voglio lo ritorno, 
//se no vado avanti fino a quando non trovo lo status cercato
//se lo start va oltre i bit disponibili o non c'è restituisco -1
int BitMap_get(BitMap* bmp, int start, int status){
	if(start > bmp->num_bits) return -1;
	while(start < bmp->num_bits){
		//se il bit in posizione start è uguale a status lo ritorno
		//se no vado avanti
		if(BitMap_getBit(bmp, start) == status) return start;
		start++;
	}
	return -1;
}

int BitMap_set(BitMap* bmp, int pos, int status){
	if(pos >= bmp->num_bits) return -1;
	
	BitMapEntryKey map = BitMap_blockToIndex(pos);
	unsigned char num_bit = 1 << map.bit_num;
	unsigned char entry_i = bmp->entries[map.entry_num];
	if(status == 1){
		// OR logico per settare a 1 il bit_num
		bmp->entries[map.entry_num] = entry_i | num_bit;
		return entry_i | num_bit;
	}
	else{
		// AND logico per settare a 0 il bit_num
		//
		bmp->entries[map.entry_num] = entry_i & (~num_bit);
		return entry_i & (~num_bit);
	}
}

void BitMap_print(BitMap* bmp, int n){
	int i, bit;
	char mask2[] = {128, 64, 32, 16, 8, 4, 2, 1};
	for(i = 0; i < 8; i++){
		bit = ((bmp->entries[n] & mask2[i]) != 0);
		printf("%d", bit);
	}
}